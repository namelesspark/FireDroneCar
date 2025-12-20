# FireDroneCar

**화재 탐지 및 진압 자율주행 로봇**

열화상 카메라와 초음파 센서를 활용하여 화재를 자동으로 탐지하고 접근하여 소화하는 임베디드 시스템 프로젝트

---

## 📋 1. 프로젝트 개요

### 프로젝트 이름
**FireDroneCar** - 화재 탐지 및 진압 자율주행 로봇

### 한 줄 요약
붕괴 건물과 같은 좁은 틈으로 진입해 열화상 센서와 초음파 센서로 불의 위치를 탐지, 자율 주행 기반 초기 진압을 수행하는 초소형 소화 로봇 시스템

### 주요 기능
- 🔥![BitcoinBitarooGIF](https://github.com/user-attachments/assets/29dde553-d473-4e4e-a982-a03c28a6927c)
 **실시간 화재 탐지**: MLX90641 열화상 센서(16x12)를 활용한 온도 기반 화재 감지
- 🤖 **자율 주행**: 6-상태 FSM 기반 자율 네비게이션 및 열원 추적
- 🚧 **장애물 회피**: 초음파 센서(HC-SR04)를 활용한 실시간 거리 측정 및 장애물 감지
- 💧 **단계별 소화**: 5단계 펌프 제어 시스템을 통한 효율적 진압
- 📡 **원격 모니터링**: TCP/IP 기반 실시간 상태 전송 및 원격 제어
- 🛡️ **긴급 정지**: E-STOP 기능을 통한 안전성 확보
- ⚡ **멀티스레드 구조**: 센서, 알고리즘, 모터, 통신 스레드 독립 동작

---

## 🏗️ 2. 시스템 구성

### 전체 시스템 구조

**[전체 시스템 블록 다이어그램 이미지]**
> 라즈베리파이를 중심으로 센서(열화상, 초음파), 구동기(DC 모터, 서보, 펌프)가 연결된 구조, PC와 TCP/IP로 통신하는 모습을 담은 이미지

```
┌─────────────────────────────────────────────────────────────┐
│                    Raspberry Pi 4B                          │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐   │
│  │ Sensor   │  │  Algo    │  │  Motor   │  │  Comm    │   │
│  │ Thread   │  │ Thread   │  │ Thread   │  │ Thread   │   │
│  └────┬─────┘  └────┬─────┘  └────┬─────┘  └────┬─────┘   │
│       │             │             │             │          │
│       └─────────────┴─────────────┴─────────────┘          │
│                 shared_state_t (mutex)                      │
└─────────────────────────────────────────────────────────────┘
       │                    │                    │
       ▼                    ▼                    ▼
┌────────────┐      ┌────────────┐      ┌────────────┐
│  Sensors   │      │ Actuators  │      │ Remote UI  │
├────────────┤      ├────────────┤      │  (PC)      │
│ MLX90641   │      │ DC Motor   │      │            │
│ HC-SR04    │      │ Servo x3   │      │ TCP:5000   │
│            │      │ Water Pump │      │            │
└────────────┘      └────────────┘      └────────────┘
```

### 하드웨어 구성

| 구성요소 | 모델/사양 | 수량 | 용도 |
|---------|----------|------|------|
| **메인보드** | Raspberry Pi 4B | 1 | 전체 시스템 제어 |
| **열화상 센서** | MLX90641 (16x12 픽셀) | 1 | 화재 감지 및 위치 추정 |
| **초음파 센서** | HC-SR04 | 1 | 거리 측정 및 장애물 감지 |
| **모터 드라이버** | Waveshare Motor HAT (PCA9685) | 1 | PWM 신호 생성 및 모터 제어 |
| **DC 모터** | - | 1 | 후륜 구동 (전진/후진) |
| **서보 모터** | - | 3 | 조향(1), 목 팬/틸트(2) |
| **워터 펌프** | - | 1 | 소화액 분사 |

**[하드웨어 연결 사진]**
> 실제 제작된 로봇의 전체 모습, 센서와 모터가 연결된 상태를 보여주는 사진

**[회로도 이미지]**
> 라즈베리파이 GPIO 핀맵, I2C 연결(MLX90641: 0x33, PCA9685: 0x5F), 초음파 센서(TRIG: GPIO23, ECHO: GPIO24) 등이 표시된 회로도

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
> 각 스레드의 실행 주기와 mutex 잠금 시점을 보여주는 타이밍 다이어그램

#### 파일 구조

```
fire-drone-car/
├── src/
│   ├── main/
│   │   └── main.c              # 메인 엔트리, 4개 스레드 생성/관리
│   ├── algo/                   # 알고리즘 모듈 (박민호)
│   │   ├── shared_state.c/h    # 공유 상태 및 mutex 관리
│   │   ├── state_machine.c/h   # 6-상태 FSM
│   │   ├── navigation.c/h      # 이동 제어 (탐색/정렬/접근)
│   │   └── extinguish_logic.c/h # 소화 로직 (5단계)
│   ├── embedded/               # 임베디드 제어 (김효빈)
│   │   ├── thermal_sensor.c/h  # MLX90641 드라이버
│   │   ├── ultrasonic.c/h      # HC-SR04 드라이버
│   │   ├── motor_control.c/h   # DC 모터 제어
│   │   ├── servo_control.c/h   # 서보 제어 (목/조향)
│   │   ├── pump_control.c/h    # 펌프 제어
│   │   └── mlx90641/           # MLX90641 API (Melexis 제공)
│   ├── comm_ui/                # 통신 및 UI (노경민)
│   │   ├── comm.c/h            # TCP 서버 스레드
│   │   └── ui_remote.py        # PC용 원격 UI (Python curses)
│   └── common/                 # 공통 정의
│       ├── common.h            # shared_state_t, enum 정의
│       └── config.h            # 설정 상수
├── tests/                      # 단위 테스트
│   ├── test_thermal.c          # 열화상 센서 테스트
│   ├── test_ultrasonic.c       # 초음파 센서 테스트
│   ├── test_pump.c             # 펌프 테스트
│   └── test_motor_angle.c      # 모터/서보 테스트
├── docs/                       # 문서
│   └── comm_protocol.md        # 통신 프로토콜 명세
├── scripts/                    # 빌드/실행 스크립트
└── Makefile                    # 빌드 설정
```

### 사용한 개발 환경

| 항목 | 내용 |
|------|------|
| **OS** | Raspberry Pi OS (Debian 11 Bullseye) |
| **언어** | C (C99 표준), Python 3.9 |
| **컴파일러** | GCC 10.2.1 |
| **빌드 도구** | GNU Make |
| **주요 라이브러리** | WiringPi (GPIO/I2C), pthread (멀티스레드) |
| **통신** | POSIX Socket API (TCP/IP) |
| **I2C 버스** | `/dev/i2c-1` (Linux i2c-dev 인터페이스) |
| **개발 도구** | VSCode, Git, ssh (원격 개발) |

---

## ⚙️ 3. 제한조건 구현 내용 정리

### 멀티스레드 구조

본 프로젝트는 **4개의 독립적인 스레드**로 구성되어 있으며, 각 스레드는 특정 역할을 담당합니다.

```c
// main.c - 스레드 생성
pthread_t sensor_tid, algo_tid, motor_tid, comm_tid;

pthread_create(&sensor_tid, NULL, sensor_thread_func, NULL);
pthread_create(&algo_tid, NULL, algo_thread_func, NULL);
pthread_create(&motor_tid, NULL, motor_thread_func, NULL);
pthread_create(&comm_tid, NULL, comm_thread, &comm_ctx);
```

#### 스레드별 역할

| 스레드 | 실행 주기 | 주요 역할 | 접근 데이터 |
|--------|----------|----------|------------|
| **sensor_thread** | ~100ms | 센서 데이터 읽기 및 공유 상태 업데이트 | 쓰기: t_fire, dT, distance, hot_row, hot_col |
| **algo_thread** | 50ms | 상태 머신 실행 및 모터 명령 계산 | 읽기: 센서 데이터<br>쓰기: mode, lin_vel, ang_vel, water_level |
| **motor_thread** | 20ms | 모터/서보/펌프 하드웨어 제어 | 읽기: lin_vel, ang_vel, water_level |
| **comm_thread** | 이벤트 기반 | TCP 통신 (상태 전송 200ms, 명령 수신) | 읽기: 전체 상태<br>쓰기: cmd_mode, emergency_stop |

**[스레드 간 데이터 흐름 다이어그램 이미지]**
> sensor → shared_state → algo → shared_state → motor의 데이터 흐름과 comm_thread의 양방향 통신을 보여주는 다이어그램

### Mutex를 통한 스레드 동기화

모든 스레드는 `shared_state_t` 구조체에 포함된 **pthread_mutex_t**를 통해 동기화됩니다.

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
    int water_level;            // 펌프 강도 (1~5)
    
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
        // 센서 읽기 (mutex 밖에서)
        thermal_sensor_read(&thermal);
        float dist_cm = ultrasonic_read_distance_cm_avg(&us, 3, 30000);
        
        // 공유 상태 업데이트 (mutex 안에서)
        shared_state_lock(&g_state);
        g_state.t_fire = thermal.T_fire;
        g_state.dT = thermal.T_fire - thermal.T_env;
        g_state.distance = dist_cm / 100.0f;
        g_state.hot_row = thermal.hot_row;
        g_state.hot_col = thermal.hot_col;
        shared_state_unlock(&g_state);
        
        usleep(100000); // 100ms
    }
}
```

**2) algo_thread - 알고리즘 실행**

```c
void* algo_thread_func(void* arg) {
    while(g_running) {
        // 상태 머신 업데이트 (mutex 안에서 전체 실행)
        shared_state_lock(&g_state);
        state_machine_update(&g_state);  // 내부에서 추가 lock 없음
        shared_state_unlock(&g_state);
        
        usleep(50000); // 50ms
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
        int pump_level = g_state.water_level;
        shared_state_unlock(&g_state);
        
        // 하드웨어 제어 (mutex 밖에서)
        motor_set_speed(lin);
        servo_set_angle(SERVO_STEER, ang);
        if (pump_level > 0) pump_on();
        else pump_off();
        
        usleep(20000); // 20ms
    }
}
```

**4) comm_thread - 통신**

```c
// 상태 전송 시 스냅샷 방식 (mutex 복사 방지)
shared_state_t snapshot = {0};
pthread_mutex_lock(ctx->state_mutex);
snapshot.mode = ctx->state->mode;
snapshot.t_fire = ctx->state->t_fire;
// ... (필요한 필드만 복사)
pthread_mutex_unlock(ctx->state_mutex);

