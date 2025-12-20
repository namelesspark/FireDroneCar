### `src/embedded/` - 센서, 엑츄에이터 제어 README

## 1) 폴더/파일 구조

```text
fire-drone-car/
├─ src/
│  └─ embedded/
│     ├─ motor_control.c/.h        # DC 모터(후륜) 제어 (PCA9685)
│     ├─ servo_control.c/.h        # 목(팬/틸트) + 조향 서보 (PCA9685)
│     ├─ pump_control.c/.h         # 워터펌프 ON/OFF (PCA9685)
│     ├─ ultrasonic.c/.h           # 초음파 거리 측정 (GPIO)
│     ├─ thermal_sensor.c/.h       # MLX90641 wrapper (프레임 -> 요약)
│     └─ mlx90641/
│        ├─ MLX90641_API.c/.h      # Melexis 제공 API
│        └─ MLX90641_I2C_Driver.c/.h # /dev/i2c-* 기반 I2C 드라이버
└─ tests/
   ├─ test_thermal.c               # 열화상 센서 단위 테스트
   ├─ test_ultrasonic.c            # 초음파 센서 단위 테스트
   ├─ test_pump.c                  # 펌프 단위 테스트(대화형)
   ├─ test_neck.c                  # 목 서보 단위 테스트
   ├─ test_motor_angle.c           # 조향 서보 각도 테스트
   └─ test_steer.c                 # DC 모터 테스트

---

## 2) 핵심 설계 원칙 (Embedded/HAL)

1. **HAL은 “하드웨어 접근만” 담당**
   - 알고리즘/상태머신(`src/algo/`)은 *무슨 동작을 할지* 결정
   - embedded는 *결정된 명령을 하드웨어로 변환/출력* (I2C/GPIO)

2. **초기화는 idempotent(여러 번 호출돼도 안전)**
   - `*_init()`는 내부 fd/핸들 상태를 보고 **중복 초기화 방지**

3. **단위/범위를 명확히 고정**
   - 모터: `[-1.0, +1.0]` (부호로 전/후진)
   - 서보: `[0°, 180°]`
   - 초음파: `cm` 반환 (상위에서 `m`로 변환)

4. **스레드 모델을 의식한 접근**
   - `main.c`는 멀티스레드(센서/알고리즘/모터/통신)

---

## 3) 전체 상태머신(알고리즘)과 Embedded의 역할

상태머신은 `src/algo/state_machine.c`에 있으며, Embedded는 각 상태에서 “어떤 하드웨어가 어떻게 움직여야 하는지”를 담당합니다.

```text
  [IDLE]
    |  (CMD START)
    v
  [SEARCH]  --(T_fire 충분히 큼)-->  [DETECT]  --(조준 안정)--> [EXTINGUISH]
     | (열원 소실/리셋)                 |                        |
     +-------------------------------<--+                        |(실패/더 가까이)
                                                                  v
                                                             [APPROACH]

  언제든 emergency_stop -> [SAFE_STOP]
