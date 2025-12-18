// navigation.c (rewritten avoidance FSM for Ackermann/car-like robot)

#include "navigation.h"
#include "../embedded/servo_control.h"

#include <stdio.h>
#include <stdlib.h>   // abs
#include <math.h>     // fabsf
#include <stdbool.h>

// ===================== Tunables (avoidance) =====================

// Distance thresholds (meters)
#ifndef OBSTACLE_DISTANCE
#define OBSTACLE_DISTANCE 0.30f
#endif

// Enter/Exit hysteresis
#define OBS_ENTER_DIST   (OBSTACLE_DISTANCE)         // e.g., 0.30
#define OBS_EXIT_DIST    (OBSTACLE_DISTANCE + 0.20f) // e.g., 0.50

// Very close = push hard to reverse
#define OBS_CRASH_DIST   0.12f

// Loop counts (ticks) - tuned for ~20-50ms control loop
#define AVOID_BRAKE_TICKS     2
#define AVOID_REV_TICKS       18
#define AVOID_FWD_TURN_TICKS  22
#define AVOID_RECOVER_TICKS   12
#define AVOID_COOLDOWN_TICKS  15

// Speeds (units must match your motor mapping expectations)
#define V_SEARCH_FWD      (SEARCH_LINEAR_VEL)
#define V_AVOID_REV       (SEARCH_LINEAR_VEL)   // you can increase a bit if reverse is weak
#define V_AVOID_FWD       (SEARCH_LINEAR_VEL)   // you can increase a bit if forward arc is weak
#define V_RECOVER_FWD     (SEARCH_LINEAR_VEL)

// Steering magnitudes (ang_vel space)
#ifndef AVOID_TURN_STEER_ANGLE
// If not defined in navigation.h, provide a fallback
#define AVOID_TURN_STEER_ANGLE  1.2f
#endif

// In reverse, Ackermann yaw rate changes sign with velocity.
// To turn the same yaw direction while reversing, steer sign must be flipped.
// This file handles that internally.
#define STEER_MAG_AVOID   (AVOID_TURN_STEER_ANGLE)
#define STEER_MAG_SEARCH  (SEARCH_STEER_ANGLE)

// ===================== Neck scan params =====================

#define TILT_MIN   60.0f
#define TILT_MAX   110.0f
#define TILT_STEP  0.3f

#define PAN_MIN    45.0f
#define PAN_MAX    135.0f
#define PAN_STEP   1.5f

// APPROACH min distance
#define APPROACH_MIN_DISTANCE  0.10f

// ===================== Internal state =====================

static float neck_tilt     = 90.0f;
static float neck_tilt_dir = -1.0f;
static float neck_pan      = 90.0f;
static float neck_pan_dir  = 1.0f;

static float detect_pan = 90.0f;

typedef enum {
    AVOID_NONE = 0,
    AVOID_BRAKE,
    AVOID_REVERSE_ARC,
    AVOID_FORWARD_ARC,
    AVOID_RECOVER
} avoid_state_t;

static avoid_state_t g_avoid_state = AVOID_NONE;
static int g_avoid_ticks = 0;
static int g_cooldown_ticks = 0;

// Turn direction selection (+1 right or left depending on your convention)
// We only ensure "alternation"; exact left/right depends on your ang->steer mapping.
static int g_turn_sign = 1;
static int g_turn_toggle = 1;

// If we exit avoidance but still too close, we can force another avoidance with flipped dir
static bool g_need_clearance = false;

// ===================== Helpers =====================

static float clampf(float x, float lo, float hi) {
    if (x < lo) return lo;
    if (x > hi) return hi;
    return x;
}

static bool distance_valid(float d) {
    // Treat 10.0m / 0.0 as typical invalid placeholders; adjust if needed
    return (d > 0.01f && d < 9.0f);
}

