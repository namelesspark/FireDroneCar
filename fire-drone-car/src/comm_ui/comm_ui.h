// src/comm_ui/comm_ui.h
#pragma once

#include <pthread.h>
#include "common.h"

// comm 스레드에서 사용할 컨텍스트 구조체
typedef struct
{
    shared_state_t *state;
    pthread_mutex_t *state_mutex;
} comm_ctx_t;

// 통신 스레드 엔트리 함수 (pthread_create 에서 사용)
void *comm_thread(void *arg);
