# FireDroneCar

**화재 탐지 및 진압 자율주행 로봇**

열화상 카메라와 초음파 센서를 활용하여 화재를 자동으로 탐지하고 접근하여 소화하는 임베디드 시스템 프로젝트

---

## 1. 프로젝트 개요

### 프로젝트 이름
**FireDroneCar** - 화재 탐지 및 진압 자율주행 로봇

### 한 줄 요약
붕괴 건물과 같은 좁은 틈으로 진입해 열화상 센서와 초음파 센서로 불의 위치를 탐지, 자율 주행 기반 초기 진압을 수행하는 초소형 소화 로봇 시스템

### 주요 기능
- 🔥 **실시간 화재 탐지**
  - MLX90641 열화상 센서(16x12)를 활용한 온도 기반 화재 감지
    - 센서 사양: I2C 통신, 4Hz refresh rate
    - 측정 범위: -40°C ~ 300°C
    - 감지 로직: 프레임 내 최고 온도(T_fire)와 평균 온도(T_env)를 계산하여 온도 차이(dT) 산출
    - 열원 위치 추적: 가장 뜨거운 픽셀의 행/열 좌표(hot_row, hot_col)를 실시간 반환
    - 감지 임계값: T_fire >= 180°C일 때 화재로 판정, T_fire < 35°C일 때 열원 소실로 판정

   

- 🤖 **자율 주행**
   - 6-상태 FSM 기반 자율 네비게이션 및 열원 추적

| 상태 | 설명 | 주요 동작 |
|---------|----------|------|
| **IDLE** | 대기 상태 | 모든 구동 정지, START 명령 대기 |
| **SEARCH** | 탐색 상태 | 전진(0.20 속도)하며 목을 좌우로 스캔, 열원 탐색 |
| **DETECT** | 정렬 상 | 차체 정지, 목 서보로 열원을 화면 중앙에 정렬 |
| **APPROACH** | - | 열원 방향으로 전진(0.15 속도), 조향 보정 |
| **EXTINGUISH** | - | 정지 후 워터펌프로 3초간 분사, 최대 5회 시도 |
| **SAFE_STOP** | - | 모든 구동 즉시 중단, CLEAR_ESTOP 명령으로 복귀 |



- 🚧 **장애물 회피**
   - 초음파 센서(HC-SR04)를 활용한 실시간 거리 측정 및 장애물 감지
      - 센서 연결: GPIO23(TRIG), GPIO24(ECHO)
      - 측정 방식: 3회 측정 평균값 사용, 타임아웃 30ms
      - 감지 거리: 30cm 이내 장애물 감지 시 회피 동작 시작
      - 회피 시퀀스: 정지(BRAKE) → 후진+회전(REVERSE_ARC) → 전진+회전(FORWARD_ARC) → 직진 복귀(RECOVER)
      - 안전 거리: 12cm 미만은 충돌 위험으로 즉시 회피, 50cm 이상이면 회피 종료
    

- 🧯 **단계별 소화**
   - 5단계 펌프 제어 시스템을 통한 효율적 진압
      - 분사 시간: 1회당 3초간 연속 분사
      - 최대 시도: 5회까지 반복 시도
      - 성공 조건: 분사 후 T_fire < 40°C로 하락하면 소화 성공
      - 실패 처리: 5회 시도 후에도 실패 시 APPROACH 상태로 전환하여 더 가까이 접근
      - 펌프 제어: PCA9685 채널 4를 통해 릴레이 모듈로 ON/OFF 제어
    

- 📡 **원격 모니터링**
   - TCP/IP 기반 실시간 상태 전송 및 원격 제어
      - 서버 포트: 5000번
      - 상태 전송 주기: 200ms마다 현재 상태 전송
      - 상태 메시지 포맷: MODE=SEARCH;T_FIRE=85.3;DT=22.1;D=1.25;HOT=4,7;ESTOP=0\n
      - 지원 명령어: START, STOP, MANUAL, ESTOP, CLEAR_ESTOP, EXIT
      - 클라이언트 UI: Python curses 기반 터미널 UI (ui_remote.py)
    

- 🛡️ **긴급 정지**
   - E-STOP 기능을 통한 안전성 확보
      - 활성화 방법: 원격 UI에서 ESTOP 명령 전송 또는 emergency_stop 플래그 설정
      - 동작: 모든 모터 즉시 정지, 조향 중앙(90°)으로 복귀, 펌프 OFF
      - 해제 방법: CLEAR_ESTOP 명령 전송 시 IDLE 상태로 복귀
      - 우선순위: 모든 상태보다 최우선 처리 (algo_thread에서 가장 먼저 확인)
    

- ⚡ **멀티스레드 구조**
   - 4개의 독립 스레드가 병렬로 동작하여 실시간 제어를 수행

| 스레드 | 주기 | 역할 | 주요 함수 |
|--------|------|------|----------|
| **sensor_thread** | 100ms | 열화상/초음파 센서 읽기 | `thermal_sensor_read()`, `ultrasonic_read_distance_cm_avg()` |
| **algo_thread** | 50ms | FSM 상태 전이, 모터 명령 계산 | `state_machine_update()` |
| **motor_thread** | 20ms | DC모터/서보/펌프 하드웨어 제어 | `motor_set_speed()`, `servo_set_angle()` |
| **comm_thread** | 200ms | TCP 통신, 명령 수신/상태 전송 | `comm_thread()` |

**동기화 방식**: 단일 `pthread_mutex_t`로 `shared_state_t` 구조체 보호, Lock 최소 범위 원칙 적용
  

---

## 2. 시스템 구성

### 전체 시스템 구조

**[전체 시스템 블록 다이어그램 이미지]**
<img width="4400" height="2970" alt="시스템구성도" src="https://github.com/user-attachments/assets/103fcfef-1b18-49ab-ad5c-6ace68744995" />


### 하드웨어 구성
- 본 로봇은 Raspberry Pi 5를 메인 제어기기이며, Adeept Robot HAT V3.1을 통해 전력 분배 및 액추에이터 제어를 통합 관리하는 구조로 설계되었음

