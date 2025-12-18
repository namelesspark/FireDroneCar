#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>
#include <stdbool.h>
#include <wiringPi.h>

#include "../common/common.h"
#include "../comm_ui/comm_ui.h"
#include "../algo/shared_state.h"
#include "../algo/state_machine.h"
#include "../algo/navigation.h"
#include "../algo/extinguish_logic.h"
#include "../embedded/thermal_sensor.h"
#include "../embedded/motor_control.h"
#include "../embedded/pump_control.h"
#include "../embedded/servo_control.h"
#include "../embedded/ultrasonic.h"


static volatile bool g_running = true;
static shared_state_t g_state;

// Signal 핸들러 Ctrl + C
void signal_handler(int sig) {
    (void)sig;  // unused parameter warning 방지
    g_running = false;
}


// 센서 스레드
void* sensor_thread_func(void* arg) {
    (void)arg;

    ultrasonic_t us; // 초음파센서 초기화
    if (ultrasonic_init(&us, 23, 24) != 0) {
        fprintf(stderr, "[sensor_thread] 초음파 센서 초기화 실패\n");
        return NULL;
    }

    thermal_data_t thermal;

    printf("[sensor_thread] STARTED\n");

    while(g_running) {
        // 열화상 카메라 읽기
        if(thermal_sensor_read(&thermal) == 0) {
            shared_state_lock(&g_state);
            g_state.t_fire = thermal.T_fire;
            g_state.dT = thermal.T_fire - thermal.T_env;
            g_state.hot_row = thermal.hot_row;
            g_state.hot_col = thermal.hot_col;
            shared_state_unlock(&g_state);
        }

        static int count = 0;
        if (++count >= 10) {
            // 수정: thermal.Tfire -> thermal.T_fire, thermal.Tenv -> thermal.T_env
            printf("[sensor] T_fire: %.1f도, dT: %.1f, 열: (%d, %d)\n", 
                   thermal.T_fire, thermal.T_fire - thermal.T_env, 
                   thermal.hot_row, thermal.hot_col);
            count = 0;
        }

        // 초음파 센서 읽기
        // 수정: dis_cm -> dist_cm 통일

        float dist_cm = ultrasonic_read_distance_cm_avg(&us, 3, 30000);
        if (dist_cm > 0) {
            shared_state_lock(&g_state);
            g_state.distance = dist_cm/100;
            shared_state_unlock(&g_state);

        }

        usleep(100000);  // 100ms (10Hz) - 센서 읽기 주기
    }

    printf("[sensor_thread] STOPPED\n");
    return NULL;
}



// 알고리즘 스레드
void* algo_thread_func(void* arg) {
    (void)arg;

    printf("[algo_thread] STARTED\n");

    while(g_running) {
        state_machine_update(&g_state);
        usleep(50000); // 50ms = 20Hz 이 시간마다 갱신
    }

    printf("[algo_thread] STOPPED\n");
    return NULL;
}


void* motor_thread_func(void* arg) {
    (void)arg;

    printf("[motor_thread] STARTED\n");

    while(g_running) {
        shared_state_lock(&g_state);
        float lin = g_state.lin_vel;
        float ang = g_state.ang_vel;  // 수정: ang_val -> ang_vel
        bool estop = g_state.emergency_stop;
        shared_state_unlock(&g_state);

        if(estop) {
            motor_stop();
            servo_set_angle(SERVO_STEER, 90.0f);
        } else {
            motor_set_speed(lin); // 뒷바퀴 DC 모터 제어
            float steering_angle = 90.0f + (ang * 45.0f);  // -45도 ~ +45도 범위
            if (steering_angle < 45.0f)  steering_angle = 45.0f; // 각도 제한
            if (steering_angle > 135.0f) steering_angle = 135.0f;
            servo_set_angle(SERVO_STEER, steering_angle);
        }
        usleep(20000);  // 20ms (50Hz)
    }
    motor_stop();
    servo_set_angle(SERVO_STEER, 90.0f);  // 조향 중앙으로
    printf("[motor_thread] STOPPED\n");
    return NULL;
}