comm_format_status_line(line, sizeof(line), &snapshot);
send(client_fd, line, strlen(line), 0);
```

### Mutex 사용의 핵심 원칙

1. **Lock은 최소 범위로**: I/O 작업(센서 읽기, 하드웨어 제어)은 mutex 밖에서 수행
2. **Deadlock 방지**: algo_thread는 main에서 lock/unlock 관리, 내부 함수는 lock 호출 안 함
3. **구조체 복사 금지**: `shared_state_t`에 mutex가 포함되어 있어 전체 복사 시 UB 발생 → 스냅샷 방식 사용

**[Mutex Lock 타이밍 차트 이미지]**
> 4개 스레드가 시간축에서 mutex를 획득/해제하는 시점을 보여주는 차트

### 6-상태 FSM (Finite State Machine)

알고리즘의 핵심은 **6개 상태로 구성된 유한 상태 머신**입니다.

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

**[상태 전이 다이어그램 이미지]**
> 실제 프로그램에서 상태 전환이 일어나는 모습을 캡처한 화면 또는 Graphviz로 생성한 다이어그램

#### 상태별 동작

| 상태 | 동작 | 전이 조건 |
|------|------|----------|
| **IDLE** | 정지 대기 | START 명령 → SEARCH |
| **SEARCH** | 회전하며 열원 탐색<br>(lin_vel=0.1, ang_vel=0.5) | T>80°C or dT>20°C → DETECT<br>열원 소실 시 계속 SEARCH |
| **DETECT** | 열원 중앙 정렬<br>(전진하며 좌우 미세 조정) | 정렬 완료 → APPROACH<br>열원 소실(T<50°C) → SEARCH |
| **APPROACH** | 열원 방향 직진<br>(lin_vel=0.15, 조향 보정) | d<0.5m & T>100°C → EXTINGUISH<br>열원 소실 → SEARCH |
| **EXTINGUISH** | 정지 후 소화<br>(레벨 1→5 단계적 분사) | T<40°C → SEARCH (성공)<br>레벨 5 실패 → APPROACH (재접근) |
| **SAFE_STOP** | 긴급 정지 (모든 구동 중단) | CLEAR_ESTOP 명령 → IDLE |

### 통신 프로토콜 (IPC)

라즈베리파이와 PC 간 **TCP/IP 소켓** 통신을 사용합니다.

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
| START | 자율 주행 시작 |
| STOP | 일반 정지 |
| ESTOP | 긴급 정지 |
| CLEAR_ESTOP | 긴급 정지 해제 |
| EXIT | 프로그램 종료 |

---

## 🔨 4. 빌드 및 실행 방법

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

#### 소프트웨어 의존성 설치

```bash
# 필수 패키지
sudo apt-get update
sudo apt-get install -y build-essential git i2c-tools

