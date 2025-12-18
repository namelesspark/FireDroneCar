// navigation.h
#ifndef NAVIGATION_H
#define NAVIGATION_H

#include "../common/common.h"
#include "shared_state.h"

#define SEARCH_LINEAR_VEL       0.20f
#define SEARCH_STEER_ANGLE      0.0f

#define AVOID_REVERSE_STEER_ANGLE  0.0f   // 후진은 보통 핸들 고정(원하면 값 줘도 됨)
#define AVOID_TURN_STEER_ANGLE     1.0f

#define DETECT_LINEAR_VEL       0.12f
#define DETECT_STEER_GAIN       0.15f

#define APPROACH_LINEAR_VEL     0.15f
#define APPROACH_STEER_GAIN     0.1f

#define THERMAL_CENTER_COL      8
#define CENTER_TOLERANCE        2
#define OBSTACLE_DISTANCE       0.30f

void navigation_init(void);
void navigation_reset_detect(void);  // 추가
void navigation_reset_avoid(void);   // 추가

void compute_search_motion(shared_state_t *state);
bool compute_detect_motion(shared_state_t *state);
void compute_approach_motion(shared_state_t *state);

void update_neck_scan(void);
float get_current_neck_tilt(void);

#endif