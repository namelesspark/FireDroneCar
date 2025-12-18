// state_machine.c
// 주의: 이 파일의 모든 함수는 lock이 잡힌 상태에서 호출됨 (algo_thread에서 lock)

#include "state_machine.h"
#include "navigation.h"
#include "extinguish_logic.h"
#include <stdio.h>

void state_machine_init(void) {
    printf("[state_machine] init COMPLETED\n");
}

void state_machine_update(shared_state_t *state) {
    // 긴급정지 최우선
    if (state->emergency_stop) {
        state->mode = MODE_SAFE_STOP;
        state->lin_vel = 0.0f;
        state->ang_vel = 0.0f;
        state->water_level = 0;
        return;
    }

    // STOP 명령
    if (state->cmd_mode == CMD_MODE_STOP) {
        state->mode = MODE_IDLE;
        state->cmd_mode = CMD_MODE_NONE;
    }

    // 상태별 처리
    switch (state->mode) {
        case MODE_IDLE:
            handle_idle(state);
            break;
        case MODE_SEARCH:
            handle_search(state);
            break;
        case MODE_DETECT:
            handle_detect(state);
            break;
        case MODE_APPROACH:
            handle_approach(state);
            break;
        case MODE_EXTINGUISH:
            handle_extinguish(state);
            break;
        case MODE_SAFE_STOP:
            state->lin_vel = 0.0f;
            state->ang_vel = 0.0f;
            state->water_level = 0;
            break;
    }
}

// ===== IDLE =====
void handle_idle(shared_state_t *state) {
    if (state->cmd_mode == CMD_MODE_START) {
        printf("[SM] IDLE → SEARCH\n");
        state->mode = MODE_SEARCH;
        state->cmd_mode = CMD_MODE_NONE;
    }

    state->lin_vel = 0.0f;
    state->ang_vel = 0.0f;
    state->water_level = 0;
}

// ===== SEARCH =====
void handle_search(shared_state_t *state) {
    // 열원 감지 → DETECT
    if (state->t_fire > FIRE_DETECT_THRESHOLD || state->dT > DELTA_TEMP_THRESHOLD) {
        printf("[SM] SEARCH → DETECT (T=%.1f, dT=%.1f)\n", state->t_fire, state->dT);
        state->mode = MODE_DETECT;
        return;
    }

    // 전진하며 탐색
    compute_search_motion(state);
}

// ===== DETECT: 전진하며 열원 중앙 정렬 =====
void handle_detect(shared_state_t *state) {
    // 열원 소실 → SEARCH
    if (state->t_fire < FIRE_LOST_THRESHOLD) {
        printf("[SM] DETECT → SEARCH (열원 소실)\n");
        state->mode = MODE_SEARCH;
        return;
    }

    // 전진하며 정렬
    bool aligned = compute_detect_motion(state);

    if (aligned) {
        printf("[SM] DETECT → APPROACH (정렬 완료)\n");
        state->mode = MODE_APPROACH;
    }
}

// ===== APPROACH =====
void handle_approach(shared_state_t *state) {
    // 열원 소실 → SEARCH
    if (state->t_fire < FIRE_LOST_THRESHOLD) {
        printf("[SM] APPROACH → SEARCH (열원 소실)\n");
        state->mode = MODE_SEARCH;
        return;
    }

    // 소화 거리 도달 → EXTINGUISH
    if (state->distance < APPROACH_DISTANCE && state->t_fire > FIRE_APPROACH_THRESHOLD) {
        printf("[SM] APPROACH → EXTINGUISH (d=%.2f, T=%.1f)\n", 
               state->distance, state->t_fire);
        state->mode = MODE_EXTINGUISH;
        return;
    }

    // 접근
    compute_approach_motion(state);
}

// ===== EXTINGUISH =====
void handle_extinguish(shared_state_t *state) {
    // 소화 중 정지
    state->lin_vel = 0.0f;
    state->ang_vel = 0.0f;

    int result = run_extinguish_loop(state);

    if (result == EXTINGUISH_SUCCESS) {
        printf("[SM] EXTINGUISH → SEARCH (성공)\n");
        state->mode = MODE_SEARCH;
    } else if (result == EXTINGUISH_NEED_CLOSER) {
        printf("[SM] EXTINGUISH → APPROACH (더 가까이)\n");
        state->mode = MODE_APPROACH;
    }
}
