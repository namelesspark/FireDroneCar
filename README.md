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
- 🔥 **실시간 화재 탐지**: MLX90641 열화상 센서(16x12)를 활용한 온도 기반 화재 감지
- 🤖 **자율 주행**: 6-상태 FSM 기반 자율 네비게이션 및 열원 추적
- 🚧 **장애물 회피**: 초음파 센서(HC-SR04)를 활용한 실시간 거리 측정 및 장애물 감지
- 🧯 **단계별 소화**: 5단계 펌프 제어 시스템을 통한 효율적 진압
- 📡 **원격 모니터링**: TCP/IP 기반 실시간 상태 전송 및 원격 제어
- 🛡️ **긴급 정지**: E-STOP 기능을 통한 안전성 확보
- ⚡ **멀티스레드 구조**: 센서, 알고리즘, 모터, 통신 스레드 독립 동작

---

## 🏗️ 2. 시스템 구성

### 전체 시스템 구조

**[전체 시스템 블록 다이어그램 이미지]**
<img width="4400" height="2970" alt="시스템구성도" src="https://github.com/user-attachments/assets/103fcfef-1b18-49ab-ad5c-6ace68744995" />


### 하드웨어 구성

| 구성요소 | 모델/사양 | 수량 | 용도 |
|---------|----------|------|------|
| **메인보드** | Raspberry Pi 5 | 1 | 전체 시스템 제어 |
| **열화상 센서** | MLX90641 (16x12 픽셀) | 1 | 화재 감지 및 위치 추정 |
| **초음파 센서** | HC-SR04 | 1 | 거리 측정 및 장애물 감지 |
| **모터 드라이버** | Waveshare Motor HAT (PCA9685) | 1 | PWM 신호 생성 및 모터 제어 |
| **DC 모터** | - | 1 | 후륜 구동 (전진/후진) |
| **서보 모터** | - | 3 | 조향(1), 목 팬/틸트(2) |
| **워터 펌프** | - | 1 | 소화액 분사 |