**가. 제어 보드 및 전원 시스템**
- **메인 컨트롤러**: Raspberry Pi 5 (데이터 처리 및 상위 제어 알고리즘 실행)
- **확장 보드**: Adeept Robot HAT V3.1 (PCA9685 PWM Driver 내장, I2C 주소: 0x5F)
- **전원부**: 18650 리튬이온 배터리 x2 (7.4V, 4A)
- 배터리 전원은 Adeept HAT의 Vin 단자로 입력되며, 40-Pin Header를 통해 라즈베리 파이에 5V 전원을 공급함

**나. 구동부 및 액추에이터**
- PCA9685 PWM Driver (I2C 주소: `0x5F`를 통해 총 5개의 액추에이터 제어
- 주행 시스템:DC Motor (후륜 구동): Motor M1 채널에 연결되어 로봇의 이동을 담당
- Servo Motor (조향): Servo Ch2 채널을 사용하여 전륜 조향을 수행
- 상부 가동부 (Neck System):
  - Servo Motor (목 상하): Servo Ch0를 통해 센서 및 소화 노즐의 틸트(Tilt) 각도를 조절
  - Servo Motor (목 좌우): Servo Ch1을 통해 센서 및 소화 노즐의 팬(Pan) 각도를 조절
  - Water Pump: Servo Ch4 채널에 연결되어 화재 탐지 시 소화액을 분사
 
| 구성요소 | 모델/사양 | 수량 | 용도 |
|---------|----------|------|------|
| **메인보드** | Raspberry Pi 5 | 1 | 전체 시스템 제어 |
| **열화상 센서** | MLX90641 (16x12 픽셀) | 1 | 화재 감지 및 위치 추정 |
| **초음파 센서** | HC-SR04 | 1 | 거리 측정 및 장애물 감지 |
| **모터 드라이버** | Adeept Robot HAT (PCA9685) | 1 | PWM 신호 생성 및 모터 제어 |
| **DC 모터** | - | 1 | 후륜 구동 (전진/후진) |
| **서보 모터** | - | 3 | 조향(1), 목 팬/틸트(2) |
| **워터 펌프** | - | 1 | 소화액 분사 |

 
**다. 인터페이스 및 통신 구조**
- I2C 통신: Raspberry Pi 5의 /dev/i2c-1 버스를 공유하며, PWM 드라이버($0x5F$)와 열화상 센서($0x33$)가 각각 독립적인 주소로 통신
- GPIO 제어: 초음파 센서는 HAT을 거치지 않고 Raspberry Pi의 40-Pin GPIO에 직접 연결되어 실시간성을 확보


**[하드웨어 연결 사진]**
- 하드웨어 다이어그램
<img width="7631" height="5455" alt="하드웨어" src="https://github.com/user-attachments/assets/b50f31b9-4b22-4dca-b49d-a131390010ba" />



### 소프트웨어 구성

#### 프로세스/스레드 구조

```
fire_drone_car (메인 프로세스)
├── sensor_thread       [센서 데이터 수집]
│   ├── MLX90641 읽기 (I2C)
│   └── HC-SR04 읽기 (GPIO)
│
├── algo_thread         [알고리즘 실행]
│   ├── 상태 머신 업데이트
│   ├── 네비게이션 계산
│   └── 소화 로직 실행
│
├── motor_thread        [구동기 제어]
│   ├── DC 모터 제어 (PWM)
│   ├── 서보 제어 (PWM)
│   └── 펌프 제어 (ON/OFF)
│
└── comm_thread         [통신 및 UI]
    ├── TCP 서버 (포트 5000)
    ├── 상태 전송 (200ms)
    └── 명령 수신 (START/STOP/ESTOP)
```

**[스레드 타이밍 다이어그램 이미지]**

---

- 알고리즘 스레드 타이밍 시퀀스 다이어그램
<img width="2808" height="2968" alt="알고리즘 스레드 타이밍 시퀀스 다이어그램" src="https://github.com/user-attachments/assets/cbae3730-6703-489f-8478-44fe6d3aee17" />

---

- 센서 스레드 타이밍 시퀀스 다이어그램
<img width="4917" height="3024" alt="센서 스레드 시퀀스" src="https://github.com/user-attachments/assets/94581bf1-2465-457f-9a46-ba77e2084cc0" />

---

- 라즈베리파이 통신 시퀀스 다이어그램
<img width="2973" height="2980" alt="라즈베리 파이 통신 시퀀스 다이어그램" src="https://github.com/user-attachments/assets/0b741217-75f9-4dd2-a194-48d9b4151a3a" />

---

- 전체 스레드 구성
<img width="3286" height="2617" alt="그림6" src="https://github.com/user-attachments/assets/32dee3b6-2169-4f37-b76f-778e71538f86" />


---


#### 파일 구조

```
fire-drone-car/
├── src/
│   ├── main/
│   │   └── main.c              # 메인 엔트리, 4개 스레드 생성/관리
│   ├── algo/                   # 알고리즘 모듈
│   │   ├── shared_state.c/h    # 공유 상태 및 mutex 관리
│   │   ├── state_machine.c/h   # 6-상태 FSM
│   │   ├── navigation.c/h      # 이동 제어 (탐색/정렬/접근)
│   │   └── extinguish_logic.c/h # 소화 로직 (5단계)
│   ├── embedded/               # 임베디드 제어
│   │   ├── thermal_sensor.c/h  # MLX90641 드라이버
│   │   ├── ultrasonic.c/h      # HC-SR04 드라이버
│   │   ├── motor_control.c/h   # DC 모터 제어
│   │   ├── servo_control.c/h   # 서보 제어 (목/조향)
│   │   ├── pump_control.c/h    # 펌프 제어
│   │   └── mlx90641/           # MLX90641 API (Melexis 제공)
│   ├── comm_ui/                # 통신 및 UI
│   │   ├── comm.c/h            # TCP 서버 스레드
│   │   └── ui_remote.py        # PC용 원격 UI (Python curses)
│   └── common/                 # 공통 정의
│       └── common.h            # shared_state_t, enum 정의
├── tests/                      # 단위 테스트
│   ├── test_thermal.c          # 열화상 센서 테스트
│   ├── test_ultrasonic.c       # 초음파 센서 테스트
│   ├── test_pump.c             # 펌프 테스트
│   ├── test_motor_angle.c      # DC 모터 테스트
│   ├── test_steer.c            # 조향 서보 테스트
│   ├── test_neck.c             # 목 서보 테스트
│   └── comm_test_main.c        # 통신 테스트
├── scripts/                    # 빌드/실행 스크립트
└── Makefile                    # 빌드 설정
```





### 사용한 개발 환경

| 항목 | 내용 |
|------|------|
| **OS** | Raspberry Pi OS (Debian 11 Bullseye) |
| **언어** | C (C99 표준), Python 3.9 |
| **컴파일러** | GCC 12.2.0 |
| **빌드 도구** | GNU Make |
| **주요 라이브러리** | WiringPi (GPIO/I2C), pthread (멀티스레드) |
| **통신** | POSIX Socket API (TCP/IP) |
| **I2C 버스** | `/dev/i2c-1` (Linux i2c-dev 인터페이스) |
| **개발 도구** | VSCode, Git, ssh (원격 개발) |

---

## 3. 제한조건 구현 내용 정리

### 멀티스레드 구조

본 프로젝트는 **4개의 독립적인 스레드**로 구성되어 있으며, 각 스레드는 특정 역할을 담당

- 알고리즘 스레드
<img width="478" height="300" alt="image" src="https://github.com/user-attachments/assets/5b1cb876-fcf0-4365-9ab6-937945566351" />

- 모터 스레드
<img width="617" height="557" alt="image" src="https://github.com/user-attachments/assets/07ec6c55-211a-423c-bbed-ee6268acc52b" />

- 센서 스레드
<img width="546" height="782" alt="image" src="https://github.com/user-attachments/assets/157b4118-5a66-44ee-ac5e-a6643d702d40" />

- main.c - 스레드 생성
<img width="486" height="376" alt="image" src="https://github.com/user-attachments/assets/a14f96bb-cdf6-4b99-be3e-18455fe6dab5" />

#### 스레드별 역할

| 스레드 | 실행 주기 | 주요 역할 | 접근 데이터 |
|--------|----------|----------|------------|
| **sensor_thread** | ~100ms | 센서 데이터 읽기 및 공유 상태 업데이트 | 쓰기: t_fire, dT, distance, hot_row, hot_col |
| **algo_thread** | 50ms | 상태 머신 실행 및 모터 명령 계산 | 읽기: 센서 데이터<br>쓰기: mode, lin_vel, ang_vel |
| **motor_thread** | 20ms | 모터/서보/펌프 하드웨어 제어 | 읽기: lin_vel, ang_vel|
| **comm_thread** | 이벤트 기반 | TCP 통신 (상태 전송 200ms, 명령 수신) | 읽기: 전체 상태<br>쓰기: cmd_mode, emergency_stop |


### Mutex를 통한 스레드 동기화
모든 스레드는 `shared_state_t` 구조체에 포함된 **pthread_mutex_t**를 통해 동기화

#### shared_state_t 구조체

```c
// src/common/common.h
typedef struct {
    pthread_mutex_t mutex;      // 동기화용 뮤텍스
    
    // 상태 정보
    robot_mode_t mode;          // IDLE/SEARCH/DETECT/APPROACH/EXTINGUISH/SAFE_STOP
    cmd_mode_t cmd_mode;        // 외부 명령 (START/STOP/MANUAL)
    
    // 모터 명령
    float lin_vel;              // 선속도 [-1.0, +1.0]
    float ang_vel;              // 각속도 (조향 각도)
    
    // 센서 데이터
    float t_fire;               // 최고 온도 (°C)
    float dT;                   // 온도 차이 (°C)
    float distance;             // 초음파 거리 (m)
    int hot_row, hot_col;       // 열원 위치 (픽셀 좌표)
    
    // 안전 관련
    bool emergency_stop;        // 긴급 정지 플래그
    bool need_closer;           // 접근 필요 플래그
    int error_code;             // 에러 코드
} shared_state_t;
```

#### Mutex 사용 패턴

**1) sensor_thread - 센서 데이터 쓰기**

