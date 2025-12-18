<<<<<<< HEAD
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include "common.h"

// comm.c 안의 스레드 함수
extern void *comm_thread(void *arg);

typedef struct
{
    shared_state_t *state;
    pthread_mutex_t *state_mutex;
} comm_ctx_t;

int main(void)
{
    shared_state_t state = {0};
    pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;

    state.mode = MODE_IDLE;
    state.cmd_mode = CMD_MODE_NONE;
    state.water_level = 0;
    state.t_fire = 25.0f;
    state.dT = 0.0f;
    state.distance = 1.0f;
    state.hot_row = 0;
    state.hot_col = 0;
    state.emergency_stop = false;

    comm_ctx_t ctx = {.state = &state, .state_mutex = &mtx};
    pthread_t tid_comm;
    if (pthread_create(&tid_comm, NULL, comm_thread, &ctx) != 0)
    {
        perror("pthread_create(comm)");
        return 1;
    }

    printf("comm_test running. Connect with: nc <PI_IP> 5000\n");

    int counter = 0;
    while (1)
    {
        pthread_mutex_lock(&mtx);

        state.mode = (counter % 2 == 0) ? MODE_SEARCH : MODE_EXTINGUISH;
        state.water_level = counter % 4;
        state.t_fire = 30.0f + counter;
        state.dT = 10.0f + 0.5f * counter;
        state.distance = 1.0f - 0.02f * counter;
        state.hot_row = counter % 16;
        state.hot_col = counter % 12;

        pthread_mutex_unlock(&mtx);

        counter++;
        sleep(1);
    }

    pthread_join(tid_comm, NULL);
    return 0;
}
=======
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include "common.h"

// comm.c 안의 스레드 함수
extern void *comm_thread(void *arg);

typedef struct
{
    shared_state_t *state;
    pthread_mutex_t *state_mutex;
} comm_ctx_t;

int main(void)
{
    shared_state_t state = {0};
    pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;

    state.mode = MODE_IDLE;
    state.cmd_mode = CMD_MODE_NONE;
    state.water_level = 0;
    state.t_fire = 25.0f;
    state.dT = 0.0f;
    state.distance = 1.0f;
    state.hot_row = 0;
    state.hot_col = 0;
    state.emergency_stop = false;

    comm_ctx_t ctx = {.state = &state, .state_mutex = &mtx};
    pthread_t tid_comm;
    if (pthread_create(&tid_comm, NULL, comm_thread, &ctx) != 0)
    {
        perror("pthread_create(comm)");
        return 1;
    }

    printf("comm_test running. Connect with: nc <PI_IP> 5000\n");

    int counter = 0;
    while (1)
    {
        pthread_mutex_lock(&mtx);

        state.mode = (counter % 2 == 0) ? MODE_SEARCH : MODE_EXTINGUISH;
        state.water_level = counter % 4;
        state.t_fire = 30.0f + counter;
        state.dT = 10.0f + 0.5f * counter;
        state.distance = 1.0f - 0.02f * counter;
        state.hot_row = counter % 16;
        state.hot_col = counter % 12;

        pthread_mutex_unlock(&mtx);

        counter++;
        sleep(1);
    }

    pthread_join(tid_comm, NULL);
    return 0;
}
>>>>>>> feature/algo-sm
