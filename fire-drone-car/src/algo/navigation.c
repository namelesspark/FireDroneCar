// navigation.c
#include "navigation.h"
#include "../embedded/servo_control.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define AVOID_REVERSE_COUNT  10
#define AVOID_TURN_COUNT     15

#define TILT_MIN   60.0f
#define TILT_MAX   110.0f
#define TILT_STEP  0.3f

#define PAN_MIN    45.0f
#define PAN_MAX    135.0f
#define PAN_STEP   1.5f

// APPROACH용 최소 거리 (장애물 거리보다 작게)
#define APPROACH_MIN_DISTANCE  0.10f

static float neck_tilt = 90.0f;
static float neck_tilt_dir = -1.0f;
static float neck_pan = 90.0f;
static float neck_pan_dir = 1.0f;

typedef enum {
    AVOID_NONE = 0,
    AVOID_REVERSE,
    AVOID_TURN
} avoid_state_t;

static avoid_state_t avoid_state = AVOID_NONE;
static int avoid_counter = 0;

// DETECT용 고개 각도 (static → 리셋 함수 필요)
static float detect_pan = 90.0f;

void navigation_init(void) {
    neck_tilt = 90.0f;
    neck_pan = 90.0f;
    avoid_state = AVOID_NONE;
    avoid_counter = 0;
    detect_pan = 90.0f;
    servo_set_angle(SERVO_NECK_TILT, neck_tilt);
    servo_set_angle(SERVO_NECK_PAN, neck_pan);
    printf("[navigation] init COMPLETED\n");
}

// 상태 리셋 (DETECT 진입 시 호출)
void navigation_reset_detect(void) {
    detect_pan = 90.0f;
    servo_set_angle(SERVO_NECK_PAN, 90.0f);
    servo_set_angle(SERVO_NECK_TILT, 90.0f);
}

// 회피 상태 리셋 (SEARCH 벗어날 때 호출)
void navigation_reset_avoid(void) {
    avoid_state = AVOID_NONE;
    avoid_counter = 0;
}

void update_neck_scan(void) {
    neck_tilt += neck_tilt_dir * TILT_STEP;
    if (neck_tilt >= TILT_MAX) neck_tilt_dir = -1.0f;
    if (neck_tilt <= TILT_MIN) neck_tilt_dir = 1.0f;
    servo_set_angle(SERVO_NECK_TILT, neck_tilt);

    neck_pan += neck_pan_dir * PAN_STEP;
    if (neck_pan >= PAN_MAX) neck_pan_dir = -1.0f;
    if (neck_pan <= PAN_MIN) neck_pan_dir = 1.0f;
    servo_set_angle(SERVO_NECK_PAN, neck_pan);
}

float get_current_neck_tilt(void) {
    return neck_tilt;
}

void compute_search_motion(shared_state_t *state) {
    update_neck_scan();

    if (avoid_state != AVOID_NONE) {
        avoid_counter--;

        if (avoid_state == AVOID_REVERSE) {
            state->lin_vel = -SEARCH_LINEAR_VEL;
            state->ang_vel = -SEARCH_STEER_ANGLE;
            if (avoid_counter <= 0) {
                avoid_state = AVOID_TURN;
                avoid_counter = AVOID_TURN_COUNT;
            }
        }
        else if (avoid_state == AVOID_TURN) {
            state->lin_vel = SEARCH_LINEAR_VEL;
            state->ang_vel = -SEARCH_STEER_ANGLE * 2.0f;
            if (avoid_counter <= 0) {
                avoid_state = AVOID_NONE;
            }
        }
        return;
    }

    if (state->distance < OBSTACLE_DISTANCE) {
        avoid_state = AVOID_REVERSE;
        avoid_counter = AVOID_REVERSE_COUNT;
        state->lin_vel = -SEARCH_LINEAR_VEL;
        state->ang_vel = -SEARCH_STEER_ANGLE;
        return;
    }

    state->lin_vel = SEARCH_LINEAR_VEL;
    state->ang_vel = SEARCH_STEER_ANGLE;
}

bool compute_detect_motion(shared_state_t *state) {
    int hot_col = state->hot_col;

    if (hot_col < 0) {
        state->lin_vel = 0.0f;
        state->ang_vel = 0.0f;
        return false;
    }

    int col_diff = hot_col - THERMAL_CENTER_COL;

    if (abs(col_diff) > CENTER_TOLERANCE) {
        detect_pan -= (float)col_diff * 0.5f;
        if (detect_pan < 45.0f) detect_pan = 45.0f;
        if (detect_pan > 135.0f) detect_pan = 135.0f;
        
        servo_set_angle(SERVO_NECK_PAN, detect_pan);
        servo_set_angle(SERVO_NECK_TILT, 90.0f);
        
        state->lin_vel = 0.0f;
        state->ang_vel = 0.0f;
        return false;
    }

    float pan_diff = detect_pan - 90.0f;
    
    if (fabsf(pan_diff) > 5.0f) {
        float steer = -pan_diff * 0.02f;
        if (steer > 0.5f) steer = 0.5f;
        if (steer < -0.5f) steer = -0.5f;
        
        state->lin_vel = DETECT_LINEAR_VEL;
        state->ang_vel = steer;
        
        detect_pan += (90.0f - detect_pan) * 0.1f;
        servo_set_angle(SERVO_NECK_PAN, detect_pan);
        
        return false;
    }

    detect_pan = 90.0f;
    servo_set_angle(SERVO_NECK_PAN, 90.0f);
    state->lin_vel = 0.0f;
    state->ang_vel = 0.0f;
    
    return true;
}

void compute_approach_motion(shared_state_t *state) {
    // APPROACH에서는 더 가까이 갈 수 있게
    if (state->distance < APPROACH_MIN_DISTANCE) {
        state->lin_vel = 0.0f;
        state->ang_vel = 0.0f;
        return;
    }

    float steer = 0.0f;
    if (state->hot_col >= 0) {
        int col_diff = state->hot_col - THERMAL_CENTER_COL;
        steer = (float)col_diff * APPROACH_STEER_GAIN;
        if (steer > 1.0f) steer = 1.0f;
        if (steer < -1.0f) steer = -1.0f;
    }

    float lin = APPROACH_LINEAR_VEL;
    if (state->distance < 0.5f) lin *= 0.7f;
    if (state->distance < 0.3f) lin *= 0.5f;

    state->lin_vel = lin;
    state->ang_vel = steer;
}