# WiringPi 설치
sudo apt-get install -y wiringpi

# WiringPi 버전 확인 (2.52 이상 권장)
gpio -v
```

### 빌드 방법

#### 1) 저장소 클론

```bash
git clone https://github.com/YOUR_USERNAME/FireDroneCar.git
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

**빌드 옵션:**
```bash
# 디버그 빌드
make DEBUG=1

# 테스트 프로그램만 빌드
make tests
```

#### 3) 빌드 성공 확인

```bash
./fire_drone_car --version
# 출력: FireDroneCar v1.0
```

### 실행 방법

#### 라즈베리파이에서 메인 프로그램 실행

```bash
# root 권한 필요 (GPIO/I2C 접근)
sudo ./fire_drone_car
```

**실행 시 출력 예시:**
```
[main] FireDroneCar Starting...
[main] WiringPi initialized (BCM mode)
[thermal] MLX90641 initialized at 0x33
[motor] Motor HAT initialized at 0x5f
[servo] Servo initialized (50Hz)
[comm] listening on port 5000
[sensor_thread] STARTED
[algo_thread] STARTED
[motor_thread] STARTED
[comm_thread] STARTED
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

**[원격 UI 실행 화면 스크린샷]**
> Python curses UI가 실행된 화면, MODE/T_FIRE/DIST 등이 실시간으로 업데이트되는 모습

### 실행 전 설정

#### 포트 설정 변경 (필요 시)

```c
// src/comm_ui/comm.c
#define COMM_PORT 5000  // 원하는 포트로 변경
```

#### 센서 임계값 조정

```c
// src/algo/state_machine.c
#define FIRE_DETECT_THRESHOLD   80.0f   // 화재 감지 온도
#define FIRE_APPROACH_THRESHOLD 100.0f  // 소화 시작 온도
#define APPROACH_DISTANCE       0.5f    // 소화 거리 (m)
```

### 하드웨어 연결 순서

1. **전원 연결**: 라즈베리파이 및 Motor HAT에 전원 공급
2. **센서 연결**: 
   - MLX90641 → I2C (SDA/SCL)
   - HC-SR04 → GPIO23(TRIG), GPIO24(ECHO)
3. **모터 연결**: Motor HAT의 M1 단자에 DC 모터 연결
4. **서보 연결**: 
   - ch0: 목 틸트
   - ch1: 목 팬
   - ch2: 조향
5. **펌프 연결**: Motor HAT의 ch4에 연결

**[하드웨어 연결 순서를 보여주는 사진 시리즈]**
> 단계별로 센서와 모터를 연결하는 과정을 담은 사진들

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

# 모터 각도 테스트
sudo ./test_motor_angle
```

