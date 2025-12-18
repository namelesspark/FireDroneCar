//extinguish_logic.c
#include "extinguish_logic.h"
#include <stdio.h>
#include <time.h>
// 나중에 embedded 헤더 포함 필요

static int current_level = WATER_LEVEL_MIN;
static int retry_count = 0;
static time_t spray_start_time = 0;
static bool is_spraying = false;


// 초기화 및 리셋
void extinguish_init(void) {
    current_level = WATER_LEVEL_MIN;
    retry_count = 0;
    spray_start_time = 0;
    is_spraying = false;

    printf("[extinguish] init COMPLETED\n");
}
void extinguish_reset(void) {
    current_level = WATER_LEVEL_MIN;
    retry_count = 0;
    spray_start_time = 0;
    is_spraying = false;

    printf("[extinguish] reset COMPLETED\n");
}



/* 메인 소화 루프
음...
1. 강도 1부터 시작
2. SPRAY_DURATION_SEC 동안 분사
3. 온도 확인
4. 온도가 안 내려갔으면? 재시도 -> 지금 코드는 일단 재시도 없이 바로 강도 올림
5. 그래도 안 내려갔으면? 강도 올림
6. 최대 강도에서도 실패하면 NEED_CLOSER 반환
*/
int run_extinguish_loop(shared_state_t *state) {
    // 뮤텍스 잠금 후 현재 온도 읽기
    shared_state_lock(state);
    float T = state->t_fire;
    shared_state_unlock(state);

    if (T < EXTINGUISH_TEMP_THRESHOLD) {// 소화 성공 판정
        printf("[extinguish] SUCCESS! %.1f°C < %.1f°C\n",
               T, EXTINGUISH_TEMP_THRESHOLD);

        // 펌프 정지
        set_water_level(state, 0);
        is_spraying = false;
        extinguish_reset();

        return EXTINGUISH_SUCCESS;
    }

    if (!is_spraying) { // 분사 시작
        printf("[extinguish] SPRAY START - 강도: %d/%d\n",
               current_level, WATER_LEVEL_MAX);

        set_water_level(state, current_level);
        spray_start_time = time(NULL);
        is_spraying = true;

        return EXTINGUISH_IN_PROGRESS;
    }

    // 분사 시간 체크
    time_t now = time(NULL);
    double elapsed = difftime(now, spray_start_time);
    if (elapsed < SPRAY_DURATION_SEC) {
        return EXTINGUISH_IN_PROGRESS;
    }

    // 분사 시간 경과 - 강도 증가
    current_level += WATER_LEVEL_STEP;
    is_spraying = false;  // 다음 루프에서 새 강도로 분사 시작

    // 최대 강도 초과 체크
    if (current_level > WATER_LEVEL_MAX) {
        printf("[extinguish] 강도 초과, 더 가까이 접근 필요\n");
        extinguish_reset();  // 강도 리셋
        return EXTINGUISH_NEED_CLOSER;  // APPROACH로 전환
    }

    printf("[extinguish] 강도 증가: %d -> %d\n",
           current_level - WATER_LEVEL_STEP, current_level);

    return EXTINGUISH_IN_PROGRESS;  // 다음 강도로 재시도
}


// 펌프 강도
void set_water_level(shared_state_t *state, int level) {
    // 범위 제한
    if (level < 0) level = 0;
    if (level > WATER_LEVEL_MAX) level = WATER_LEVEL_MAX;

    // 뮤텍스 잠금, shared_state 업데이트
    shared_state_lock(state);
    state->water_level = level;
    shared_state_unlock(state);

    // TODO - embedded 함수 구현 후 실제 펌프 제어 호출해야 함

}


// 현재 소화 상태 조회
int get_current_water_level(void) {
    return current_level;
}

int get_retry_count(void) {
    return retry_count;
}