```c
void* sensor_thread_func(void* arg) {
    while(g_running) {
        // 열화상 카메라 읽기, 공유 상태 업데이트
        if(thermal_sensor_read(&thermal) == 0) {
            shared_state_lock(&g_state);
            g_state.t_fire = thermal.T_fire;
            g_state.dT = thermal.T_fire - thermal.T_env;
            g_state.hot_row = thermal.hot_row;
            g_state.hot_col = thermal.hot_col;
            shared_state_unlock(&g_state);
        }
        // 초음파 센서 읽기
        float dist_cm = ultrasonic_read_distance_cm_avg(&us, 3, 30000);
        if (dist_cm > 0) {
            shared_state_lock(&g_state);
            g_state.distance = dist_cm / 100;  // cm → m 변환
            shared_state_unlock(&g_state);
        }
        usleep(100000);  // 100ms
    }
}
```

**2) algo_thread - 알고리즘 실행**

```c
void* algo_thread_func(void* arg) {
    while(g_running) {
        shared_state_lock(&g_state);     // 여기서 lock
        state_machine_update(&g_state);  // 내부에서 lock 안 함
        shared_state_unlock(&g_state);   // 여기서 unlock
        usleep(50000);
    }
}
```

**3) motor_thread - 하드웨어 제어**

```c
void* motor_thread_func(void* arg) {
    while(g_running) {
        // 모터 명령 읽기 (mutex 안에서)
        shared_state_lock(&g_state);
        float lin = g_state.lin_vel;
        float ang = g_state.ang_vel;
        bool estop = g_state.emergency_stop;
        shared_state_unlock(&g_state);
        
        // 하드웨어 제어 (mutex 밖에서)
        if(estop) {
            motor_stop();
            servo_set_angle(SERVO_STEER, 90.0f);
        } else {
            motor_set_speed(lin);
            float steering_angle = 90.0f + (ang * 45.0f);
            steering_angle = fmaxf(45.0f, fminf(135.0f, steering_angle));
            servo_set_angle(SERVO_STEER, steering_angle);
        }
        usleep(20000);  // 20ms (50Hz)
    }
}
```

