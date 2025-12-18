// extinguish_logic.h
#ifndef EXTINGUISH_LOGIC_H
#define EXTINGUISH_LOGIC_H

#include "../common/common.h"
#include "shared_state.h"

#define SPRAY_DURATION_SEC  3.0f
#define EXTINGUISH_TEMP     40.0f
#define EXTINGUISH_TRY      5

#define EXTINGUISH_SUCCESS      0
#define EXTINGUISH_IN_PROGRESS  1
#define EXTINGUISH_NEED_CLOSER  -1

void extinguish_init(void);
void extinguish_reset(void);

// 주의: lock이 잡힌 상태에서 호출됨
int run_extinguish_loop(shared_state_t *state);

#endif