```

### 상태별 하드웨어 동작 요약

| 상태 | 센서(입력) | 구동기(출력) | 핵심 포인트 |
|---|---|---|---|
| IDLE | (선택) 초음파/열화상 | 모터/조향 정지, 목 정면 | 안전 대기 |
| SEARCH | 열화상/초음파 | DC 모터 전진 + 목 스캔 + 회피 FSM | 장애물 회피는 초음파 기반 |
| DETECT | 열화상(hot_col) | **목 팬**으로만 조준(차체 정지) | 핫스팟 흔들림 안정화 카운트 |
| APPROACH | 열화상+초음파 | 전진 + 조향 보정 | 너무 가까우면 정지 |
| EXTINGUISH | 열화상 | 펌프 분사(ON/OFF) | 일정 시간 분사 후 온도 재평가 |
| SAFE_STOP | - | 모터 정지 + 조향 중앙 | 최우선 안전 |

> 실제 제어 루프는 `main.c`의 `algo_thread`(상태 업데이트) + `motor_thread`(모터/조향 출력) + `sensor_thread`(센서 업데이트)로 구성되어 있습니다.

---

## 4) 모듈별 상세

### 4.1 DC 모터 — `motor_control.c/.h`

**역할**: 후륜 DC 모터(M1)를 PWM으로 구동

**하드웨어/버스**
- I2C: `/dev/i2c-1`
- PWM 드라이버: PCA9685 (`0x5f`)
- 사용 채널: `M1_IN1_CH=15`, `M1_IN2_CH=14`

**API**
```c
int  motor_init(void);
void motor_set_speed(float speed); // [-1.0, +1.0]
void motor_stop(void);
```

**동작**
- `speed > 0`: IN1에 duty, IN2=0
- `speed < 0`: IN2에 duty, IN1=0
- `speed == 0`: stop

---

### 4.2 서보(목/조향) — `servo_control.c/.h`

**역할**
- `SERVO_NECK_TILT`(상하), `SERVO_NECK_PAN`(좌우), `SERVO_STEER`(조향)

**하드웨어/버스**
- I2C: `/dev/i2c-1`
- PCA9685 주소: `0x5f`
- PWM 주파수: **50Hz(서보 표준)**
- 채널 매핑
  - Neck Tilt: ch0
  - Neck Pan:  ch1
  - Steer:     ch2

**API**
```c
int servo_init(void);
int servo_set_angle(servo_id_t id, float angle_deg); // 0~180
```

**각도→PWM 변환**
- `500us ~ 2400us` 범위를 0~180°로 선형 매핑
---

### 4.3 워터 펌프 — `pump_control.c/.h`

**역할**: 펌프 ON/OFF (PCA9685 채널 4를 full duty로 켬)

**하드웨어/버스**
- PCA9685 주소: `0x5F`
- 채널: `PUMP_CHANNEL=4` (서보 포트 4번)

**API**
```c
int  pump_init(void);
void pump_on(void);
void pump_off(void);
```

**구현 주의**
- `pump_control.c`는 `wiringPiI2C`를 사용하고, `motor/servo`는 `i2c-dev(ioctl)`을 사용합니다.

---

### 4.4 열화상 센서 — `thermal_sensor.c/.h` (+ `mlx90641/`)

**역할**: MLX90641(16x12) 프레임을 읽고 요약 정보(최대 온도, 좌표, 평균)를 계산

**하드웨어/버스**
- I2C 주소: `0x33`
- 라이브러리: `mlx90641/MLX90641_API.*` + `mlx90641/MLX90641_I2C_Driver.*`

**API**
```c
int thermal_sensor_init(void);
int thermal_sensor_read(thermal_data_t *out);
```

**출력 데이터(`thermal_data_t`)**
- `frame[12][16]`: 픽셀 온도(°C)
- `T_env`: 프레임 평균 온도
- `T_fire`: 최대 픽셀 온도
- `hot_row/hot_col`: 최대 픽셀 위치
  - 최대 온도가 50°C 미만이면 `(-1, -1)`로 처리

**동작 특징**
- 프레임을 2번 읽어 안정화 시도
- 프레임 read는 최대 약 1초(200 * 5ms) 재시도로 타임아웃

---

### 4.5 초음파 센서 — `ultrasonic.c/.h`

**역할**: HC-SR04 계열 방식(Trig/Echo) 거리(cm) 측정

**하드웨어/핀(기본 예시)**
- `TRIG=BCM 23`, `ECHO=BCM 24` (main.c에서 사용)

**API**
```c
int   ultrasonic_init(ultrasonic_t *sensor, int trig_pin, int echo_pin);
float ultrasonic_read_distance_cm(const ultrasonic_t *sensor, int timeout_usec);
float ultrasonic_read_distance_cm_avg(const ultrasonic_t *sensor, int samples, int timeout_usec);
float ultrasonic_read_distance_cm_dbg(const ultrasonic_t *sensor, int timeout_usec, us_err_t *out_err);
```

**주의**
- 사용 전 `wiringPiSetupGpio()`(BCM 번호) 호출 필수
- `timeout_usec`를 너무 짧게 주면 거리 측정이 자주 실패합니다.

---

## 5) 규칙/컨벤션

### 5.1 초기화 순서(권장)

`main.c` 기준 권장 호출 순서:

1) `wiringPiSetupGpio()` (GPIO 기반 모듈: 초음파)
2) `thermal_sensor_init()`
3) `servo_init()` (PCA9685 주파수 50Hz 세팅)
4) `motor_init()`
5) `pump_init()`

> **현재 main.c는 motor_init → pump_init → servo_init 순서**라서,
> 최종적으로 PCA9685는 `servo_init()`의 **50Hz 설정으로 덮어쓰기** 됩니다.

### 5.2 I2C 공유 규칙

- PCA9685(0x5f)는 **모터/서보/펌프가 공유**합니다.

### 5.3 단위 규칙

- `motor_set_speed(speed)`
  - `speed ∈ [-1, +1]`
  - 상위 레이어의 `lin_vel`은 현재 “속도 비율”로 사용 중
- 초음파는 `cm` 반환, main에서 `m`로 변환(`dist_cm/100`)

### 5.4 빌드 산출물 관리

- `tests/`에 있는 `test_thermal`, `test_ultrasonic` 같은 실행파일은 **빌드 산출물**입니다.

---

## 6) 제거/변경된 기능(정리)

프로젝트 진행 중 인터페이스/구현이 아래처럼 정리(단순화)되었습니다.

- (기획/초기) `motor_set_velocity(lin, ang)` 형태의 차체 제어
  - → (현재) **Ackermann 방식**: `lin_vel → motor_set_speed(lin)`, `ang_vel → 조향 서보 각도`로 분리

- (기획/초기) 펌프 `PWM duty(0~100%)` / 레벨 제어
  - → (현재) `pump_on()/pump_off()`의 ON/OFF만 제공

---

## 7) 의존성(라즈베리파이 기준)

### 런타임/커널 설정
- I2C 활성화 (`raspi-config` → Interface Options → I2C)
- `/dev/i2c-1` 존재 확인

### 패키지
```bash
sudo apt-get update
sudo apt-get install -y build-essential i2c-tools