**4) comm_thread - 통신**

```c
void* comm_thread(void* arg) {
    comm_ctx_t *ctx = (comm_ctx_t *)arg;
    int server_fd = comm_setup_server_socket(5000);
    
    while(running) {
        // select로 클라이언트 접속/데이터 대기
        // 상태 전송 (200ms 주기)
        if (now - last_status_ms >= 200) {
            pthread_mutex_lock(ctx->state_mutex);
            // 필드별로 스냅샷 복사 (구조체 전체 복사 금지)
            snapshot.mode = ctx->state->mode;
            snapshot.t_fire = ctx->state->t_fire;
            // ...
            pthread_mutex_unlock(ctx->state_mutex);
            
            comm_format_status_line(line, sizeof(line), &snapshot);
            send(client_fd, line, strlen(line), 0);
        }
        
        // 명령 수신 처리
        if (recv_data) {
            comm_handle_command_line(line, ctx->state, ctx->state_mutex);
        }
    }
}
```

### Mutex 사용의 핵심 원칙

1. **Lock은 최소 범위로**: I/O 작업(센서 읽기, 하드웨어 제어)은 mutex 밖에서 수행
2. **Deadlock 방지**: algo_thread는 main에서 lock/unlock 관리, 내부 함수는 lock 호출 안 함
3. **구조체 복사 금지**: `shared_state_t`에 mutex가 포함되어 있어 전체 복사 시 UB 발생 → 스냅샷 방식 사용

**[Mutex Lock 타이밍 차트 이미지]**
<img width="9560" height="5060" alt="Mermaid Chart - Create complex, visual diagrams with text -2025-12-20-191022" src="https://github.com/user-attachments/assets/29c844b3-5cba-483a-aaf5-b529621b9c03" />

### 6-상태 FSM (Finite State Machine)

알고리즘의 핵심은 **6개 상태로 구성된 유한 상태 머신**

```
┌──────┐   START    ┌────────┐   T>80°C   ┌────────┐
│ IDLE │ ────────► │ SEARCH │ ─────────► │ DETECT │
└──────┘           └────────┘   or dT>20  └────────┘
   ▲                    ▲                      │
   │                    │                 정렬완료
 STOP               열원소실                   │
   │                (T<50°C)                   ▼
   │                    │              ┌──────────┐
   │                    └────────────  │ APPROACH │
   │                                   └──────────┘
   │                                        │
   │               소화성공              d<0.5m
   │               (T<40°C)            & T>100°C
   │                   │                   │
   │         ┌─────────┴───────────────────▼────┐
   │         │         EXTINGUISH                │
   │         │  (최대강도 실패 시 APPROACH로)     │
   │         └───────────────────────────────────┘
   │
┌──┴──────┐
│SAFE_STOP│ ◄──── emergency_stop = true (언제든지)
└─────────┘
```

#### 상태별 동작

| 상태 | 동작 | 전이 조건 |
|------|------|----------|
| **IDLE** | 정지 대기 | START 명령 → SEARCH |
| **SEARCH** | 회전하며 열원 탐색<br>(lin_vel=0.1, ang_vel=0.5) | T>80°C or dT>20°C → DETECT<br>열원 소실 시 계속 SEARCH |
| **DETECT** | 열원 중앙 정렬<br>(전진하며 좌우 미세 조정) | 정렬 완료 → APPROACH<br>열원 소실(T<50°C) → SEARCH |
| **APPROACH** | 열원 방향 직진<br>(lin_vel=0.15, 조향 보정) | d<0.5m & T>100°C → EXTINGUISH<br>열원 소실 → SEARCH |
| **EXTINGUISH** | 정지 후 소화<br>(분사 시도 5회) | T<40°C → SEARCH (성공)<br>5회 시도 후 열원 소실 실패 → APPROACH (재접근) |
| **SAFE_STOP** | 긴급 정지 (모든 구동 중단) | CLEAR_ESTOP 명령 → IDLE |


#### 주요 임계값 정의

```c
// state_machine.h
#define FIRE_DETECT_THRESHOLD      60.0f   // 열원 감지 (참고용)
#define FIRE_APPROACH_THRESHOLD    180.0f  // 소화 시작 온도
#define FIRE_LOST_THRESHOLD        35.0f   // 열원 소실 판정
#define DELTA_TEMP_THRESHOLD       15.0f   // dT 감지 임계값

// navigation.h
#define SEARCH_LINEAR_VEL          0.20f   // SEARCH 전진 속도
#define APPROACH_LINEAR_VEL        0.15f   // APPROACH 전진 속도
#define OBSTACLE_DISTANCE          0.30f   // 장애물 감지 거리 (m)

// extinguish_logic.h
#define SPRAY_DURATION_SEC         3.0f    // 분사 시간
#define EXTINGUISH_TEMP            40.0f   // 소화 성공 온도
#define EXTINGUISH_TRY             5       // 최대 분사 시도 횟수
```

### 통신 프로토콜 (IPC)

라즈베리파이와 PC 간 **TCP/IP 소켓** 통신을 사용함함



#### 프로토콜 구조
```
┌─────────────┐                      ┌──────────────┐
│ 라즈베리파이 │                      │ PC (Python)  │
│   (C)       │                      │ ui_remote.py │
└──────┬──────┘                      └──────┬───────┘
       │                                     │
       │  상태 전송 (200ms 주기)             │
       │ ──────────────────────────────────►│
       │  MODE=SEARCH;T_FIRE=85.3;...        │
       │                                     │
       │  명령 수신 (비동기)                  │
       │◄─────────────────────────────────── │
       │  START / STOP / ESTOP               │
```

#### 상태 메시지 포맷

```
MODE=<상태>;T_FIRE=<온도>;DT=<온도차>;D=<거리>;HOT=<행>,<열>;ESTOP=<0/1>\n
```

**예시:**
```
MODE=SEARCH;T_FIRE=85.3;DT=22.1;D=1.25;HOT=4,7;ESTOP=0
MODE=EXTINGUISH;T_FIRE=130.2;DT=50.0;D=0.35;HOT=6,9;ESTOP=0
```

