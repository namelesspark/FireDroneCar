// shared_state.h
#ifndef SHARED_STATE_H
#define SHARED_STATE_H

#include "../common/common.h"

void shared_state_init(shared_state_t *state);
void shared_state_destroy(shared_state_t *state);

// main.c의 각 스레드 진입점에서만 사용
void shared_state_lock(shared_state_t *state);
void shared_state_unlock(shared_state_t *state);

#endif