# wiringPi
sudo apt-get install -y wiringpi
```

### 디바이스 탐색
```bash
sudo i2cdetect -y 1
# 기대: 0x33(MLX90641), 0x5f(PCA9685)
```

---

## 8) 빌드/테스트

> 현재 `scripts/build.sh`가 비어 있으므로, 아래는 **직접 gcc로 빌드하는 예시**입니다.
> (프로젝트 루트: `fire-drone-car/`에서 실행)

### 8.1 전체 통합 실행파일 빌드(예시)
```bash
cd fire-drone-car

gcc -O2 -Wall -Wextra -pthread \
  -I./src/common -I./src/algo -I./src/comm_ui -I./src/embedded -I./src/embedded/mlx90641 \
  -o fire_drone_car \
  ./src/main/main.c \
  ./src/algo/shared_state.c ./src/algo/state_machine.c ./src/algo/navigation.c ./src/algo/extinguish_logic.c \
  ./src/comm_ui/comm.c ./src/comm_ui/protocol.c \
  ./src/embedded/motor_control.c ./src/embedded/servo_control.c ./src/embedded/pump_control.c \
  ./src/embedded/ultrasonic.c ./src/embedded/thermal_sensor.c \
  ./src/embedded/mlx90641/MLX90641_API.c ./src/embedded/mlx90641/MLX90641_I2C_Driver.c \
  -lwiringPi -lm
```

실행:
```bash
sudo ./fire_drone_car
```
# `tests/` — Embedded 단위 테스트 README

`tests/` 폴더는 **센서/구동기(embedded) 모듈을 하드웨어에 직접 붙여서 확인하는** 목적의 단위 테스트 모음입니다.

---

## 1) 테스트 목록

| 파일 | 대상 | 설명 |
|---|---|---|
| `test_thermal.c` | MLX90641 | 프레임 읽고 `T_env / T_fire / hot(row,col)` 출력 |
| `test_ultrasonic.c` | Ultrasonic | TRIG/ECHO 측정값 출력(에러 원인 디버그 포함) |
| `test_pump.c` | Pump | 콘솔 입력(0=ON, 1=OFF)으로 펌프 수동 토글 |
| `test_neck.c` | Servo(Neck) | 목 팬/틸트 반복 스윕 |
| `test_motor_angle.c` | Servo(Steer) | 조향 서보 0/90/180 이동 |
| `test_steer.c` | DC Motor | 속도 램프(전진/정지/후진) |

---

## 2) 실행 전 공통 준비

### 하드웨어/OS
- 라즈베리파이에서만 동작(실제 I2C/GPIO 필요)
- I2C 활성화: `raspi-config` → Interface Options → I2C
- 주소 확인: `sudo i2cdetect -y 1`  
  - 기대: `0x33`(MLX90641), `0x5f`(PCA9685)

### 패키지
```bash
sudo apt-get update
sudo apt-get install -y build-essential i2c-tools
sudo apt-get install -y wiringpi
```

---

## 3) 빌드/실행 (gcc 예시)

> 프로젝트 루트: `fire-drone-car/`에서 실행

### 3.1 열화상 센서
```bash
cd fire-drone-car

gcc -O2 -Wall -Wextra \
  -I./src/embedded -I./src/embedded/mlx90641 \
  -o ./tests/test_thermal_new \
  ./tests/test_thermal.c \
  ./src/embedded/thermal_sensor.c \
  ./src/embedded/mlx90641/MLX90641_API.c ./src/embedded/mlx90641/MLX90641_I2C_Driver.c \
  -lm

sudo ./tests/test_thermal_new
```

### 3.2 초음파 센서
```bash
cd fire-drone-car

gcc -O2 -Wall -Wextra \
  -I./src/embedded \
  -o ./tests/test_ultrasonic_new \
  ./tests/test_ultrasonic.c ./src/embedded/ultrasonic.c \
  -lwiringPi

sudo ./tests/test_ultrasonic_new
```

### 3.3 펌프
```bash
cd fire-drone-car

gcc -O2 -Wall -Wextra \
  -I./src/embedded \
  -o ./tests/test_pump_new \
  ./tests/test_pump.c ./src/embedded/pump_control.c \
  -lwiringPi

sudo ./tests/test_pump_new
```

---

## 4) include 경로 정리

현재 repo 구조는 `tests/`와 `src/embedded/`가 형제 관계입니다.

```text
fire-drone-car/
├─ src/embedded/...
└─ tests/...
```

## 5) 안전/주의사항

- DC 모터 테스트는 차체를 들어 바퀴를 공중에 띄운 상태에서 진행.
- I2C/GPIO 권한 문제를 피하려면 테스트는 우선 `sudo`로 실행.

## 6) 자주 발생하는 문제

### `i2cdetect`에서 0x33/0x5f가 안 보임
- 배선/전원/ADDR 핀 확인
- I2C 활성화 여부 확인
- 다른 장치가 같은 주소를 점유하지 않는지 확인

### 초음파가 항상 timeout
- ECHO가 3.3V 레벨인지 확인
- TRIG/ECHO 핀 번호가 BCM 기준인지 확인 (`wiringPiSetupGpio()` 사용)
