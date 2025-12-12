// src/tests/test_steer.c

#include <stdio.h>
#include <unistd.h>   // sleep, usleep
#include "../embedded/servo_control.h"
#include "../embedded/motor_control.h"

static void steer_only_test(void)
{
    float steer_center = 90.0f;
    float steer_left   = 60.0f;   // 필요에 따라 더/덜 꺾기
    float steer_right  = 120.0f;

    printf("[test_steer] center steering...\n");
    servo_set_angle(SERVO_STEER, steer_center);
    sleep(1);

    printf("[test_steer] steer left...\n");
    servo_set_angle(SERVO_STEER, steer_left);
    sleep(1);

    printf("[test_steer] steer right...\n");
    servo_set_angle(SERVO_STEER, steer_right);
    sleep(1);

    printf("[test_steer] back to center...\n");
    servo_set_angle(SERVO_STEER, steer_center);
    sleep(1);
}

static void drive_with_steer_test(void)
{
    float steer_center = 90.0f;
    float steer_left   = 70.0f;
    float steer_right  = 110.0f;

    float speed = 0.3f; // 너무 빠르지 않게 (0.0~1.0)

    printf("[test_steer] forward with center steering...\n");
    servo_set_angle(SERVO_STEER, steer_center);
    motor_set_speed(speed);
    sleep(2);

    printf("[test_steer] forward with left steering...\n");
    servo_set_angle(SERVO_STEER, steer_left);
    sleep(2);

    printf("[test_steer] forward with right steering...\n");
    servo_set_angle(SERVO_STEER, steer_right);
    sleep(2);

    printf("[test_steer] stop.\n");
    motor_stop();
    servo_set_angle(SERVO_STEER, steer_center);
    sleep(1);
}

int main(void)
{
    printf("==== test_steer: steering servo + DC motor test ====\n");

    if (servo_init() != 0) {
        fprintf(stderr, "[test_steer] servo_init failed\n");
        return 1;
    }

    if (motor_init() != 0) {
        fprintf(stderr, "[test_steer] motor_init failed\n");
        return 1;
    }

    // 초기 상태: 조향 중앙, 모터 정지
    servo_set_angle(SERVO_STEER, 90.0f);
    motor_stop();
    sleep(1);

    printf("\n=== 1단계: 조향 서보만 테스트 ===\n");
    steer_only_test();

    printf("\n=== 2단계: 천천히 전진하면서 좌/우 조향 테스트 ===\n");
    printf("⚠ 바퀴가 굴러가니, 로봇을 손으로 잡거나 넓은 공간에서 테스트할 것!\n");
    drive_with_steer_test();

    printf("[test_steer] done. Stopping motor and centering steer.\n");
    motor_stop();
    servo_set_angle(SERVO_STEER, 90.0f);
    return 0;
}
