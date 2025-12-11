# algo 폴더 - 알고리즘 모듈

## 파일 구성

| 파일 | 역할 |
|------|------|
| `shared_state.h/c` | 스레드 간 공유 데이터 관리 (뮤텍스) |
| `state_machine.h/c` | 로봇 상태 전이 (IDLE → SEARCH → DETECT → APPROACH → EXTINGUISH) |
| `navigation.h/c` | 주행 알고리즘 (탐색 회전, 화재 접근) |
| `extinguish_logic.h/c` | 소화 알고리즘 (강도 1~5 단계적 분사) |

---

## 로직 흐름

```
        ┌──────────────────────────────────────┐
        │                                      │
        ▼                                      │
     [IDLE] ──(START 명령)──▶ [SEARCH]         │
                                │              │
                         (온도 > 50°C)         │
                                │              │
                                ▼              │
                            [DETECT]           │
                                │              │
                         (위치 확인됨)          │
                                │              │
                                ▼              │
                           [APPROACH]          │
                                │              │
                      (거리 < 0.5m, 온도 > 70°C)│
                                │              │
                                ▼              │
                          [EXTINGUISH] ────────┘
                                │        (소화 성공)
                                │
                         (최대 강도 실패)
                                │
                                ▼
                      [APPROACH] (더 가까이)


언제든 emergency_stop → [SAFE_STOP]
```

---

## 각 모듈 설명

### 1. shared_state (공유 상태)

모든 스레드가 같이 보는 "게시판" 역할

```c
// 사용 예시
shared_state_lock(state);      // 잠금
float temp = state->t_fire;    // 값 읽기
state->lin_vel = 0.5f;         // 값 쓰기
shared_state_unlock(state);    // 해제
```

**편의 함수도 있음**
```c
float temp = shared_state_get_temperature(state);  // 안전하게 읽기
shared_state_set_velocity(state, 0.5f, 0.0f);      // 안전하게 쓰기
```

---

### 2. state_machine (상태 머신)

`nav_thread`에서 주기적으로 호출해야함

```c
// main.c 또는 nav_thread에서
while (running) {
    state_machine_update(&shared_state);
    usleep(100000);  // 100ms 주기
}
```

**임계값 (state_machine.h에서 수정 가능)**
| 상수 | 값 | 설명 |
|------|-----|------|
| `FIRE_DETECT_THRESHOLD` | 50°C | 화재 감지 온도 |
| `FIRE_APPROACH_THRESHOLD` | 70°C | 소화 시작 온도 |
| `APPROACH_DISTANCE` | 0.5m | 소화 시작 거리 |
| `SAFE_DISTANCE_MIN` | 0.2m | 최소 안전 거리 |

---

### 3. navigation (주행 알고리즘)

**SEARCH 모드:** 제자리 회전하며 열원 탐색
```c
compute_search_motion(state);
// → lin_vel = 0, ang_vel = 0.5 rad/s
```

**APPROACH 모드** 화재 방향으로 전진 + 방향 보정
```c
compute_approach_motion(state);
// → 열화상 픽셀 위치 기반으로 방향 보정
```

**열화상 픽셀 -> 각도 변환**
```
MLX90641 (16x12, 55° FOV)

     0  1  2  3  4  5  6  7 [8] 9 10 11 12 13 14 15
                            ↑
                          중앙

hot_col = 12 → 중앙에서 +4 → 약 +13.8° 오른쪽
hot_col = 3  → 중앙에서 -5 → 약 -17.2° 왼쪽
```

---

### 4. extinguish_logic (소화 알고리즘)

**동작 순서:**
```
강도 1 분사 (3초) → 온도 체크 → 안 꺼짐
강도 2 분사 (3초) → 온도 체크 → 안 꺼짐
등등등
강도 5 분사 (3초) → 온도 체크 → 안 꺼짐
NEED_CLOSER 반환 → APPROACH로 전환
```

**반환값:**
| 값 | 의미 | state_machine 동작 |
|----|------|-------------------|
| `EXTINGUISH_SUCCESS` (0) | 소화 성공 | → SEARCH |
| `EXTINGUISH_IN_PROGRESS` (1) | 진행 중 | → 계속 EXTINGUISH |
| `EXTINGUISH_NEED_CLOSER` (-1) | 더 가까이 | → APPROACH |

---


### common.h 수정 필요

`shared_state_t` 구조체에 다음 추가해줘:

```c
// common.h의 shared_state_t 맨 아래에 추가
pthread_mutex_t mutex;   // 뮤텍스 (스레드 동기화용)
bool ext_cmd;            // 소화 명령 플래그
```

그리고 맨 위에:
```c
#include <pthread.h>
```

---

### 아직 embedded 함수가 구현이 안 돼있어 algo에서 다음 작업이 수행되지 않고 있음~

algo 코드에서 호출하는 함수:

**motor_control.h/c**
```c
void motor_set_velocity(float lin_vel, float ang_vel);
void motor_stop(void);
```

**pump_control.h/c**
```c
void pump_set_pwm(int duty);  // duty: 0~100%
void pump_stop(void);
```

**ultrasonic.h/c**
```c
float ultrasonic_get_distance(void);  // 반환: 미터 단위
```
---
