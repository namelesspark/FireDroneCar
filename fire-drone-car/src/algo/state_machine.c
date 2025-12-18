// state_machine.c

#include "state_machine.h"
#include "navigation.h"
#include "extinguish_logic.h"
#include <stdio.h>
#include <math.h>


void state_machine_init(void) {
    printf("state machine initialized.\n");
}

void state_machine_update(shared_state_t *state) {
    shared_state_lock(state);
    robot_mode_t current_mode = state->mode;
    bool emergency = state->emergency_stop;
    cmd_mode_t cmd = state->cmd_mode;
    shared_state_unlock(state);

    // 긴급정지 최우선
    if (emergency) {
        shared_state_lock(state);
        state->mode = MODE_SAFE_STOP;
        shared_state_unlock(state);
        state_safe_stop_handler(state);
        return;
    }
    
    // 외부 명령 처리
    if (cmd == CMD_MODE_STOP) {
        shared_state_lock(state);
        state->mode = MODE_IDLE;
        shared_state_unlock(state);
        state_idle_handler(state);
        return;
    }

    // 상태별 핸들러 호출
    switch (current_mode) {
        case MODE_IDLE:
            state_idle_handler(state);
            break;
            
        case MODE_SEARCH:
            state_search_handler(state);
            break;
            
        case MODE_DETECT:
            state_detect_handler(state);
            break;
            
        case MODE_APPROACH:
            state_approach_handler(state);
            break;
            
        case MODE_EXTINGUISH:
            state_extinguish_handler(state);
            break;
            
        case MODE_SAFE_STOP:
            state_safe_stop_handler(state);
            break;
            
        default:
            printf("[state_machine] 오류: 정의되지 않은 모드 %d\n", current_mode);
            break;
    }
}


void state_idle_handler(shared_state_t *state) {
    // IDLE: 대기 상태
    shared_state_lock(state);
    cmd_mode_t cmd = state->cmd_mode;
    shared_state_unlock(state);
    
    // START 명령 대기
    if (cmd == CMD_MODE_START) {
        printf("[state_machine] START 명령 수신 → SEARCH 모드로 전환\n");
        shared_state_lock(state);
        state->mode = MODE_SEARCH;
        state->cmd_mode = CMD_MODE_NONE;  // 명령 소비
        shared_state_unlock(state);
    }
    
    // 모든 출력 0
    shared_state_lock(state);
    state->lin_vel = 0.0f;
    state->ang_vel = 0.0f;
    state->water_level = 0;
    state->ext_cmd = false;
    shared_state_unlock(state);
}

void state_search_handler(shared_state_t *state) {
    // SEARCH: 제자리 회전하는 로직
    shared_state_lock(state);
    float T = state->t_fire;
    float dT = state->dT;
    shared_state_unlock(state);
    
    // 화재 감지 확인
    if (T > FIRE_DETECT_THRESHOLD || dT > 20.0f) {
        printf("[state_machine] 화재 감지! (T=%.1f, dT=%.1f) → DETECT 모드\n", T, dT);
        shared_state_lock(state);
        state->mode = MODE_DETECT;
        shared_state_unlock(state);
        return;
    }
    
    // 회전하며 탐색 (navigation.c에 구현)
    compute_search_motion(state);
    
    printf("[state_machine] SEARCH: 화재 탐색 중... (T=%.1f)\n", T);
}

void state_detect_handler(shared_state_t *state) {
    // DETECT: 화재 방향으로 중앙 정렬
    shared_state_lock(state);
    float T = state->t_fire;
    int hot_col = state->hot_col;
    shared_state_unlock(state);
    
    // 화재 방향 확인
    if (hot_row >= 0 && hot_col >= 0) {
        printf("[state_machine] 화재 위치 확인 (%d, %d) → APPROACH 모드\n", 
               hot_row, hot_col);
        shared_state_lock(state);
        state->mode = MODE_APPROACH;
        shared_state_unlock(state);
    } else {
        // 화재 위치 못 찾으면 다시 SEARCH
        printf("[state_machine] 화재 위치 불명확 → SEARCH로 복귀\n");
        shared_state_lock(state);
        state->mode = MODE_SEARCH;
        shared_state_unlock(state);
    }
}

void state_approach_handler(shared_state_t *state) {
    // APPROACH: 화재에 접근
    shared_state_lock(state);
    float T = state->t_fire;
    float d = state->distance;
    int hot_row = state->hot_row;
    int hot_col = state->hot_col;
    shared_state_unlock(state);
    
    // 화재 소실 확인
    if (T < FIRE_DETECT_THRESHOLD) {
        printf("[state_machine] 화재 소실 → SEARCH로 복귀\n");
        shared_state_lock(state);
        state->mode = MODE_SEARCH;
        shared_state_unlock(state);
        return;
    }
    
    // 소화 거리 도달 확인
    if (d < APPROACH_DISTANCE && T > FIRE_APPROACH_THRESHOLD) {
        printf("[state_machine] 소화 거리 도달 (d=%.2fm, T=%.1f) → EXTINGUISH\n", d, T);
        shared_state_lock(state);
        state->mode = MODE_EXTINGUISH;
        shared_state_unlock(state);
        return;
    }
    
    // 화재 방향으로 접근 (navigation.c에 구현)
    compute_approach_motion(state);
    
    printf("[state_machine] APPROACH: 접근 중 (d=%.2fm, T=%.1f)\n", d, T);
}

void state_extinguish_handler(shared_state_t *state) {
    // EXTINGUISH: 소화 알고리즘 실행
    // extinguish_logic.c에 구현된 함수 호출
    int result = run_extinguish_loop(state);
    
    if (result == 0) {
        // 소화 성공
        printf("[state_machine] 소화 성공! → SEARCH로 복귀\n");
        shared_state_lock(state);
        state->mode = MODE_SEARCH;
        shared_state_unlock(state);
    } else if (result == -1) {
        // 소화 실패 (거리 조정 필요)
        printf("[state_machine] 소화 실패 → APPROACH로 재시도\n");
        shared_state_lock(state);
        state->mode = MODE_APPROACH;
        shared_state_unlock(state);
    }
    // result == 1: 진행 중 (계속 EXTINGUISH 유지)
}

void state_safe_stop_handler(shared_state_t *state) {
    // SAFE_STOP: 비상 정지
    shared_state_lock(state);
    state->lin_vel = 0.0f;
    state->ang_vel = 0.0f;
    state->water_level = 0;
    state->ext_cmd = false;
    shared_state_unlock(state);
    
    printf("[state_machine] SAFE_STOP: 비상 정지 중\n");
}