#### 명령 메시지

| 명령 | 기능 |
|------|------|
| START | 자율 주행 시작 (IDLE → SEARCH) |
| STOP | 일반 정지 (→ IDLE) |
| MANUAL | 수동 모드 전환 |
| ESTOP | 긴급 정지 (→ SAFE_STOP) |
| CLEAR_ESTOP | 긴급 정지 해제 (→ IDLE) |
| EXIT | 프로그램 종료 |

---

## 4. 빌드 및 실행 방법

### 사전 준비

#### 하드웨어 설정

1. **I2C 활성화**
```bash
sudo raspi-config
# Interface Options → I2C → Enable
```

2. **디바이스 확인**
```bash
sudo i2cdetect -y 1
# 기대 결과: 0x33 (MLX90641), 0x5f (PCA9685)
```

3. **GPIO 권한 설정**
```bash
sudo usermod -aG gpio,i2c,spi $USER
# 재부팅 필요
sudo reboot
```

### 빌드 방법

#### 1) 저장소 클론

```bash
git clone https://github.com/namelesspark/FireDroneCar
cd FireDroneCar/fire-drone-car
```

#### 2) 빌드

```bash
# Makefile 사용
make clean
make

# 빌드 산출물 확인
ls -lh fire_drone_car
```

#### 3) 빌드 성공 확인

```bash
./fire_drone_car --version
# 출력: FireDroneCar v1.0
```

### 실행 방법

#### 라즈베리파이에서 메인 프로그램 실행

```bash
sudo ./fire_drone_car
```

**실행 시 출력 예시:**
```
========================================
========================================
        FireDroneCar 구동 시작           
========================================
========================================
[메인] wiringPi GPIO setup OK (BCM numbering)
[메인] shared state 초기화 완료
[메인] 열화상 카메라 정상
[메인] 모터 정상
[메인] 워터 펌프 정상
[메인] 서보 모터 정상
[메인] 스레드 생성 완료
[메인] 시스템 가동
[메인] Ctrl + C로 동작 중지
```

#### PC에서 원격 UI 실행

**Windows:**
```bash
pip install windows-curses
cd fire-drone-car/src/comm_ui
python ui_remote.py --host 192.168.1.100 --port 5000
```

**Linux/Mac:**
```bash
cd fire-drone-car/src/comm_ui
python3 ui_remote.py --host 192.168.1.100 --port 5000
```

**간단한 통신 테스트 (netcat 사용):**
```bash
# 터미널에서 간단히 연결 테스트
nc  5000

# 연결 후 명령 입력
START
# → 상태 메시지가 200ms마다 수신됨
```

### 실행 전 설정

#### 포트 설정 변경 (필요 시)

```c
// src/comm_ui/comm.c
#define COMM_PORT 5000  // 원하는 포트로 변경
```

#### 센서 임계값 조정

```c
// src/algo/state_machine.h
#define FIRE_APPROACH_THRESHOLD 180.0f  // 화재 감지/소화 시작 온도
#define FIRE_LOST_THRESHOLD     35.0f   // 열원 소실 온도
#define APPROACH_DISTANCE       0.5f    // 소화 거리 (m)
```

### 하드웨어 연결 순서

1. **전원 연결**: 라즈베리파이 및 Robot HAT에 전원 공급
2. **센서 연결**: 
   - MLX90641 → I2C (SDA/SCL, 주소 0x33)
   - HC-SR04 → GPIO23(TRIG), GPIO24(ECHO)
3. **모터 연결**: Robot HAT의 M1 단자에 DC 모터 연결 (채널 14, 15)
4. **서보 연결**: 
   - ch0: 목 틸트 (NECK_TILT)
   - ch1: 목 팬 (NECK_PAN)
   - ch2: 조향 (STEER)
5. **펌프 연결**: Robot HAT의 ch4에 릴레이를 통해 연결


### 테스트 프로그램 실행

개별 하드웨어 동작을 확인하려면:

```bash
cd fire-drone-car/tests

# 열화상 센서 테스트
sudo ./test_thermal

# 초음파 센서 테스트
sudo ./test_ultrasonic

# 펌프 테스트
sudo ./test_pump

# 조향 서보 테스트
sudo ./test_steer

# 목 서보 테스트
sudo ./test_neck

# DC 모터 테스트
sudo ./test_motor_angle
```

---

## 5. 데모 소개

### 데모 영상

**[장애물 회피 동작 영상]**

https://github.com/user-attachments/assets/ccf1ac07-1633-43f0-bddd-4d0c2afb525b


### 시나리오 설명

#### 시나리오 1: 기본 화재 진압

1. **초기 상태**
   - 로봇이 IDLE 상태로 대기
   - PC UI에서 "START" 명령 전송

2. **탐색 단계**
   - MODE=SEARCH로 전환
   - 로봇이 제자리 회전하며 열원 탐색
   - 열화상 센서가 80°C 이상 감지 시 → DETECT로 전환

3. **정렬 단계**
   - MODE=DETECT
   - 목 서보를 좌우로 움직이며 열원을 화면 중앙에 정렬
   - UI에서 HOT=(6,8) → (6,7) → (6,8) 변화 확인

4. **접근 단계**
   - MODE=APPROACH
   - 로봇이 열원 방향으로 전진
   - 초음파 센서로 거리 측정 (DIST: 1.5m → 0.8m → 0.4m)
   - T_FIRE 값 증가 (85°C → 105°C → 130°C)

5. **소화 단계**
   - MODE=EXTINGUISH
   - 정지 후 펌프 작동 (레벨 1부터 시작)
   - 3초 분사 후 온도 재측정
   - T_FIRE가 40°C 미만으로 떨어질 때까지 레벨 증가 (1→2→3)
   - 성공 시 → SEARCH로 복귀


#### 시나리오 2: 장애물 회피

1. **접근 중 장애물 감지**
   - APPROACH 상태에서 전진 중
   - 초음파 센서가 30cm 이내 장애물 감지
   - 자동으로 정지 및 회피 동작

