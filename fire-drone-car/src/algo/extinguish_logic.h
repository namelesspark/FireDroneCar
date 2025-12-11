#ifndef EXTINGUISH_LOGIC_H
#define EXTINGUISH_LOGIC_H

#include "../common/common.h"
#include "shared_state.h"

// 물펌프 강도 단계 (1~5)
#define WATER_LEVEL_MIN 1
#define WATER_LEVEL_MAX 5
#define WATER_LEVEL_STEP 1

// 각 강도에서 분사 시간
#define SPRAY_DURATION_SEC 3.0f

// 소화 성공 판정 온도
#define EXTINGUISH_TEMP_THRESHOLD 40.0f

// 최대 재시도 횟수 (같은 강도에서)
#define MAX_RETRY_PER_LEVEL 2

//소화 결과 코드
#define EXTINGUISH_SUCCESS 0
#define EXTINGUISH_FAILED -2
#define EXTINGUISH_IN_PROGRESS 1
#define EXTINGUISH_NEED_CLOSER -1

// 함수
// 초기화 및 리셋
void extinguish_init(void);
void extinguish_reset(void);


// state_machine에서 호출, 메인 소화 루프
int run_extinguish_loop(shared_state_t *state);

 // 펌프 강도
void set_water_level(shared_state_t *state, int level);

 // 현재 소화 상태 조회
int get_current_water_level(void);
int get_retry_count(void);

#endif // EXTINGUISH_LOGIC_H
