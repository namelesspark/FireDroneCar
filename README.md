# comm_ui/ 모듈 README

FireDroneCar 프로젝트의 통신 및 원격 UI 모듈입니다.  
라즈베리파이와 PC 간 TCP/IP 소켓 통신을 담당하며, 실시간 상태 모니터링과 원격 제어를 제공합니다.

---

## 파일 구조

```
comm_ui/
├── comm_ui.h           # comm_ctx_t 구조체 정의, comm_thread 선언
├── comm.c              # TCP 서버 스레드 구현 (상태 전송 + 명령 수신)
├── protocol.h / .c     # (미구현) 프로토콜 파싱 유틸리티
├── ui_console.c        # (미구현) 로컬 콘솔 UI
└── ui_remote.py        # PC용 원격 UI (Python curses 기반)
```

---

## 핵심 설계 원칙

### 1. 단일 클라이언트 TCP 서버

```c
// main.c에서 comm_thread 시작
comm_ctx_t comm_ctx = {
    .state = &g_state,
    .state_mutex = &g_state.mutex,
    .shutdown_flag = &g_running
};
pthread_create(&comm_tid, NULL, comm_thread, &comm_ctx);
```

**특징:**
- 포트 5000에서 대기
- 한 번에 하나의 클라이언트만 접속 허용
- 새 클라이언트 접속 시 기존 연결 자동 해제

### 2. 양방향 통신

```
┌─────────────┐                      ┌──────────────┐
│ 라즈베리파이 │                      │ PC (Python)  │
│ (comm.c)    │                      │ (ui_remote.py)│
└──────┬──────┘                      └──────┬───────┘
       │                                     │
       │  상태 전송 (200ms마다)              │
       │ ──────────────────────────────────► │
       │  MODE=SEARCH;T_FIRE=85.3;...        │
       │                                     │
       │  명령 수신 (언제든지)                │
       │ ◄────────────────────────────────── │
       │  START / STOP / ESTOP / ...         │
       │                                     │
```

---

## 통신 프로토콜

### 상태 메시지 (라즈베리파이 → PC)

**포맷:** 세미콜론으로 구분된 키=값 쌍, 개행 문자로 종료

```
MODE=<모드>;T_FIRE=<온도>;DT=<온도차>;D=<거리>;HOT=<행>,<열>;ESTOP=<0/1>\n
```

**예시:**
```
MODE=SEARCH;T_FIRE=85.3;DT=22.1;D=1.25;HOT=4,7;ESTOP=0
MODE=APPROACH;T_FIRE=120.5;DT=45.2;D=0.48;HOT=6,8;ESTOP=0
MODE=EXT;T_FIRE=130.2;DT=50.0;D=0.35;HOT=6,9;ESTOP=0
MODE=SAFE;T_FIRE=45.0;DT=5.0;D=0.50;HOT=0,0;ESTOP=1
```

**필드 설명:**

| 필드 | 설명 | 예시 값 |
|------|------|---------|
| MODE | 현재 로봇 상태 | IDLE, SEARCH, DETECT, APPROACH, EXT, SAFE |
| T_FIRE | 최고 온도 (°C) | 85.3 |
| DT | 온도 차이 (°C) | 22.1 |
| D | 초음파 거리 (m) | 1.25 |
| HOT | 열원 위치 (행,열) | 4,7 (16x12 센서 기준) |
| ESTOP | 긴급 정지 상태 | 0 (OFF) / 1 (ON) |

### 명령 메시지 (PC → 라즈베리파이)

**포맷:** 대소문자 무관 문자열, 개행 문자로 종료

```
<명령>\n
```

**지원 명령:**

| 명령 | 설명 | 동작 |
|------|------|------|
| START | 자율 주행 시작 | cmd_mode = CMD_MODE_START |
| STOP | 일반 정지 | cmd_mode = CMD_MODE_STOP |
| MANUAL | 수동 모드 전환 | cmd_mode = CMD_MODE_MANUAL |
| ESTOP / E-STOP | 긴급 정지 | emergency_stop = true |
| CLEAR_ESTOP / CESTOP | 긴급 정지 해제 | emergency_stop = false, 안전하게 IDLE로 복귀 |
| EXIT / QUIT | 프로그램 종료 | shutdown_flag = false |

**명령 처리 규칙:**
- 모든 명령은 `shared_state_t`의 mutex로 동기화
- 알 수 없는 명령은 무시 (에러 메시지 없음)
- 공백과 대소문자는 무시됨

---

## 모듈별 상세

### comm.c - TCP 서버 스레드

**주요 함수:**

```c
void *comm_thread(void *arg);  // 메인 스레드 함수
```

**동작 흐름:**

