// shared_state.c
#include "shared_state.h"
#include <string.h>
#include <pthread.h>

void shared_state_init(shared_state_t *state) {
    pthread_mutex_init(&state->mutex, NULL);

    // 기본값 설정
    state->mode = MODE_IDLE;
    state->cmd_mode = CMD_MODE_NONE;

    state->water_level = 0;
    state->water_level_override = false;

    state->lin_vel = 0.0f;
    state->ang_vel = 0.0f; // 회전 속도

    state->dT = 0.0f;

    state->hot_row = -1;
    state->hot_col = -1;

    state->emergency_stop = false;
    state->error_code = 0;
}

void shared_state_destroy(shared_state_t *state) {
    pthread_mutex_destroy(&state->mutex);
}

void shared_state_lock(shared_state_t *state) {
    pthread_mutex_lock(&state->mutex);
}

void shared_state_unlock(shared_state_t *state) {
    pthread_mutex_unlock(&state->mutex);
}