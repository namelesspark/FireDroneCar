// navigation.c

#include <stdio.h>
#include <stdlib.h>
#include "navigation.h"
#include <math.h>

// embedded ultrasonic.h, motor_control 헤더 포함 필요함

void navigation_init(void) {
    printf("[navigation] init COMPLETED\n");
}


// SEARCH 상태 -> 일단 제자리를 회전하면서 탐색하는 로직
void compute_search_motion(shared_state_t *state) {
    // 장애물 체크
    if (check_obstacle(state)) {
        // 장애물 있으면 잠깐 멈추고 반대로 회전
        shared_state_set_velocity(state, 0.0f, -SEARCH_ANGULAR_VEL);
        return;
    }

    // 천천히 전진하며 회전
    shared_state_set_velocity(state, 0.1f, SEARCH_ANGULAR_VEL);
}

// DETECT 상태
void compute_detect_motion(shared_state_t *state) {
    shared_state_lock(state);
    int hot_col = state->hot_col;
    shared_state_unlock(state);

    // hot_col이 안잡히면 계속 탐색
    if (hot_col < 0) {
        shared_state_set_velocity(state, 0.0f, SEARCH_ANGULAR_VEL * 0.3f);
        return;
    }

    int col_diff = hot_col - THERMAL_CENTER_COL; // 중앙과 차이 계싼

    //중앙에 가까우면 정렬 완료
    if(abs(col_diff) <= 2) {
        printf("navigation DETECT SUCCESS! 열원 중앙 정렬 완료\n");
        shared_state_set_velocity(state, 0.0f, 0.0f);
        return;
    }

    // 열원 방향으로 회전
    float angle_error = pixel_to_angle(col_diff);
    float ang_vel = angle_error * 0.05f;

    shared_state_set_velocity(state, 0.0f, ang_vel);
}


// APPROACH 모드: 화재 방향으로 접근
void compute_approach_motion(shared_state_t *state) {
    // 장애물 체크
    if (check_obstacle(state)) {
        printf("navigation APPROACH 장애물 감지\n");
        shared_state_set_velocity(state, 0.0f, 0.0f);
        return;
    }

    shared_state_lock(state);
    int hot_col = state->hot_col;
    float distance = state->distance;
    shared_state_unlock(state);

    // 열원이 중앙에서 벗어났으면 회전하면서 전진
    float ang_vel = 0.0f;
    if (hot_col >= 0) {
        int col_diff = hot_col - THERMAL_CENTER_COL;
        float angle_error = pixel_to_angle(col_diff);
        ang_vel = angle_error * APPROACH_ANGULAR_GAIN * 0.01f;

        // 각속도 제한
        if (ang_vel > 0.5f) ang_vel = 0.5f;
        if (ang_vel < -0.5f) ang_vel = -0.5f;
    }

    // 거리에 따른 속도 조절
    float lin_vel = APPROACH_LINEAR_VEL;

    // 가까워지면 속도 줄이기
    if (distance < 0.5f) {
        lin_vel = APPROACH_LINEAR_VEL * 0.5f;
    }
    if (distance < 0.3f) {
        lin_vel = APPROACH_LINEAR_VEL * 0.3f;
    }

    shared_state_set_velocity(state, lin_vel, ang_vel);

    // TODO - embedded 함수 구현 후 motor_set_velocity(lin_vel, ang_vel);

    printf("[navigation] APPROACH: lin=%.2f m/s, ang=%.2f rad/s, dist=%.2fm\n",
           lin_vel, ang_vel, distance);
}

// 픽셀 위치 -> 각도 변환
float pixel_to_angle(int col_offset) {
    // col_offset: 중앙(0)에서의 픽셀 차이
    // 양수 = 오른쪽, 음수 = 왼쪽
    return (float)col_offset * PIXEL_TO_ANGLE;
}

// 장애물 회피 체크
bool check_obstacle(shared_state_t *state) {
    shared_state_lock(state);
    float distance = state->distance;
    shared_state_unlock(state);

    // 실제 초음파 센서에서 읽어온 값 사용
    // distance = ultrasonic_get_distance();

    if (distance < OBSTACLE_DISTANCE) {
        return true;  // 장애물 있음
    }
    return false;  // 장애물 없음
}