```
1. 서버 소켓 생성 (포트 5000)
   └─ comm_setup_server_socket()

2. select() 루프 시작
   ├─ 새 클라이언트 accept
   │  └─ 기존 클라이언트 있으면 close 후 교체
   │
   ├─ 클라이언트로부터 명령 수신
   │  ├─ '\n' 단위로 파싱
   │  └─ comm_handle_command_line() 호출
   │
   └─ 200ms마다 상태 전송
      ├─ shared_state_t 스냅샷 생성 (mutex 사용)
      ├─ comm_format_status_line() 호출
      └─ send()

3. 종료 시 소켓 정리
```

**중요 상수:**

```c
#define COMM_PORT 5000                  // 서버 포트
#define COMM_STATUS_INTERVAL_MS 200     // 상태 전송 주기 (ms)
#define COMM_RECV_BUF_SIZE 1024         // 수신 버퍼 크기
```

**안전성 보장:**

```c
// mutex 복사를 피하기 위한 스냅샷 방식
shared_state_t snapshot = {0};
pthread_mutex_lock(ctx->state_mutex);
snapshot.mode = ctx->state->mode;
snapshot.t_fire = ctx->state->t_fire;
// ... (필요한 필드만 복사)
pthread_mutex_unlock(ctx->state_mutex);
```

### ui_remote.py - PC용 원격 UI

**실행 방법:**

```bash
# Windows에서 curses 설치 필요
pip install windows-curses

# 실행
python ui_remote.py --host <라즈베리파이_IP> --port 5000
```

**화면 구성:**

```
┌────────────────────────────────────────────────────────┐
│ FireDroneCar Remote UI  |  172.30.121.144:5000         │
├────────────────────────────────────────────────────────┤
│ MODE : SEARCH                                          │
│ T_FIRE :   85.3 °C    ΔT :   22.1 °C                  │
│ DIST  :   1.25 m     HOT : (4, 7)                     │
│ ESTOP : OFF                                            │
│                                                        │
│ RAW : MODE=SEARCH;T_FIRE=85.3;DT=22.1;D=1.25;...     │
├────────────────────────────────────────────────────────┤
│ 명령 예시: START / STOP / ESTOP / CLEAR_ESTOP / EXIT   │
│ cmd> _                                                 │
└────────────────────────────────────────────────────────┘
```

**주요 기능:**

1. **실시간 대시보드**: 
   - 상태 자동 갱신 (200ms마다)
   - 현재 모드, 온도, 거리, 열원 위치 표시
   
2. **명령 입력**:
   - 하단 `cmd>` 프롬프트에서 입력
   - Enter 키로 전송
   - 화면 깨짐 없이 입력 가능 (curses 기반)

3. **스레드 구조**:
   - 메인 스레드: curses UI 렌더링
   - 백그라운드 스레드: 소켓 수신 (recv_loop)

---

## 데이터 흐름

```
┌─────────────┐                  ┌─────────────┐
│sensor_thread│                  │ algo_thread │
│  (main.c)   │                  │  (main.c)   │
└──────┬──────┘                  └──────┬──────┘
       │ 쓰기                            │ 읽기/쓰기
       │ t_fire, dT                      │ mode
       │ distance                        │ lin_vel, ang_vel
       │ hot_row, hot_col                │
       │                                 │
       ▼                                 ▼
┌──────────────────────────────────────────────┐
│           shared_state_t                      │
│         (mutex로 동기화)                       │
└──────────────┬───────────────────────────────┘
               │ 읽기                    ▲ 쓰기
               │ (스냅샷)               │ (명령)
               ▼                         │
┌──────────────────────────────────────────────┐
│           comm_thread                         │
│  ┌──────────────┐      ┌──────────────┐     │
│  │ 상태 전송     │      │ 명령 수신     │     │
│  │ (200ms마다)  │      │ (언제든지)    │     │
│  └──────┬───────┘      └───────▲──────┘     │
└─────────┼──────────────────────┼────────────┘
          │                      │
          │ TCP/IP Socket        │
          │ (포트 5000)          │
          ▼                      │
┌──────────────────────────────────────────────┐
│         ui_remote.py (PC)                    │
│  ┌──────────────┐      ┌──────────────┐     │
│  │ 상태 표시     │      │ 명령 입력     │     │
│  │ (대시보드)    │      │ (cmd>)       │     │
│  └──────────────┘      └──────────────┘     │
└──────────────────────────────────────────────┘
```

---

## 스레드 안전성

| 스레드 | Lock 규칙 | 접근 필드 |
|--------|----------|----------|
| comm_thread | 상태 읽기: lock → 스냅샷 복사 → unlock | mode, t_fire, dT, distance, hot_row/col, emergency_stop |
| | 명령 쓰기: lock → 수정 → unlock | cmd_mode, emergency_stop |
| sensor_thread | lock → 센서 데이터 쓰기 → unlock | t_fire, dT, distance, hot_row, hot_col |
| algo_thread | main에서 lock/unlock 관리 | mode, lin_vel, ang_vel, water_level |
| motor_thread | lock → 모터 명령 읽기 → unlock | lin_vel, ang_vel |