2. **우회 후 재접근**
   - 회피 후 열원 재탐지
   - 다시 APPROACH → EXTINGUISH

---

## 6. 문제점 및 해결 방안 / 한계

### 개발 중 겪은 주요 문제와 해결

#### 1) 롤 펌프 DC 어댑터 선 끊어짐, 어댑터 타입 문제

**문제:**
- 워터펌프용 DC 어댑터 전원선이 물리적으로 끊어짐
- 라즈베리파이에 연결 불가능한 어댑터 규격 (기존 선 재사용 불가)

**해결 방법:**
- **Step 1**: 결속 부품 확인 및 대체 방안 수립
- **Step 2**: 릴레이 모듈 및 회로 설계
- **Step 3**: 전용 제어 채널 할당 및 결선
- **Step 4**: 구동 테스트 및 전력 안정성 확인

**[워터펌프와 DC 어댑터 연결 사진]**
<img width="1730" height="1730" alt="워터펌프 연결잭 사진" src="https://github.com/user-attachments/assets/4fb567b1-f059-420c-8516-e83fa5a7d5c2" />

---

<img width="1126" height="1502" alt="워터펌프 연결 사진" src="https://github.com/user-attachments/assets/501e307e-ccae-40ec-a1b0-3d737deae404" />

**채택한 해결책:**
```c
// pump_control.c
// PCA9685 Channel 4를 사용하여 펌프 ON/OFF 제어
void pump_on(void) {
    pca9685_setPWM(PUMP_CHANNEL, 0, 4095);  // Full duty = ON
}

void pump_off(void) {
    pca9685_setPWM(PUMP_CHANNEL, 0, 0);     // 0 duty = OFF
}
```

#### 2) 전원 공급 하드웨어 이슈로 인한 시스템 전체 장애

**문제:**
- DC 모터 속도 증가 시 라즈베리파이 전원이 갑자기 꺼지는 현상
- 초음파 센서도 동시에 측정 실패 발생
- `lin_vel` 값을 증가시킬수록 증상 악화

**원인 분석:**

```
증상 발생 메커니즘:
1. Differential gear 구동축 탈락 발견
   └─> 바퀴 회전 불가 상태에서 모터만 공회전
   
2. 속도 부족으로 lin_vel 값 증가 (0.15 → 0.3)
   └─> DC 모터에 더 높은 PWM duty 인가
   
3. 구동축 없이 모터만 과부하 회전
   └─> 순간 전류 급증 (정상: 1-2A → 비정상: 4A 이상 추정)
   
4. 배터리 전압 강하 (7.4V → 5V 이하)
   └─> 라즈베리파이 5V 레귤레이터 출력 저하
   
5. 라즈베리파이 Brown-out으로 자동 셧다운
   └─> 초음파 센서도 전원 부족으로 동작 중단
```
> 바퀴와 모터 사이 구동축이 빠진 상태, 이로 인해 모터가 공회전하며 과전류 발생

**해결:**
해결하지 못했음 - 하드웨어 부품 교체 필요


#### 3) SEARCH ↔ DETECT 상태 전환 진동 문제

**문제:**
- dT(온도 차이) 조건으로만 화재 감지 시 무한 반복 발생
- 실제 화재 근처 진입 시 주변 온도 상승으로 오탐 발생

| 문제점 | 입계값 | 기준 설정 |
|--------|--------|----------|
| dT 조건으로 쉽게 전환 | SEARCH → DETECT | T > 80°C 또는 dT > 20°C |
| 감지 입계값 너무 가까움 | DETECT → SEARCH | T < 50°C |

**해결 방안:**

| 변경 전 | 변경 후 | 역할 |
|---------|---------|------|
| FIRE_DETECT_THRESHOLD = 80°C | FIRE_APPROACH_THRESHOLD = 180°C | 명확한 화재만 감지 |
| FIRE_LOST_THRESHOLD = 50°C | FIRE_LOST_THRESHOLD = 35°C | 실온 근처로 판정 기준 완화 |

**결과:** 
- 실온 환경에서 안정적 동작 (손, 사람 체온 무시)
- 명확한 화재만 감지하여 오탐 감소


#### 4) 배터리 관리 및 테스트 중단 문제

**문제:**
- 알고리즘 통합 테스트 중 배터리 전체 방전
- 충전 시간 부족으로 테스트 일정 지연 (약 2-3시간 대기)
- 18650 배터리 2개 직렬 연결 사용 중

**영향:**
- 최종 통합 테스트에서 실제 주행 시나리오 검증 불가
- EXTINGUISH 상태의 5단계 소화 로직 실측 불가

**대응:**
대응하지 못했음 - 예비 배터리 팩 준비 필요


#### 5) 단일 공유 상태 구조체로 인한 Lock 경합

**문제:**
- 모든 스레드가 하나의 `shared_state_t` 구조체 공유
- 각 스레드는 일부 필드만 사용하지만 전체 구조체를 lock
- 불필요한 Lock 대기 시간 발생

**현재 구조:**
```c
typedef struct {
    pthread_mutex_t mutex;      // 하나의 mutex로 전체 보호
    
    // sensor_thread만 쓰기
    float t_fire, dT, distance;
    int hot_row, hot_col;
    
    // algo_thread만 쓰기
    robot_mode_t mode;
    
    // motor_thread만 읽기
    float lin_vel, ang_vel;
    
    // comm_thread만 읽기/쓰기
    cmd_mode_t cmd_mode;
    bool emergency_stop;
} shared_state_t;
```

**문제 시나리오:**
```
시간 t=0ms:
- motor_thread가 lin_vel, ang_vel 읽으려고 lock 획득

시간 t=1ms:
- sensor_thread가 t_fire 업데이트하려고 lock 시도
  → 대기! (motor_thread가 사용 중)
  → motor_thread는 t_fire를 전혀 안 쓰는데도 대기해야 함
```

