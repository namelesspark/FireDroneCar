// state_machine.h
#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H

#pragma once
#include "../common/common.h"
#include "shared_state.h"

// 열원 온도 임계값 추가함
#define FIRE_DETECT_THRESHOLD      80.0f // 열원 감지 온도
#define FIRE_APPROACH_THRESHOLD    100.0f // 접근 시작 온도
#define EXTINGUISH_TEMP_THRESHOLD  60.0f // 소화 완료 온도
#define APPROACH_DISTANCE          0.5f    // 소화 시작 거리

void state_machine_init(void);

void state_machine_update(shared_state_t *state);

// 각 상태별 핸들러 (내부용)
void state_idle_handler(shared_state_t *state);
void state_search_handler(shared_state_t *state);
void state_detect_handler(shared_state_t *state);
void state_approach_handler(shared_state_t *state);
void state_extinguish_handler(shared_state_t *state);
void state_safe_stop_handler(shared_state_t *state);

#endif