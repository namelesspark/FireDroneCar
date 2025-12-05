// state_machine.h
#pragma once
#include "../common/common.h"

void state_machine_init(void);
void state_machine_update(shared_state_t *state, pthread_mutex_t *mutex);