**측정된 영향:**
| 스레드 | 사용하는 필드 | 사용 안 하는 필드 | Lock 대기 발생 |
|--------|--------------|------------------|---------------|
| sensor_thread | t_fire, dT, distance, hot_row/col (5개) | mode, lin_vel, ang_vel, cmd_mode 등 (6개) | algo_thread 실행 중 대기 |
| algo_thread | 전체 (11개) | 없음 | 다른 모든 스레드 실행 중 대기 |
| motor_thread | lin_vel, ang_vel (2개) | t_fire, dT, distance 등 (9개) | sensor_thread 실행 중 대기 |
| comm_thread | 전체 읽기, cmd_mode/emergency_stop 쓰기 | 없음 | 모든 스레드 실행 중 대기 |

**향후 개선 방안:**

**1. 필드별 개별 Mutex 사용**
```c
typedef struct {
    // 센서 데이터 (sensor_thread 쓰기, algo_thread 읽기)
    pthread_mutex_t sensor_mutex;
    float t_fire, dT, distance;
    int hot_row, hot_col;
    
    // 제어 명령 (algo_thread 쓰기, motor_thread 읽기)
    pthread_mutex_t control_mutex;
    float lin_vel, ang_vel;
    
    // 상태 (algo_thread 쓰기, comm_thread 읽기)
    pthread_mutex_t state_mutex;
    robot_mode_t mode;
    
    // 명령 (comm_thread 쓰기, algo_thread 읽기)
    pthread_mutex_t command_mutex;
    cmd_mode_t cmd_mode;
    bool emergency_stop;
} shared_state_improved_t;
```

**개선 효과:**
```
시간 t=0ms:
- motor_thread: control_mutex만 lock (lin_vel, ang_vel 읽기)
- sensor_thread: sensor_mutex만 lock (t_fire 업데이트)
  → 동시 실행 가능! Lock 경합 없음
```

**2. Lock-Free 구조 사용 (Read-Copy-Update)**
```c
typedef struct {
    // 읽기 전용 포인터 (atomic)
    _Atomic(sensor_data_t*) sensor_data;
    _Atomic(control_data_t*) control_data;
} shared_state_lockfree_t;

// sensor_thread
void update_sensor_data() {
    sensor_data_t* new_data = malloc(sizeof(sensor_data_t));
    new_data->t_fire = read_thermal();
    // ...
    
    // Atomic swap (lock 없음)
    sensor_data_t* old = atomic_exchange(&g_state.sensor_data, new_data);
    free(old);
}

// algo_thread
void read_sensor_data() {
    // Atomic load (lock 없음)
    sensor_data_t* data = atomic_load(&g_state.sensor_data);
    float temp = data->t_fire;  // 안전하게 읽기
}
```

**3. 메시지 큐 방식**
```c
// 각 스레드 간 독립적인 큐
message_queue_t sensor_to_algo_queue;
message_queue_t algo_to_motor_queue;

// sensor_thread
void* sensor_thread_func() {
    while(running) {
        sensor_data_t data = read_sensors();
        mq_send(&sensor_to_algo_queue, &data);  // lock 없음
    }
}

// algo_thread
void* algo_thread_func() {
    while(running) {
        sensor_data_t data;
        mq_receive(&sensor_to_algo_queue, &data);  // lock 없음
        // ...
    }
}
```

**성능 비교:**

| 방식 | Lock 대기 | 동시 실행 | 복잡도 | 비고 |
|------|----------|----------|--------|------|
| **현재 (Single Mutex)** | 높음 | ❌ 순차적 | 낮음 | 하나의 스레드만 접근 가능 |
| **개별 Mutex** | 낮음 | ✅ 부분 가능 | 중간 | 서로 다른 필드는 동시 접근 가능 |
| **Lock-Free** | 없음 | ✅ 완전 가능 | 높음 | Atomic operation 사용 |
| **Message Queue** | 매우 낮음 | ✅ 완전 가능 | 높음 | 버퍼링으로 인한 지연 존재 |

**현재 버전 선택 이유 (Trade-off):**
1. **구현 단순성**: 4주 프로젝트 기간 내 완성 가능
2. **디버깅 용이성**: 하나의 mutex로 동기화 이슈 추적 쉬움
3. **충분한 성능**: 
   - sensor_thread: 100ms 주기 (lock 시간 약 10ms)
   - algo_thread: 50ms 주기 (lock 시간 약 40ms, 핵심 로직)
   - motor_thread: 20ms 주기 (lock 시간 약 5ms)
   - 로봇 제어에는 현재 응답 속도로 충분
4. **안전성 우선**: Lock 누락으로 인한 Race Condition 위험 최소화

**개선 시 기대 효과:**
- **Lock 경합 감소**: 서로 독립적인 필드 접근 시 대기 불필요
- **센서 샘플링 향상**: sensor_thread가 다른 스레드에 덜 블로킹됨
- **실시간성 개선**: 긴급 정지 명령의 응답 지연 감소
- **확장성**: 추가 센서/구동기 통합 시 기존 Lock 영향 최소화

---

### 현재 버전의 한계점

1. **DC 모터 제어 불가**: 구동축 탈락 및 전선 끊김으로 실제 후륜 구동 테스트 불가
2. **워터펌프 분사량 부족**: 전압 문제로 물이 제대로 올라오지 않음
3. **통신 기능 미테스트**: TCP 서버 코드는 구현되어 있으나, 원격 UI와 연동한 실제 테스트 미완료
4. **배터리 수명**: 연속 동작 시간 약 20-30분으로 장시간 테스트 어려움
5. **단일 Mutex**: 스레드 간 Lock 경합 발생 가능

---

### 시간이 더 있었으면 하고 싶었던 개선 사항

#### 1. 전원 회로 분리

**현재 문제**: 모터 구동 시 순간 전류 급증으로 라즈베리파이 Brown-out 발생

**개선 방안**:
- DC 모터용 전원과 라즈베리파이 전원을 물리적으로 분리
- 모터 전용 7.4V 배터리 팩 + 라즈베리파이 전용 5V 보조배터리 구성
- 전압 강하 방지를 위한 DC-DC 컨버터(Buck Converter) 적용
- 각 전원 라인에 퓨즈 및 역전압 보호 다이오드 추가

**기대 효과**: 모터 과부하 시에도 제어 시스템 안정성 확보, 센서 측정 오류 감소

