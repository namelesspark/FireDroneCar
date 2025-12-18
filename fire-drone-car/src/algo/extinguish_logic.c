// extinguish_logic.c
// 주의: 이 파일의 함수는 lock이 잡힌 상태에서 호출됨

#include "extinguish_logic.h"
#include "../embedded/pump_control.h"
#include <stdio.h>
#include <time.h>

static int current_try = 0;
static time_t spray_start = 0;
static bool spraying = false;

void extinguish_init(void) {
    extinguish_reset();
    printf("[extinguish] init COMPLETED\n");
}

void extinguish_reset(void) {
    current_try = 0;
    spray_start = 0;
    spraying = false;
}

int run_extinguish_loop(shared_state_t *state) {
    // 소화 성공
    if (state->t_fire < EXTINGUISH_TEMP) {
        pump_off();
        extinguish_reset();
        return EXTINGUISH_SUCCESS;
    }

    // 분사 시작
    if (!spraying) {
        pump_on();
        spray_start = time(NULL);
        spraying = true;
        current_try++;
        printf("[EXT] spray try %d/%d\n", current_try, EXTINGUISH_TRY);
        return EXTINGUISH_IN_PROGRESS;
    }

    // 분사 중
    if (difftime(time(NULL), spray_start) < SPRAY_DURATION_SEC) {
        return EXTINGUISH_IN_PROGRESS;
    }

    // 분사 완료
    pump_off();
    spraying = false;

    // 시도 횟수 초과 → 더 가까이
    if (current_try >= EXTINGUISH_TRY) {
        printf("[EXT] %d회 시도 실패 → 더 가까이\n", EXTINGUISH_TRY);
        extinguish_reset();
        return EXTINGUISH_NEED_CLOSER;
    }

    return EXTINGUISH_IN_PROGRESS;
}