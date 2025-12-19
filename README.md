# algo/ 모듈 README

FireDroneCar 프로젝트의 알고리즘 모듈입니다.  
상태 머신, 내비게이션, 소화 로직을 담당합니다.

---

## 파일 구조

```
algo/
├── shared_state.h / .c    # 공유 상태 관리 (mutex 포함)
├── state_machine.h / .c   # 6-상태 FSM
├── navigation.h / .c      # 이동 제어 (탐색/감지/접근)
└── extinguish_logic.h / .c # 소화 로직
```

---

## 핵심 설계 원칙

### 1. Mutex는 main에서만 관리

```c
// main.c의 algo_thread에서
while(g_running) {
    shared_state_lock(&g_state);      // ← main에서 lock
    state_machine_update(&g_state);   // algo 내부는 lock 안 함
    shared_state_unlock(&g_state);    // ← main에서 unlock
    usleep(50000);
}
```

**이 모듈의 모든 함수는 lock이 잡힌 상태에서 호출됨.**  
따라서 algo 내부에서는 추가 lock/unlock을 하지 않습니다.

### 2. 함수 호출 체인

```
algo_thread (main.c)
    │
    └── state_machine_update()
            ├── handle_idle()
            ├── handle_search()      → compute_search_motion()
            ├── handle_detect()      → compute_detect_motion()
            ├── handle_approach()    → compute_approach_motion()
            ├── handle_extinguish()  → run_extinguish_loop() → pump_on/off()
            └── handle_safe_stop()
```

---

## 상태 머신 (6-State FSM)

```
┌──────────────────────────────────────────────────────────────┐
│                                                              │
│    ┌──────┐   START     ┌────────┐    T>80°C    ┌────────┐  │
│    │ IDLE │ ──────────► │ SEARCH │ ───────────► │ DETECT │  │
│    └──────┘             └────────┘    or dT>20   └────────┘  │
│        ▲                     ▲                       │       │
│        │                     │                  정렬완료     │
│      STOP                열원소실                    │       │
│        │                 (T<50°C)                    ▼       │
│        │                     │               ┌──────────┐   │
│        │                     └────────────── │ APPROACH │   │
│        │                                     └──────────┘   │
│        │                                           │        │
│        │                    소화성공           d<0.5m       │
│        │                    (T<40°C)          & T>100°C     │
│        │                        │                  │        │
│        │              ┌─────────┴──────────────────▼────┐   │
│        │              │         EXTINGUISH              │   │
│        │              │  (최대강도 실패 시 APPROACH로)   │   │
│        │              └─────────────────────────────────┘   │
│        │                                                    │
│    ┌───┴─────┐                                              │
│    │SAFE_STOP│ ◄──────── emergency_stop = true (언제든지)   │
│    └─────────┘                                              │
│                                                              │
└──────────────────────────────────────────────────────────────┘
```

### 상태별 동작

| 상태 | 설명 | lin_vel | ang_vel | 전이 조건 |
|------|------|---------|---------|----------|
| IDLE | 대기 상태 | 0 | 0 | START 명령 → SEARCH |
| SEARCH | 회전하며 열원 탐색 | 0.1 | 0.5 | T>80°C or dT>20°C → DETECT |
| DETECT | 전진하며 열원 중앙 정렬 | 0.08 | 가변 | 정렬 완료 → APPROACH |
| APPROACH | 열원 방향 접근 | 0.15 | 가변 | d<0.5m & T>100°C → EXTINGUISH |
| EXTINGUISH | 정지 후 소화 | 0 | 0 | T<40°C → SEARCH |
| SAFE_STOP | 긴급 정지 | 0 | 0 | emergency_stop 해제 필요 |

---

## 모듈별 상세

### shared_state.h / .c

공유 상태 구조체와 mutex 관리 함수 제공.

```c
typedef struct {
    pthread_mutex_t mutex;
    
    robot_mode_t mode;      // 현재 상태
    cmd_mode_t cmd_mode;    // 외부 명령
    
    int water_level;        // 펌프 강도 (1~5)
    float lin_vel, ang_vel; // 모터 명령
    
    float t_fire;           // 최고 온도 (°C)
    float dT;               // 온도 차이 (°C)
    float distance;         // 초음파 거리 (m)
    int hot_row, hot_col;   // 열원 위치 (픽셀)
    
    bool emergency_stop;    // 긴급 정지 플래그
    // ...
} shared_state_t;
```

**제공 함수:**
- `shared_state_init()` - 초기화
- `shared_state_destroy()` - mutex 해제
- `shared_state_lock()` - mutex 잠금 (main에서만 사용)
- `shared_state_unlock()` - mutex 해제 (main에서만 사용)