---

#### 2. 필드별 Mutex 분리

**현재 문제**: 단일 mutex로 전체 `shared_state_t` 보호 → 불필요한 Lock 경합 발생

**개선 방안**:
```c
typedef struct {
    // 센서 데이터 (sensor_thread 쓰기, algo_thread 읽기)
    pthread_mutex_t sensor_mutex;
    float t_fire, dT, distance;
    int hot_row, hot_col;
    
    // 제어 명령 (algo_thread 쓰기, motor_thread 읽기)
    pthread_mutex_t control_mutex;
    float lin_vel, ang_vel;
    
    // 로봇 상태 (algo_thread 쓰기, comm_thread 읽기)
    pthread_mutex_t state_mutex;
    robot_mode_t mode;
    
    // 외부 명령 (comm_thread 쓰기, algo_thread 읽기)
    pthread_mutex_t command_mutex;
    cmd_mode_t cmd_mode;
    bool emergency_stop;
} shared_state_improved_t;
```

**기대 효과**: 
- sensor_thread와 motor_thread가 동시 실행 가능 (서로 다른 필드 접근)
- 긴급 정지 명령의 응답 지연 감소
- 센서 샘플링 주기 안정화

---

#### 3. PID 제어 도입

**현재 문제**: 단순 비례 제어로 조향 시 오버슈트 및 진동 발생

**개선 방안**:
```c
typedef struct {
    float Kp, Ki, Kd;       // PID 게인
    float integral;          // 적분 누적값
    float prev_error;        // 이전 오차 (미분용)
    float integral_limit;    // Anti-windup 제한
} pid_controller_t;

float pid_compute(pid_controller_t *pid, float setpoint, float measured, float dt) {
    float error = setpoint - measured;
    
    // 적분 (Anti-windup 적용)
    pid->integral += error * dt;
    pid->integral = fmaxf(-pid->integral_limit, fminf(pid->integral_limit, pid->integral));
    
    // 미분
    float derivative = (error - pid->prev_error) / dt;
    pid->prev_error = error;
    
    return pid->Kp * error + pid->Ki * pid->integral + pid->Kd * derivative;
}
```

**적용 대상**:
- 열원 중앙 정렬 시 목 서보 각도 제어
- APPROACH 상태에서 조향 각도 제어
- 직진 주행 시 편차 보정

**기대 효과**: 부드러운 조향, 정렬 시간 단축, 오버슈트 감소

---

#### 4. 열화상 이미지 실시간 스트리밍

**현재 문제**: 원격 UI에서 숫자 데이터만 확인 가능, 열원 분포 시각화 불가

**개선 방안**:
```c
// 상태 메시지에 열화상 프레임 추가
void comm_format_thermal_frame(char *buf, size_t size, float frame[12][16]) {
    // 192개 픽셀을 압축하여 전송
    // 방법 1: Base64 인코딩
    // 방법 2: 온도값을 0-255 범위로 정규화하여 바이너리 전송
    // 방법 3: 변화가 큰 픽셀만 선택적 전송 (델타 압축)
}
```

**Python UI 측 시각화**:
```python
import numpy as np
import matplotlib.pyplot as plt

def render_thermal_heatmap(frame_data):
    frame = np.array(frame_data).reshape(12, 16)
    plt.imshow(frame, cmap='hot', interpolation='bilinear')
    plt.colorbar(label='Temperature (°C)')
```

**기대 효과**: 화재 위치 직관적 파악, 디버깅 용이, 열원 분포 패턴 분석 가능

---

#### 5. 웹 기반 대시보드 UI

**현재 문제**: curses 기반 터미널 UI는 설치 필요, 시각적 제한

---

## 7. 팀원 및 역할

| 이름 | 역할 | 담당 모듈 | 주요 기여 |
|------|------|-----------|----------|
| **박민호** | 자율주행 알고리즘 개발 | `src/algo/` | - 6-상태 설계 및 구현<br>- 네비게이션 로직 (탐색/정렬/접근)<br>- 단계별 소화 알고리즘<br>- 스레드 동기화 설계 |
| **김효빈** | 임베디드(모터 제어 함수) 개발 | `src/embedded/` | - MLX90641 드라이버 포팅<br>- PCA9685 기반 모터/서보 제어<br>- 초음파 센서 드라이버 구현<br>- 적외선 카메라 열원 행, 열 값 반환 함수 구현<br>-하드웨어 테스트 프로그램 작성 |
| **노경민** | 통신/UI 개발 | `src/comm_ui/` | - TCP 소켓 서버 구현<br>- 통신 프로토콜 설계<br>- Python curses 기반 원격 UI<br>- 실시간 모니터링 시스템 |
| **이상엽** | 불꽃소방대 팀장, 하드웨어 설계 | `hardware/` | - 회로 설계 및 배선<br>- 기구 설계 및 조립<br>- BOM 작성 및 부품 조달<br>- |

### 협업 도구
- **버전 관리**: Git/GitHub (Feature Branch 전략)
- **문서화**: README
- **커뮤니케이션**: 카카오톡, 대면 회의, Discord 온라인 회의 (주 2회)


---

## 📚 참고 자료

- [MLX90641 Datasheet](https://www.melexis.com/en/documents/documentation/datasheets/datasheet-mlx90641)
- [Waveshare Motor HAT Documentation](https://www.waveshare.com/wiki/Motor_Driver_HAT)
- [WiringPi Reference](http://wiringpi.com/reference/)
- [PCA9685 Datasheet](https://www.nxp.com/docs/en/data-sheet/PCA9685.pdf)

---

본 프로젝트는 국립금오공과대학교 임베디드 시스템 과목의 팀 프로젝트로 제작되었습니다. (금붕이, 금순이 4명)

---
## 📞 문의
**EMAIL**
- 노경민: sadong2135@gmail.com
- 박민호: jade.lake8852@gmail.com
- 이상엽: dltkdduq8732@naver.com
- 김효빈: gyqls09@gmail.com

**GitHub Repository**: https://github.com/namelesspark/FireDroneCar
---

**최종 업데이트**: 2024년 12월 21일 20:33