**[하드웨어 연결 사진]**
![20251219_114947](https://github.com/user-attachments/assets/8d783e52-4569-46a4-9058-de4fa4b576a2)



![20251219_114958](https://github.com/user-attachments/assets/7a2420a7-1ba5-4212-9db1-bd440c781421)



![20251219_115025](https://github.com/user-attachments/assets/8fef9fec-a618-49ad-a520-23976181e825)


**[회로도 이미지]**
<img width="7631" height="5455" alt="Mermaid Chart - Create complex, visual diagrams with text -2025-12-20-184915" src="https://github.com/user-attachments/assets/68d79e6f-7c62-48cc-ad74-85b366ad5253" />



<img width="4660" height="1595" alt="Mermaid Chart - Create complex, visual diagrams with text -2025-12-20-185036" src="https://github.com/user-attachments/assets/49571fe1-d1c5-4b90-9187-dda35d553bf6" />



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

- 알고리즘 스레드 타이밍 시퀀스 다이어그램
<img width="2808" height="2968" alt="알고리즘 스레드 타이밍 시퀀스 다이어그램" src="https://github.com/user-attachments/assets/cbae3730-6703-489f-8478-44fe6d3aee17" />



- 센서 스레드 타이밍 시퀀스 다이어그램
<img width="4917" height="3024" alt="센서 스레드 시퀀스" src="https://github.com/user-attachments/assets/94581bf1-2465-457f-9a46-ba77e2084cc0" />



- 라즈베리파이 통신 시퀀스 다이어그램
<img width="2973" height="2980" alt="라즈베리 파이 통신 시퀀스 다이어그램" src="https://github.com/user-attachments/assets/0b741217-75f9-4dd2-a194-48d9b4151a3a" />



- 전체 스레드 구성
<img width="3286" height="2617" alt="그림6" src="https://github.com/user-attachments/assets/32dee3b6-2169-4f37-b76f-778e71538f86" />





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
        .....
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
        float ang = g_state.ang_vel;  // 수정: ang_val -> ang_vel
        bool estop = g_state.emergency_stop;
        shared_state_unlock(&g_state);
        
        // 하드웨어 제어 (mutex 밖에서)
        if(estop) {
            motor_stop();
            servo_set_angle(SERVO_STEER, 90.0f);
        } else {
            motor_set_speed(lin); // 뒷바퀴 DC 모터 제어
            float steering_angle = 90.0f + (ang * 45.0f);  // -45도 ~ +45도 범위
            if (steering_angle < 45.0f)  steering_angle = 45.0f; // 각도 제한
            if (steering_angle > 135.0f) steering_angle = 135.0f;
            servo_set_angle(SERVO_STEER, steering_angle);
        }
        usleep(20000);  // 20ms (50Hz)
    }
}
```

**4) comm_thread - 통신**

```c
// 현재 구현 상태 없음
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
    printf("========================================\n");
    printf("========================================\n");
    printf("        FireDroneCar 구동 시작           \n");
    printf("========================================\n");
    printf("========================================\n");

    ... wirignPi 셋업 확인, 하드웨어 상태 확인 후
    printf("[메인] 스레드 생성 완료\n");
    printf("[메인] 시스템 가동\n");
    printf("[메인] Ctrl + C로 동작 중지\n");
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

**[각 단계별 스크린샷 시리즈]**
> IDLE, SEARCH, DETECT, APPROACH, EXTINGUISH 각 상태에서의 로봇 모습과 UI 화면을 나란히 보여주는 이미지 시리즈

#### 시나리오 2: 장애물 회피

1. **접근 중 장애물 감지**
   - APPROACH 상태에서 전진 중
   - 초음파 센서가 30cm 이내 장애물 감지
   - 자동으로 정지 및 회피 동작

2. **우회 후 재접근**
   - 회피 후 열원 재탐지
   - 다시 APPROACH → EXTINGUISH

**[장애물 회피 동작 영상]**
> 로봇이 장애물을 감지하고 회피하는 과정을 담은 클립



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
> 왼쪽: 끊어진 DC 어댑터 잭, 오른쪽: 릴레이를 통해 재연결된 워터펌프 시스템

**채택한 해결책:**
```c
// pump_control.c
// PCA9685 Channel 4를 사용하여 펌프 ON/OFF 제어
void pump_on(void) {
    // Servo port 4번을 통해 릴레이 모듈 제어
    setPWM(PUMP_CHANNEL, 0, 4095);  // Full duty = ON
}

void pump_off(void) {
    setPWM(PUMP_CHANNEL, 0, 0);     // 0 duty = OFF
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
해결하지 못했음


#### 3) SEARCH ↔ DETECT 상태 전환 진동 문제

**문제:**
- dT(온도 차이) 조건으로만 화재 감지 시 무한 반복 발생
- SEARCH 상태에서 갑자기 입계값 너무 가까움
  - 실제 화재(150°C) 근처 진입 주변 온도가 50°C일 때, 상온 25°C에서 순간적으로 dT > 20°C 감지 → DETECT 전환
  - 하지만 T_fire < 80°C이므로 다시 SEARCH 복귀

| 문제점 | 입계값 | 기준 설정 |
|--------|--------|----------|
| dT 조건으로 쉽게 전환 | SEARCH → DETECT | T > 80°C 또는 dT > 20°C |
| 감지 입계값 너무 가까움 | DETECT → SEARCH | T < 50°C |

**해결 방안:**

| 변경 | 역할 | 입계값 | 기준 설정 |
|------|------|--------|----------|
| 150°C | 열원 감지 | FIRE_DETECT_THRESHOLD | 80°C |
| 35°C | 열원 소실 판정 | FIRE_LOST_THRESHOLD | 50°C |
| 15°C | dT 판정 | DELTA_TEMP_THRESHOLD | 20°C |

```c
// state_machine.c - 수정 전
#define FIRE_DETECT_THRESHOLD   80.0f
#define DELTA_TEMP_THRESHOLD    20.0f

// state_machine.c - 수정 후
#define FIRE_DETECT_THRESHOLD   150.0f   // 열원 감지 (명확한 화재)
#define FIRE_LOST_THRESHOLD     35.0f    // 열원 소실 (실온 근처)
#define DELTA_TEMP_THRESHOLD    40.0f    // dT 판정
```

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
- 배터리 잔량 표시 LED 미확인

**대응:**
대응하지 못했음


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

## 👥 7. 팀원 및 역할

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

본 프로젝트는 국립금오공과대학교 임베디드 시스템 과목의 팀 프로젝트로 제작되었습니다. (금붕이, 금순이 4명)

---
## 📞 문의
**EMAIL**
노경민: sadong2135@gmail.com
박민호: jade.lake8852@gmail.com
이상엽: dltkdduq8732@naver.com
김효빈: gyqls09@gmail.com

**GitHub Repository**: https://github.com/namelesspark/FireDroneCar
---

**최종 업데이트**: 2024년 12월 21일
