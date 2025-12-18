// state_machine.h
#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H

#include "../common/common.h"
#include "shared_state.h"

// 온도 임계값 (°C)
#define FIRE_DETECT_THRESHOLD      60.0f   // 열원 감지
#define FIRE_APPROACH_THRESHOLD    180.0f  // 소화 시작 온도
#define FIRE_LOST_THRESHOLD        35.0f   // 열원 소실 판정
#define DELTA_TEMP_THRESHOLD       15.0f   // dT 감지 임계값

// 거리 임계값 (m)
#define APPROACH_DISTANCE          0.5f

void state_machine_init(void);

// 주의: 호출 전에 shared_state_lock 필요
void state_machine_update(shared_state_t *state);

// 내부 핸들러 함수 선언
void handle_idle(shared_state_t *state);
void handle_search(shared_state_t *state);
void handle_detect(shared_state_t *state);
void handle_approach(shared_state_t *state);
void handle_extinguish(shared_state_t *state);

#endif