int main(void) {
    printf("========================================\n");
    printf("========================================\n");
    printf("        FireDroneCar 구동 시작           \n");
    printf("========================================\n");
    printf("========================================\n");

    if (wiringPiSetupGpio() == -1) {
        printf("[메인] wiringPiSetupGpio failed\n");
        return 1;
    }
    printf("[메인] wiringPi GPIO setup OK (BCM numbering)\n");

    // Signal 핸들러 등록
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    // shared_state 초기화
    shared_state_init(&g_state);
    printf("[메인] shared state 초기화 완료\n");

    // 하드웨어 초기화
    if(thermal_sensor_init() != 0) {
        fprintf(stderr, "[메인] 열화상 카메라 초기화 오류\n");
        return -1;
    }
    printf("[메인] 열화상 카메라 정상\n");

    if (motor_init() != 0) {
        fprintf(stderr, "[메인] 모터 초기화 오류\n");
        return -1;
    }
    printf("[메인] 모터 정상\n");
    
    if (pump_init() != 0) {
        fprintf(stderr, "[메인] 워터 펌프 초기화 오류\n");
        return -1;
    }
    printf("[메인] 워터 펌프 정상\n");
    
    if (servo_init() != 0) {
        fprintf(stderr, "[메인] 서보 모터 초기화 오류\n");
        return -1;
    }
    printf("[메인] 서보 모터 정상\n");

    // 알고리즘 초기화
    state_machine_init();
    navigation_init();
    extinguish_init();
    servo_set_angle(SERVO_STEER, 90.0f);

    // 스레드 생성
    pthread_t sensor_thread, algo_thread, motor_thread;
    //pthread_t comm_thread;

    // 수정: pthread_create 호출 추가
    if (pthread_create(&sensor_thread, NULL, sensor_thread_func, NULL) != 0) {
        fprintf(stderr, "[메인] sensor_thread 생성 실패\n");
        return -1;
    }
    
    if (pthread_create(&algo_thread, NULL, algo_thread_func, NULL) != 0) {
        fprintf(stderr, "[메인] algo_thread 생성 실패\n");
        g_running = false;
        pthread_join(sensor_thread, NULL);
        return -1;
    }
    
    if (pthread_create(&motor_thread, NULL, motor_thread_func, NULL) != 0) {
        fprintf(stderr, "[메인] motor_thread 생성 실패\n");
        g_running = false;
        pthread_join(sensor_thread, NULL);
        pthread_join(algo_thread, NULL);
        return -1;
    }

    // comm_thread는 comm_ui.h의 함수 사용
    // TODO: comm_thread 함수 구현 필요

    printf("[메인] 스레드 생성 완료\n");
    printf("[메인] 시스템 가동\n");
    printf("[메인] Ctrl + C로 동작 중지\n");

    // 메인 루프
    while (g_running) {
        sleep(1);  // 5초마다 상태 출력
        
        shared_state_lock(&g_state);
        robot_mode_t mode = g_state.mode;
        float T = g_state.t_fire;
        float d = g_state.distance;
        int wl = g_state.water_level;
        shared_state_unlock(&g_state);
        
        const char* mode_str[] = {"IDLE", "SEARCH", "DETECT", "APPROACH", "EXTINGUISH", "SAFE_STOP"};
        printf("\n[메인] State: %s, T=%.1f°C, Dist=%.2fcm, Water=%d\n", mode_str[mode], T, d, wl);
    }

    // 스레드 종료 대기
    printf("[메인] 스레드 종료 대기중...\n");
    pthread_join(sensor_thread, NULL);
    pthread_join(algo_thread, NULL);
    pthread_join(motor_thread, NULL);
    // pthread_join(comm_thread, NULL);

    // 정리
    motor_stop();
    pump_off();
    servo_set_angle(SERVO_STEER, 90.0f);
    shared_state_destroy(&g_state);

    printf("[메인] 시스템 종료\n");
    return 0;
}