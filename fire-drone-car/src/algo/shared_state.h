// shared_state.h
#ifndef SHARED_STATE_H
#define SHARED_STATE_H

#include "../common/common.h"


// 초기화 및 관리
void shared_state_init(shared_state_t *state);
void shared_state_destroy(shared_state_t *state);

// 뮤텍스 잠금 해제
void shared_state_lock(shared_state_t *state);
void shared_state_unlock(shared_state_t *state);

// 속도 설정 편의 함수
void shared_state_set_velocity(shared_state_t *state, float lin, float ang);

#endif // SHARED_STATE_H