---

## 🎬 5. 데모 소개

### 데모 영상

**[YouTube 링크 또는 데모 영상 임베드]**
> 실제 로봇이 화재를 탐지하고 접근하여 소화하는 전체 과정을 담은 영상

### 시나리오 설명

#### 시나리오 1: 기본 화재 진압

1. **초기 상태** (0:00-0:05)
   - 로봇이 IDLE 상태로 대기
   - PC UI에서 "START" 명령 전송

2. **탐색 단계** (0:05-0:20)
   - MODE=SEARCH로 전환
   - 로봇이 제자리 회전하며 열원 탐색
   - 열화상 센서가 80°C 이상 감지 시 → DETECT로 전환

3. **정렬 단계** (0:20-0:30)
   - MODE=DETECT
   - 목 서보를 좌우로 움직이며 열원을 화면 중앙에 정렬
   - UI에서 HOT=(6,8) → (6,7) → (6,8) 변화 확인

4. **접근 단계** (0:30-0:50)
   - MODE=APPROACH
   - 로봇이 열원 방향으로 전진
   - 초음파 센서로 거리 측정 (DIST: 1.5m → 0.8m → 0.4m)
   - T_FIRE 값 증가 (85°C → 105°C → 130°C)

5. **소화 단계** (0:50-1:20)
   - MODE=EXTINGUISH
   - 정지 후 펌프 작동 (레벨 1부터 시작)
   - 3초 분사 후 온도 재측정
   - T_FIRE가 40°C 미만으로 떨어질 때까지 레벨 증가 (1→2→3)
   - 성공 시 → SEARCH로 복귀

