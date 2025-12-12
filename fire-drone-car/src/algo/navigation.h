// navigation.h

#ifndef NAVIGATION_H
#define NAVIGATION_H

#include "../common/common.h"
#include "shared_state.h"

// 탐색 회전 속도 (rad/s)
#define SEARCH_ANGULAR_VEL      0.5f

// 접근 속도 (m/s)
#define APPROACH_LINEAR_VEL     0.15f
#define APPROACH_ANGULAR_GAIN   0.8f   // 방향 보정 게인

// 열화상 센서 중앙 픽셀 (16x12)
#define THERMAL_CENTER_COL      8
#define THERMAL_CENTER_ROW      6
#define THERMAL_COLS            16
#define THERMAL_ROWS            12

// 픽셀당 각도 (55도 FOV 기준)
#define PIXEL_TO_ANGLE          (55.0f / 16.0f)  // 약 3.44도/픽셀

// 장애물 회피 거리 (m)
#define OBSTACLE_DISTANCE       0.30f



// 네비게이션 초기화
void navigation_init(void);

// SEARCH 모드: 제자리 회전 탐색
void compute_search_motion(shared_state_t *state);

// DETECT 모드: 미세 조정 회전
void compute_detect_motion(shared_state_t *state);

// APPROACH 모드: 화재 방향으로 접근
void compute_approach_motion(shared_state_t *state);

// 열화상 픽셀 -> 각도 변환
float pixel_to_angle(int col);

// 장애물 회피 체크
bool check_obstacle(shared_state_t *state);

#endif // NAVIGATION_H