**주의사항:**
- `shared_state_t`에 `pthread_mutex_t`가 포함되어 있으므로 **구조체 전체 복사 금지**
- 필요한 필드만 스냅샷으로 복사하여 사용

---

## 사용 예시

### 1. 기본 사용 (자율 주행)

```bash
# 라즈베리파이에서
sudo ./fire_drone_car

# PC에서 (다른 터미널)
python ui_remote.py --host 172.30.121.144 --port 5000

# ui_remote.py에서
cmd> START      # 자율 주행 시작
cmd> STOP       # 정지
cmd> EXIT       # 종료
```

### 2. 긴급 정지 시나리오

```bash
cmd> START              # 자율 주행 중
# (상황 발생)
cmd> ESTOP              # 즉시 정지, MODE=SAFE로 전환
# (상황 해결)
cmd> CLEAR_ESTOP        # 긴급 정지 해제, MODE=IDLE로 복귀
cmd> START              # 재시작
```

### 3. 디버깅 모드

```bash
# 실시간으로 상태 확인하면서 명령 테스트
cmd> START
# 화면에서 MODE=SEARCH → DETECT → APPROACH → EXT 전환 확인
# T_FIRE, DT, DIST 값 변화 모니터링
cmd> STOP
```

---

## 미구현 기능

현재 프로젝트에서 다음 파일들은 생성되어 있지만 구현되지 않았습니다:

- `protocol.h / .c` - 프로토콜 파싱 유틸리티 분리
- `ui_console.c` - 라즈베리파이 로컬 콘솔 UI

필요 시 향후 구현 가능합니다.

---

## 의존성

```
comm_ui/
├── depends on: ../common/common.h
│   └─ shared_state_t, robot_mode_t, cmd_mode_t
│
└── depends on: POSIX 표준 라이브러리
    ├─ <pthread.h>
    ├─ <sys/socket.h>
    └─ <arpa/inet.h>
```

---

## 빌드 & 테스트

### 라즈베리파이

```bash
# 전체 빌드
cd fire-drone-car
make clean && make

# 실행 (root 권한 필요)
sudo ./fire_drone_car
```

### PC (Python UI)

```bash
# Windows
pip install windows-curses
python ui_remote.py --host <라즈베리파이_IP> --port 5000

# Linux/Mac
python3 ui_remote.py --host <라즈베리파이_IP> --port 5000
```

### 통신 테스트

```bash
# 간단한 telnet 테스트
telnet <라즈베리파이_IP> 5000
# 명령어 입력:
START
ESTOP
CLEAR_ESTOP
EXIT
```

---

## 트러블슈팅

### 1. 연결 실패 (`Connection refused`)

**원인:**
- 라즈베리파이에서 프로그램이 실행되지 않음
- 방화벽이 포트 5000을 차단
- IP 주소가 잘못됨

**해결:**
```bash
# 라즈베리파이에서 프로그램 실행 확인
ps aux | grep fire_drone_car

# 포트 5000이 열려있는지 확인
netstat -tuln | grep 5000

# IP 주소 확인
hostname -I
```

### 2. 상태가 업데이트되지 않음

**원인:**
- sensor_thread가 동작하지 않음
- shared_state_t 업데이트 실패

**해결:**
```bash
# 로그 확인
tail -f /var/log/fire_drone_car.log

# sensor_thread 시작 메시지 확인
# "[sensor_thread] STARTED" 출력 확인
```

### 3. 명령이 동작하지 않음

**원인:**
- algo_thread에서 cmd_mode를 무시
- mutex deadlock

**해결:**
- `[comm] recv cmd: 'XXX'` 로그 확인
- state_machine.c에서 cmd_mode 처리 로직 확인

---

## 향후 개선 방향

1. **프로토콜 확장**
   - JSON 기반 프로토콜로 전환
   - 바이너리 프로토콜로 효율성 향상

2. **다중 클라이언트 지원**
   - 모니터링 전용 클라이언트 추가
   - 제어 권한 관리

3. **웹 기반 UI**
   - Flask/FastAPI 서버 추가
   - 브라우저 기반 대시보드

4. **로깅 및 재생**
   - 상태 히스토리 기록
   - 오프라인 재생 기능

---

## 주의사항

1. **mutex 복사 금지** - `shared_state_t` 전체 복사 시 UB 발생
2. **단일 클라이언트** - 동시에 두 개 이상의 클라이언트 연결 불가
3. **네트워크 보안** - 외부 네트워크에서 접근 시 보안 고려 필요
4. **실시간성** - 200ms 주기로 상태 전송, 명령 응답 지연 가능