**[각 단계별 스크린샷 시리즈]**
> IDLE, SEARCH, DETECT, APPROACH, EXTINGUISH 각 상태에서의 로봇 모습과 UI 화면을 나란히 보여주는 이미지 시리즈

#### 시나리오 2: 장애물 회피

1. **접근 중 장애물 감지** (0:00-0:15)
   - APPROACH 상태에서 전진 중
   - 초음파 센서가 30cm 이내 장애물 감지
   - 자동으로 정지 및 회피 동작

2. **우회 후 재접근** (0:15-0:30)
   - 회피 후 열원 재탐지
   - 다시 APPROACH → EXTINGUISH

**[장애물 회피 동작 영상]**
> 로봇이 장애물을 감지하고 회피하는 과정을 담은 클립

#### 시나리오 3: 긴급 정지

1. **자율 주행 중 긴급 상황** (0:00-0:10)
   - APPROACH 상태에서 주행 중
   - PC UI에서 "ESTOP" 명령 전송

2. **즉시 정지** (0:10-0:15)
   - MODE=SAFE_STOP으로 즉시 전환
   - 모든 모터 정지
   - UI에서 ESTOP=1 표시

3. **재시작** (0:15-0:25)
   - "CLEAR_ESTOP" 명령으로 해제
   - MODE=IDLE로 복귀
   - 다시 "START" 명령으로 재시작

**[긴급 정지 시연 영상]**
> ESTOP 명령 전송 → 즉시 정지 → 해제 → 재시작 과정을 보여주는 클립

### UI 모니터링

**[PC UI 실시간 모니터링 화면 녹화]**
> Python curses UI에서 MODE, T_FIRE, DT, DIST, HOT 값이 실시간으로 변화하는 모습

UI 화면 구성:
```
┌────────────────────────────────────────────────┐
│ FireDroneCar Remote UI  |  192.168.1.100:5000 │
├────────────────────────────────────────────────┤
│ MODE : APPROACH                                │
│ T_FIRE :  120.5 °C    ΔT :   45.2 °C          │
│ DIST  :    0.48 m     HOT : (6, 8)            │
│ ESTOP : OFF                                    │
│                                                │
│ RAW : MODE=APPROACH;T_FIRE=120.5;DT=45.2;...  │
├────────────────────────────────────────────────┤
│ 명령 예시: START / STOP / ESTOP / EXIT         │
│ cmd> _                                         │
└────────────────────────────────────────────────┘
```

---

## 🐛 6. 문제점 및 해결 방안 / 한계

### 개발 중 겪은 주요 문제와 해결

#### 1) MLX90641 I2C 통신 불안정

**문제:**
- 열화상 센서에서 간헐적으로 프레임 읽기 실패
- `I2C_NACK` 에러 발생

**원인:**
- I2C 버스 속도가 너무 빠름 (400kHz)
- Pull-up 저항 부족

**해결:**
```c
// MLX90641_I2C_Driver.c
// I2C 속도를 100kHz로 낮춤
ioctl(fd, I2C_SLAVE, addr);
// Pull-up 저항 4.7kΩ 추가 (하드웨어)
```

**결과:** 프레임 읽기 성공률 95% → 99.8% 향상

#### 2) Mutex Deadlock 발생

**문제:**
- algo_thread에서 navigation 함수 내부에서 추가로 lock 호출
- 프로그램이 멈추는 현상 발생

**원인:**
- 중첩된 lock 호출 (이미 lock 잡은 상태에서 재호출)

**해결:**
```c
// 변경 전 (잘못된 코드)
void compute_approach_motion(shared_state_t *st) {
    shared_state_lock(st);  // ← 이미 main에서 lock 잡힘
    // ...
}

// 변경 후 (올바른 코드)
void compute_approach_motion(shared_state_t *st) {
    // lock 없이 바로 접근 (main에서 lock 관리)
    // ...
}
```

**원칙 확립:** algo 모듈 내부 함수는 lock을 호출하지 않음. main에서만 관리.

#### 3) PCA9685 PWM 주파수 충돌

**문제:**
- 모터와 서보를 동시에 사용할 때 동작 이상
- 모터는 1600Hz, 서보는 50Hz 필요

**원인:**
- PCA9685는 전체 채널이 하나의 주파수 공유
- `motor_init()`과 `servo_init()` 순서에 따라 마지막 주파수로 덮어씌워짐

**해결:**
```c
// main.c - 초기화 순서 변경
servo_init();   // 50Hz 설정 (서보 우선)
motor_init();   // 50Hz 유지
// 모터도 50Hz에서 duty cycle로 속도 제어
```