### state_machine.h / .c

상태 머신 핵심 로직.

**제공 함수:**
- `state_machine_init()` - 초기화
- `state_machine_update(state)` - 매 주기 호출 (lock 상태에서)

**임계값 설정:**
```c
#define FIRE_DETECT_THRESHOLD   80.0f   // 열원 감지 온도
#define FIRE_APPROACH_THRESHOLD 100.0f  // 소화 시작 온도  
#define FIRE_LOST_THRESHOLD     50.0f   // 열원 소실 판정
#define DELTA_TEMP_THRESHOLD    20.0f   // dT 감지 임계값
#define APPROACH_DISTANCE       0.5f    // 소화 거리 (m)
```

### navigation.h / .c

이동 관련 계산 함수.

**제공 함수:**
- `compute_search_motion(state)` - SEARCH 상태 이동 계산
- `compute_detect_motion(state)` - DETECT 상태 (열원 정렬), 완료 시 true 반환
- `compute_approach_motion(state)` - APPROACH 상태 접근

**주요 상수:**
```c
#define THERMAL_CENTER_COL  8       // 16x12 센서 중앙 열
#define CENTER_TOLERANCE    2       // 중앙 정렬 허용 오차 (픽셀)
#define OBSTACLE_DISTANCE   0.30f   // 장애물 정지 거리 (m)
```

### extinguish_logic.h / .c

소화 로직 (단계별 분사).

**제공 함수:**
- `extinguish_init()` - 초기화
- `extinguish_reset()` - 상태 리셋
- `run_extinguish_loop(state)` - 소화 로직 실행

**반환값:**
```c
#define EXTINGUISH_SUCCESS      0   // 소화 성공 (T<40°C)
#define EXTINGUISH_IN_PROGRESS  1   // 진행 중
#define EXTINGUISH_NEED_CLOSER -1   // 최대 강도로도 실패, 더 가까이 필요
```

**동작 흐름:**
1. 레벨 1로 시작, 3초간 분사
2. 온도 40°C 미만 → 성공
3. 아니면 레벨 증가 (최대 5)
4. 레벨 5에서도 실패 → APPROACH로 복귀

---

## 데이터 흐름

```
┌─────────────┐                  ┌─────────────┐
│sensor_thread│                  │ motor_thread│
│  (main.c)   │                  │  (main.c)   │
└──────┬──────┘                  └──────▲──────┘
       │                                │
       │ 쓰기                           │ 읽기
       │ t_fire, dT                     │ lin_vel
       │ distance                       │ ang_vel
       │ hot_row, hot_col               │ water_level
       │                                │
       ▼                                │
┌──────────────────────────────────────┐
│           shared_state_t              │
│         (mutex로 동기화)               │
└──────────────────────────────────────┘
       │                                ▲
       │ 읽기                           │ 쓰기
       │ 센서 데이터                     │ 모터 명령
       ▼                                │
┌──────────────────────────────────────┐
│           algo_thread                 │
│         state_machine_update()        │
│         navigation 계산               │
│         extinguish 로직               │
└──────────────────────────────────────┘
```

---

## 스레드 간 Lock 규칙

| 스레드 | Lock 위치 | 비고 |
|--------|----------|------|
| sensor_thread | 직접 lock/unlock | 센서 데이터 쓰기 시 |
| algo_thread | main에서 lock/unlock | 내부 함수는 lock 안 함 |
| motor_thread | 직접 lock/unlock | 모터 명령 읽기 시 |
| comm_thread | 직접 lock/unlock | 상태 전송 시 |

---

## 제거된 기능

- `water_level_override`, `ext_cmd` 관련 복잡한 분기 삭제
- 외부에서 water_level 직접 제어 기능 삭제
- 불필요한 중첩 조건문 단순화

---

## 의존성

```
algo/
├── depends on: ../common/common.h (shared_state_t, enum 정의)
└── depends on: ../embedded/pump_control.h (pump_on, pump_off)
```

---

## 빌드 & 테스트

```bash
# 전체 빌드
cd fire-drone-car
make clean && make

# 실행 (라즈베리파이, root 필요)
sudo ./fire_drone_car
```

---

## 주의사항

1. **모든 algo 함수는 lock 상태에서 호출됨** - 내부에서 lock 호출 금지
2. **pump_on/off는 extinguish_logic에서만 호출** - 다른 곳에서 직접 호출 금지
3. **상태 전이 로그 확인** - `[SM]` 태그로 상태 변화 추적 가능