// NOTE: Make this non-static to avoid header conflicts if navigation.h declares it.
void update_neck_scan(void) {
    neck_tilt += neck_tilt_dir * TILT_STEP;
    if (neck_tilt >= TILT_MAX) neck_tilt_dir = -1.0f;
    if (neck_tilt <= TILT_MIN) neck_tilt_dir =  1.0f;
    servo_set_angle(SERVO_NECK_TILT, neck_tilt);

    neck_pan += neck_pan_dir * PAN_STEP;
    if (neck_pan >= PAN_MAX) neck_pan_dir = -1.0f;
    if (neck_pan <= PAN_MIN) neck_pan_dir =  1.0f;
    servo_set_angle(SERVO_NECK_PAN, neck_pan);
}

float get_current_neck_tilt(void) {
    return neck_tilt;
}

static void avoid_start(int turn_sign, bool crash_mode) {
    g_turn_sign = turn_sign;

    g_avoid_state = AVOID_BRAKE;
    g_avoid_ticks = crash_mode ? (AVOID_BRAKE_TICKS / 2) : AVOID_BRAKE_TICKS;

    g_need_clearance = true;
}

static void avoid_set_next_dir_toggle(void) {
    g_turn_sign = g_turn_toggle;
    g_turn_toggle *= -1;
}

static void set_cmd(shared_state_t *s, float lin, float ang) {
    s->lin_vel = lin;
    s->ang_vel = ang;
}

// ===================== Public API =====================

void navigation_init(void) {
    neck_tilt = 90.0f;
    neck_pan  = 90.0f;
    g_avoid_state = AVOID_NONE;
    g_avoid_ticks = 0;
    g_cooldown_ticks = 0;
    g_need_clearance = false;

    detect_pan = 90.0f;

    servo_set_angle(SERVO_NECK_TILT, neck_tilt);
    servo_set_angle(SERVO_NECK_PAN,  neck_pan);

    printf("[navigation] init COMPLETED\n");
}

void navigation_reset_detect(void) {
    detect_pan = 90.0f;
    servo_set_angle(SERVO_NECK_PAN,  90.0f);
    servo_set_angle(SERVO_NECK_TILT, 90.0f);
}

void navigation_reset_avoid(void) {
    g_avoid_state = AVOID_NONE;
    g_avoid_ticks = 0;
    g_cooldown_ticks = 0;
    g_need_clearance = false;
}

// ===================== SEARCH Motion (new avoidance FSM) =====================