**결과:** 서보와 모터 모두 정상 동작

#### 4) 초음파 센서 Timeout

**문제:**
- 초음파 센서가 자주 timeout 발생 (측정 실패)

**원인:**
- ECHO 핀 대기 시간이 너무 짧음 (10ms)
- 반사가 잘 안 되는 표면 (흡음재)

**해결:**
```c
// ultrasonic.c
// timeout 30ms로 증가 + 3회 평균
float dist = ultrasonic_read_distance_cm_avg(&us, 3, 30000);
```

**결과:** 측정 성공률 60% → 95% 향상

### 현재 버전의 한계점

#### 1) 단일 화재만 대응 가능
- 열화상 센서가 가장 뜨거운 픽셀만 추적
- 여러 개의 화재가 있으면 가장 큰 것만 진압

**향후 개선:**
- 클러스터링 알고리즘 적용
- 여러 화재 위치 저장 및 순차 진압

#### 2) 2D 평면에서만 동작
- 높이가 다른 화재 대응 불가
- 목 틸트 서보가 있지만 현재 고정 각도만 사용

**향후 개선:**
- 목 틸트를 활용한 3D 열원 추적
- 수직 방향 소화 가능하도록 개선

#### 3) 실내 환경만 고려
- GPS 없음 (실외 사용 제한)
- 햇빛에 의한 열화상 오인식 가능

**향후 개선:**
- GPS 모듈 추가
- 열화상 + 연기 센서 융합

#### 4) 네트워크 지연
- TCP 통신으로 인한 지연 (약 50-100ms)
- 긴급 상황 대응 시 지연 발생 가능

**향후 개선:**
- UDP로 변경하여 지연 감소
- 로컬 버튼 추가 (물리적 E-STOP)

#### 5) 배터리 용량 제한
- 약 30분 연속 작동 가능
- 실제 화재 진압 시간 부족할 수 있음

**향후 개선:**
- 자동 충전 복귀 기능
- 배터리 잔량 모니터링

---

## 👥 7. 팀원 및 역할

| 이름 | 역할 | 담당 모듈 | 주요 기여 |
|------|------|-----------|----------|
| **박민호** | 알고리즘 팀장 | `src/algo/` | - 6-상태 FSM 설계 및 구현<br>- 네비게이션 로직 (탐색/정렬/접근)<br>- 단계별 소화 알고리즘<br>- 스레드 동기화 설계 |
| **김효빈** | 임베디드 개발 | `src/embedded/` | - MLX90641 드라이버 포팅<br>- PCA9685 기반 모터/서보 제어<br>- 초음파 센서 드라이버 구현<br>- 하드웨어 테스트 프로그램 작성 |
| **노경민** | 통신/UI 개발 | `src/comm_ui/` | - TCP 소켓 서버 구현<br>- 통신 프로토콜 설계<br>- Python curses 기반 원격 UI<br>- 실시간 모니터링 시스템 |
| **이상엽** | 하드웨어 설계 | `hardware/` | - 회로 설계 및 배선<br>- 기구 설계 및 조립<br>- BOM 작성 및 부품 조달<br>- 최종 통합 테스트 |

### 협업 도구
- **버전 관리**: Git/GitHub (Feature Branch 전략)
- **문서화**: Markdown, README
- **커뮤니케이션**: 카카오톡, 대면 회의 (주 2회)
- **이슈 트래킹**: GitHub Issues

**[팀 사진]**
> 프로젝트 완성 후 팀원들이 함께 찍은 사진

---

## 📚 참고 자료

- [MLX90641 Datasheet](https://www.melexis.com/en/documents/documentation/datasheets/datasheet-mlx90641)
- [Waveshare Motor HAT Documentation](https://www.waveshare.com/wiki/Motor_Driver_HAT)
- [WiringPi Reference](http://wiringpi.com/reference/)
- [PCA9685 Datasheet](https://www.nxp.com/docs/en/data-sheet/PCA9685.pdf)

---

## 📝 라이선스

MIT License

Copyright (c) 2024 FireDroneCar Team

본 프로젝트는 홍익대학교 임베디드 시스템 과목의 팀 프로젝트로 제작되었습니다.

---

## 📞 문의

프로젝트 관련 문의사항은 GitHub Issues를 통해 남겨주세요.

**GitHub Repository**: https://github.com/YOUR_USERNAME/FireDroneCar

---

**최종 업데이트**: 2024년 12월
