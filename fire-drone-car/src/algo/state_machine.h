// state_machine.h
#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H

#pragma once
#include "../common/common.h"
#include "shared_state.h"

void state_machine_init(void);

void state_machine_update(shared_state_t *state, pthread_mutex_t *mutex);

// 각 상태별 핸들러 (내부용)
void state_idle_handler(shared_state_t *state);
void state_search_handler(shared_state_t *state);
void state_detect_handler(shared_state_t *state);
void state_approach_handler(shared_state_t *state);
void state_extinguish_handler(shared_state_t *state);
void state_safe_stop_handler(shared_state_t *state);