// navigation.c
// 주의: 이 파일의 모든 함수는 lock이 잡힌 상태에서 호출됨

#include "navigation.h"
#include <stdio.h>
#include <stdlib.h>

void navigation_init(void) {
    printf("[navigation] init COMPLETED\n");
}

// ===== SEARCH: 전진하며 회전 탐색 =====
void compute_search_motion(shared_state_t *state) {
    // 장애물 있으면 반대로 조향
    if (state->distance < OBSTACLE_DISTANCE) {
        state->lin_vel = SEARCH_LINEAR_VEL * 0.5f;
        state->ang_vel = -SEARCH_STEER_ANGLE;
        return;
    }

    // 전진하며 한쪽으로 조향
    state->lin_vel = SEARCH_LINEAR_VEL;
    state->ang_vel = SEARCH_STEER_ANGLE;
}

// ===== DETECT: 전진하며 열원 중앙 정렬 =====
// 반환: true = 정렬 완료, false = 진행 중
bool compute_detect_motion(shared_state_t *state) {
    int hot_col = state->hot_col;

    // 열원 못 찾음 → 느린 탐색
    if (hot_col < 0) {
        state->lin_vel = DETECT_LINEAR_VEL;
        state->ang_vel = SEARCH_STEER_ANGLE * 0.5f;
        return false;
    }

    int col_diff = hot_col - THERMAL_CENTER_COL;

    // 중앙 정렬 완료
    if (abs(col_diff) <= CENTER_TOLERANCE) {
        state->lin_vel = 0.0f;
        state->ang_vel = 0.0f;
        return true;
    }

    // 열원 방향으로 조향하며 전진
    float steer = (float)col_diff * DETECT_STEER_GAIN;
    if (steer > 1.0f) steer = 1.0f;
    if (steer < -1.0f) steer = -1.0f;

    state->lin_vel = DETECT_LINEAR_VEL;
    state->ang_vel = steer;

    return false;
}

// ===== APPROACH: 열원 방향으로 접근 =====
void compute_approach_motion(shared_state_t *state) {
    // 장애물 있으면 정지
    if (state->distance < OBSTACLE_DISTANCE) {
        state->lin_vel = 0.0f;
        state->ang_vel = 0.0f;
        return;
    }

    // 열원 방향 보정
    float steer = 0.0f;
    if (state->hot_col >= 0) {
        int col_diff = state->hot_col - THERMAL_CENTER_COL;
        steer = (float)col_diff * APPROACH_STEER_GAIN;
        if (steer > 1.0f) steer = 1.0f;
        if (steer < -1.0f) steer = -1.0f;
    }

    // 거리에 따른 속도
    float lin = APPROACH_LINEAR_VEL;
    if (state->distance < 0.5f) lin *= 0.5f;
    if (state->distance < 0.3f) lin *= 0.5f;

    state->lin_vel = lin;
    state->ang_vel = steer;
}