void compute_search_motion(shared_state_t *state) {
    update_neck_scan();

    if (g_cooldown_ticks > 0) g_cooldown_ticks--;

    const float d = state->distance;
    const bool d_ok = distance_valid(d);

    // If we've cleared enough distance, drop clearance requirement
    if (d_ok && d >= OBS_EXIT_DIST) {
        g_need_clearance = false;
    }

    // Hard crash guard: extremely close => start avoidance immediately (even during cooldown)
    if (d_ok && d < OBS_CRASH_DIST && g_avoid_state == AVOID_NONE) {
        avoid_set_next_dir_toggle();
        avoid_start(g_turn_sign, true);
    }

    // Run avoidance FSM if active
    if (g_avoid_state != AVOID_NONE) {
        if (g_avoid_ticks > 0) g_avoid_ticks--;

        switch (g_avoid_state) {
        case AVOID_BRAKE:
            // Short brake to unload wheels before reverse arc
            set_cmd(state, 0.0f, 0.0f);
            if (g_avoid_ticks <= 0) {
                g_avoid_state = AVOID_REVERSE_ARC;
                g_avoid_ticks = AVOID_REV_TICKS;
            }
            return;

        case AVOID_REVERSE_ARC: {
            // Reverse while changing heading.
            // IMPORTANT: reverse should flip steering sign to achieve same yaw direction.
            float lin = -V_AVOID_REV;
            float ang = -(float)g_turn_sign * STEER_MAG_AVOID; // sign flipped for reverse
            set_cmd(state, lin, ang);

            // If still extremely close, extend reverse a bit
            if (d_ok && d < OBS_CRASH_DIST && g_avoid_ticks < 6) {
                g_avoid_ticks = 6;
            }

            if (g_avoid_ticks <= 0) {
                g_avoid_state = AVOID_FORWARD_ARC;
                g_avoid_ticks = AVOID_FWD_TURN_TICKS;
            }
            return;
        }

        case AVOID_FORWARD_ARC: {
            // Forward arc to pass along the obstacle side
            float lin = V_AVOID_FWD;
            float ang = (float)g_turn_sign * STEER_MAG_AVOID;
            set_cmd(state, lin, ang);

            // If we regained clearance early, move to recover sooner
            if (!g_need_clearance && g_avoid_ticks > 6) {
                g_avoid_ticks = 6;
            }

            if (g_avoid_ticks <= 0) {
                g_avoid_state = AVOID_RECOVER;
                g_avoid_ticks = AVOID_RECOVER_TICKS;
            }
            return;
        }

        case AVOID_RECOVER:
            // Straighten briefly to stabilize heading and avoid re-trigger oscillation
            set_cmd(state, V_RECOVER_FWD, 0.0f);

            if (g_avoid_ticks <= 0) {
                g_avoid_state = AVOID_NONE;
                g_cooldown_ticks = AVOID_COOLDOWN_TICKS;

                // If we still haven't cleared, force another avoidance with opposite direction next time
                if (g_need_clearance && d_ok && d < OBS_EXIT_DIST) {
                    // do nothing here; next entry will toggle and retry
                }
            }
            return;

        default:
            g_avoid_state = AVOID_NONE;
            break;
        }
    }

    // Not in avoidance: decide whether to start avoidance
    if (d_ok && d < OBS_ENTER_DIST && g_cooldown_ticks == 0) {
        avoid_set_next_dir_toggle();
        avoid_start(g_turn_sign, false);
        // Immediate output for this tick
        set_cmd(state, 0.0f, 0.0f);
        return;
    }

    // If we still need clearance but we are not avoiding (e.g., cooldown), do not push into wall
    if (g_need_clearance && d_ok && d < OBS_EXIT_DIST) {
        set_cmd(state, 0.0f, 0.0f);
        return;
    }

    // Normal search: forward + optional gentle steer (usually 0)
    set_cmd(state, V_SEARCH_FWD, STEER_MAG_SEARCH);
}

// ===================== DETECT Motion =====================

bool compute_detect_motion(shared_state_t *state) {
    int hot_col = state->hot_col;
    if (hot_col < 0) {
        state->lin_vel = 0.0f;
        state->ang_vel = 0.0f;
        return false;
    }

    int col_diff = hot_col - THERMAL_CENTER_COL;

    // 머리로만 중앙 맞추기
    if (abs(col_diff) > CENTER_TOLERANCE) {
        detect_pan -= (float)col_diff * 0.5f;
        if (detect_pan < 45.0f)  detect_pan = 45.0f;
        if (detect_pan > 135.0f) detect_pan = 135.0f;

        servo_set_angle(SERVO_NECK_PAN,  detect_pan);
        servo_set_angle(SERVO_NECK_TILT, 90.0f);

        // 차체는 정지
        state->lin_vel = 0.0f;
        state->ang_vel = 0.0f;
        return false;
    }

    // aligned: 머리 각도 유지(리셋 금지)
    servo_set_angle(SERVO_NECK_PAN, detect_pan);
    servo_set_angle(SERVO_NECK_TILT, 90.0f);

    state->lin_vel = 0.0f;
    state->ang_vel = 0.0f;
    return true;
}


// ===================== APPROACH Motion =====================

void compute_approach_motion(shared_state_t *state) {
    if (state->distance < APPROACH_MIN_DISTANCE) {
        set_cmd(state, 0.0f, 0.0f);
        return;
    }

    float steer = 0.0f;
    if (state->hot_col >= 0) {
        int col_diff = state->hot_col - THERMAL_CENTER_COL;
        steer = (float)col_diff * APPROACH_STEER_GAIN;
        steer = clampf(steer, -1.0f, 1.0f);
    }

    float lin = APPROACH_LINEAR_VEL;
    if (state->distance < 0.5f) lin *= 0.7f;
    if (state->distance < 0.3f) lin *= 0.5f;

    state->lin_vel = lin;
    state->ang_vel = steer;
}
