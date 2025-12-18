// navigation.h
#ifndef NAVIGATION_H
#define NAVIGATION_H

#include "../common/common.h"
#include "shared_state.h"

// 속도/조향 상수
#define SEARCH_LINEAR_VEL       0.2f
#define SEARCH_STEER_ANGLE      0.6f

#define DETECT_LINEAR_VEL       0.08f
#define DETECT_STEER_GAIN       0.15f

#define APPROACH_LINEAR_VEL     0.15f
#define APPROACH_STEER_GAIN     0.1f

// 열화상 센서 (16x12)
#define THERMAL_CENTER_COL      8

// 중앙 정렬 허용 오차 (픽셀)
#define CENTER_TOLERANCE        2

// 장애물 거리 (m)
#define OBSTACLE_DISTANCE       0.30f

void navigation_init(void);

// 주의: 모든 함수는 lock이 잡힌 상태에서 호출됨
void compute_search_motion(shared_state_t *state);
bool compute_detect_motion(shared_state_t *state);  // true = 정렬 완료
void compute_approach_motion(shared_state_t *state);

// 머리 상하 스캔 (search에서 호출)
void update_neck_scan(void);
float get_current_neck_tilt(void);

#endif
