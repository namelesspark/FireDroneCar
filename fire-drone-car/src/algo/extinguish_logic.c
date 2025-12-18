// extinguish_logic.c
// 주의: 이 파일의 함수는 lock이 잡힌 상태에서 호출됨

#include "extinguish_logic.h"
#include <stdio.h>
#include <time.h>

static int current_level = WATER_LEVEL_MIN;
static time_t spray_start = 0;
static bool spraying = false;

void extinguish_init(void) {
    extinguish_reset();
    printf("[extinguish] init COMPLETED\n");
}

void extinguish_reset(void) {
    current_level = WATER_LEVEL_MIN;
    spray_start = 0;
    spraying = false;
}

int run_extinguish_loop(shared_state_t *state) {
    // 소화 성공
    if (state->t_fire < EXTINGUISH_TEMP) {
        state->water_level = 0;
        extinguish_reset();
        return EXTINGUISH_SUCCESS;
    }

    // 분사 시작
    if (!spraying) {
        state->water_level = current_level;
        spray_start = time(NULL);
        spraying = true;
        printf("[EXT] spray level %d\n", current_level);
        return EXTINGUISH_IN_PROGRESS;
    }

    // 분사 중
    if (difftime(time(NULL), spray_start) < SPRAY_DURATION_SEC) {
        return EXTINGUISH_IN_PROGRESS;
    }

    // 강도 올림
    current_level++;
    spraying = false;

    if (current_level > WATER_LEVEL_MAX) {
        extinguish_reset();
        return EXTINGUISH_NEED_CLOSER;
    }

    return EXTINGUISH_IN_PROGRESS;
}
