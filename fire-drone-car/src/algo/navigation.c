// navigation.c
// 주의: 이 파일의 모든 함수는 lock이 잡힌 상태에서 호출됨

#include "navigation.h"
#include "../embedded/servo_control.h"
#include <stdio.h>
#include <stdlib.h>

#define AVOID_REVERSE_COUNT  10   // 후진 유지 횟수 (50ms * 10 = 500ms)
#define AVOID_TURN_COUNT     15   // 회전 유지 횟수 (50ms * 15 = 750ms)

#define TILT_MIN   60.0f
#define TILT_MAX   110.0f   // 150 → 110으로 줄임
#define TILT_STEP  0.3f

#define PAN_MIN    45.0f
#define PAN_MAX    135.0f
#define PAN_STEP   1.5f     // 좌우는 더 빠르게

// 머리 상하 (TILT)
static float neck_tilt = 90.0f;
static float neck_tilt_dir = -1.0f;  // -1: 아래로, 1: 위로

// 머리 좌우 (PAN)
static float neck_pan = 90.0f;
static float neck_pan_dir = 1.0f;

// 장애물 회피 상태
typedef enum {
    AVOID_NONE = 0,     // 정상 주행
    AVOID_REVERSE,      // 후진 중
    AVOID_TURN          // 회전 중
} avoid_state_t;

static avoid_state_t avoid_state = AVOID_NONE;
static int avoid_counter = 0;  // 상태 유지 카운터 (호출 횟수 기준)

void navigation_init(void) {
    neck_tilt = 90.0f;
    neck_pan = 90.0f;
    avoid_state = AVOID_NONE;
    avoid_counter = 0;
    servo_set_angle(SERVO_NECK_TILT, neck_tilt);
    servo_set_angle(SERVO_NECK_PAN, neck_pan);
    printf("[navigation] init COMPLETED\n");
    printf("[navigation] init COMPLETED\n");
}


// ===== 머리 상하좌우 스캔 =====
void update_neck_scan(void) {
    // 상하 (느리게)
    neck_tilt += neck_tilt_dir * TILT_STEP;
    if (neck_tilt >= TILT_MAX) neck_tilt_dir = -1.0f;
    if (neck_tilt <= TILT_MIN) neck_tilt_dir = 1.0f;
    servo_set_angle(SERVO_NECK_TILT, neck_tilt);

    // 좌우 (빠르게)
    neck_pan += neck_pan_dir * PAN_STEP;
    if (neck_pan >= PAN_MAX) neck_pan_dir = -1.0f;
    if (neck_pan <= PAN_MIN) neck_pan_dir = 1.0f;
    servo_set_angle(SERVO_NECK_PAN, neck_pan);
}

float get_current_neck_tilt(void) {
    return neck_tilt;
}


// ===== SEARCH: 전진하며 회전 탐색 =====
void compute_search_motion(shared_state_t *state) {
    update_neck_scan();

    // 회피 시퀀스 진행 중이면 처리
    if (avoid_state != AVOID_NONE) {
        avoid_counter--;

        if (avoid_state == AVOID_REVERSE) {
            // 1단계: 후진 + 반대 조향
            state->lin_vel = -SEARCH_LINEAR_VEL;
            state->ang_vel = -SEARCH_STEER_ANGLE;

            if (avoid_counter <= 0) {
                // 후진 완료 → 회전 단계로
                avoid_state = AVOID_TURN;
                avoid_counter = AVOID_TURN_COUNT;
            }
        }
        else if (avoid_state == AVOID_TURN) {
            // 2단계: 정지 + 반대 조향 (제자리 회전 효과)
            state->lin_vel = SEARCH_LINEAR_VEL;
            state->ang_vel = -SEARCH_STEER_ANGLE * 2.0f;

            if (avoid_counter <= 0) {
                // 회전 완료 → 정상 주행
                avoid_state = AVOID_NONE;
            }
        }
        return;
    }

    if (state->distance < OBSTACLE_DISTANCE) {
        printf("[nav] 장애물 감지 (d=%.2f) → 후진 시작\n", state->distance);
        avoid_state = AVOID_REVERSE;
        avoid_counter = AVOID_REVERSE_COUNT;

        state->lin_vel = -SEARCH_LINEAR_VEL;
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
    static float detect_pan = 90.0f;

    int hot_col = state->hot_col;

    // 열원 못 찾음 → 느린 탐색
    if (hot_col < 0) {
        state->lin_vel = 0.0f;
        state->ang_vel = 0.0f;
        return false;
    }

    int col_diff = hot_col - THERMAL_CENTER_COL;

    // 1단계: 고개로 열원 중앙 정렬
    if (abs(col_diff) > CENTER_TOLERANCE) {
        // 열원이 오른쪽 → 고개 오른쪽으로 (각도 감소)
        // 열원이 왼쪽 → 고개 왼쪽으로 (각도 증가)
        detect_pan -= (float)col_diff * 0.5f;
        
        // 범위 제한
        if (detect_pan < 45.0f) detect_pan = 45.0f;
        if (detect_pan > 135.0f) detect_pan = 135.0f;
        
        servo_set_angle(SERVO_NECK_PAN, detect_pan);
        servo_set_angle(SERVO_NECK_TILT, 90.0f);
        
        state->lin_vel = 0.0f;
        state->ang_vel = 0.0f;
        return false;
    }

    // 2단계: 고개가 중앙(90도)인지 확인
    float pan_diff = detect_pan - 90.0f;
    
    if (fabsf(pan_diff) > 5.0f) {
        // 고개가 틀어져 있음 → 차체를 고개 방향으로 회전
        // pan < 90 → 고개가 오른쪽 → 차체 오른쪽 회전 (ang > 0)
        // pan > 90 → 고개가 왼쪽 → 차체 왼쪽 회전 (ang < 0)
        float steer = -pan_diff * 0.02f;
        if (steer > 0.5f) steer = 0.5f;
        if (steer < -0.5f) steer = -0.5f;
        
        state->lin_vel = DETECT_LINEAR_VEL;
        state->ang_vel = steer;
        
        // 차체 회전하면서 고개는 서서히 중앙으로
        detect_pan += (90.0f - detect_pan) * 0.1f;
        servo_set_angle(SERVO_NECK_PAN, detect_pan);
        
        return false;
    }

    // 3단계: 열원 중앙 + 고개 중앙 → 정렬 완료
    detect_pan = 90.0f;
    servo_set_angle(SERVO_NECK_PAN, 90.0f);
    state->lin_vel = 0.0f;
    state->ang_vel = 0.0f;
    
    return true;
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
