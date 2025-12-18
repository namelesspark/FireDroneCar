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
}

// ===== SEARCH =====
void handle_search(shared_state_t *state) {
    if (state->t_fire >= FIRE_APPROACH_THRESHOLD) { // 200도 이상
        printf("[SM] SEARCH → DETECT (T=%.1f)\n", state->t_fire);
        navigation_reset_avoid();
        navigation_reset_detect();
        state->mode = MODE_DETECT;
        return;
    }
    compute_search_motion(state);
}

// ===== DETECT: 전진하며 열원 중앙 정렬 =====
// state_machine.c

void handle_detect(shared_state_t *state) {
    static int lock_count = 0;          // 머리 고정(정렬) 연속 유지 카운트
    static int stable_hot_count = 0;    // 핫스팟 좌표 안정 카운트
    static int last_hot_col = -999;

    // 열원 소실
    if (state->hot_col < 0 || state->t_fire < FIRE_LOST_THRESHOLD) {
        lock_count = 0;
        stable_hot_count = 0;
        last_hot_col = -999;
        navigation_reset_detect();
        state->mode = MODE_SEARCH;
        return;
    }

    // 핫스팟 좌표가 흔들리는지(프레임 간 jitter) 체크: ±1칸 이내면 안정으로 간주
    if (abs(state->hot_col - last_hot_col) <= 1) stable_hot_count++;
    else stable_hot_count = 0;
    last_hot_col = state->hot_col;

    // DETECT는 "머리 조준"만
    bool aligned = compute_detect_motion(state);

    // 소화 후보 조건: 200도 이상 + 핫스팟 안정
    const bool fire_ready = (state->t_fire >= FIRE_APPROACH_THRESHOLD);
    const bool hotspot_stable = (stable_hot_count >= 5);  // 5프레임 연속 안정(튜닝)

    if (fire_ready && hotspot_stable && aligned) lock_count++;
    else lock_count = 0;

    // 머리가 "고정" 상태로 충분히 유지되면 소화 시작
    if (lock_count >= 12) {  // 8프레임 연속 고정(튜닝)
        lock_count = 0;
        state->mode = MODE_EXTINGUISH;
        return;
    }

    // DETECT에서는 차체 정지(안전)
    state->lin_vel = 0.0f;
    state->ang_vel = 0.0f;
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
