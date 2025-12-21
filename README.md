# Fire Detection and Suppression Robot

## 프로젝트 개요
연기가 자욱한 환경에서 센서 기반 자율 주행으로 화재를 탐지하고 진압하는 지상 로봇 프로토타입

## 팀원 및 역할 분담
- **박민호**: 알고리즘 / 상태머신 / 네비게이션 로직 (algo/)
- **김효빈**: 임베디드 제어 / 센서 드라이버 / 모터 제어 (embedded/)
- **노경민**: 통신 프로토콜 / UI / 원격 모니터링 (comm_ui/)
- **이상엽**: 하드웨어 설계 / 회로 / 기구 설계 (hardware/)

## 주요 기능
- 멀티스레드 기반 실시간 제어 (4개 스레드)
- 열화상 센서 기반 화재 탐지 (MLX90641)
- 초음파 센서 기반 장애물 회피
- 단계적 소화 알고리즘 (레벨 1~5)
- TCP/UDP 기반 원격 모니터링
- Mutex 기반 스레드 동기화

## 시스템 구조
```
SEARCH → APPROACH → EXTINGUISH → (SUCCESS/FAIL)
```

## 폴더 구조
```
fire-drone-car/
├── docs/              # 설계 문서, 보고서
│   └── notes/        # 회의록
├── hardware/          # 회로도, 핀맵, BOM
│   ├── schematics/   # 회로도
│   ├── pinmap/       # 핀맵
│   └── cad/          # 3D 모델
├── src/
│   ├── main/         # 메인 엔트리
│   ├── algo/         # 알고리즘 (박민호)
│   ├── embedded/     # 임베디드 제어 (김효빈)
│   ├── comm_ui/      # 통신 UI (노경민)
│   └── common/       # 공통 설정
├── scripts/          # 빌드/실행 스크립트
└── tests/            # 단위 테스트
```

## 빌드 및 실행
```bash
cd scripts
chmod +x build.sh run.sh
./build.sh
./run.sh
```

## 개발 가이드

### 1. 환경 설정
- Raspberry Pi 4/5
- WiringPi 라이브러리
- pthread (멀티스레드)
- I2C 활성화

### 2. 각 담당자 작업 영역
- **박민호**: `src/algo/` - 상태머신, 네비게이션, 소화 로직
- **김효빈**: `src/embedded/` - 센서/모터 드라이버
- **노경민**: `src/comm_ui/` - TCP/UDP 통신, UI
- **이상엽**: `hardware/` - 회로도, BOM, 기구 설계

### 3. 개발 순서
1. Week 1: 설계 및 인터페이스 정의
2. Week 2: 각 모듈 구현 및 단위 테스트
3. Week 3: 통합 테스트
4. Week 4: 최종 조정 및 발표 준비

## 프로젝트 기간
4주 (2024년 12월)



## 하드웨어 구성도
<img width="7631" height="5455" alt="Mermaid Chart - Create complex, visual diagrams with text -2025-12-20-184915" src="https://github.com/user-attachments/assets/e3880e8e-c8db-4763-af69-72b2427e6549" />

## 시스템 구성
2.1 하드웨어 구성
본 로봇은 Raspberry Pi 5를 메인 제어기로 채택하여 고성능 연산을 수행하며, Adeept Robot HAT V3.1을 통해 전력 분배 및 액추에이터 제어를 통합 관리하는 구조로 설계되었습니다.

가. 제어 보드 및 전원 시스템

메인 컨트롤러 (Main Controller): Raspberry Pi 5를 사용하여 알고리즘 연산, 센서 데이터 처리 및 멀티스레드 제어를 수행합니다.
확장 보드 (Expansion Board): Adeept Robot HAT V3.1을 장착하여 모터 제어 및 전원 관리 효율을 높였습니다.
전원부 (Power System): $7.4V$, $4A$ 사양의 18650 리튬이온 배터리 2개를 직렬 연결하여 사용합니다.
배터리 전원은 Adeept HAT의 Vin 단자로 입력됩니다.
HAT의 40-Pin Header를 통해 Raspberry Pi 5에 안정적인 $5V$ 전원을 공급합니다.

나. 센서부 (Sensors)

화재 탐지 및 자율 주행 환경 인지를 위해 다음과 같은 센서 인터페이스를 구축하였습니다.

| 구분 | 부품명 | 연결 인터페이스 | 주요 역할 |
| :--- | :--- | :--- | :--- |
| **열화상 센서** | MLX90641 | I2C (Port X1, `0x33`) | 화점 탐지 및 온도 모니터링 |
| **초음파 센서** | HC-SR04 | GPIO 23 (TRIG) / 24 (ECHO) | 장애물 거리 측정 및 회피 |

다. 구동부 및 액추에이터 (Actuators)

-- PCA9685 PWM Driver (I2C 주소: $0x5F$F를 통해 총 5개의 액추에이터를 정밀하게 제어합니다.
- 주행 시스템:DC Motor (후륜 구동): Motor M1 채널에 연결되어 로봇의 이동을 담당합니다.
- Servo Motor (조향): Servo Ch2 채널을 사용하여 전륜 조향을 수행합니다.
-- 상부 가동부 (Neck System):
- Servo Motor (목 상하): Servo Ch0를 통해 센서 및 소화 노즐의 틸트(Tilt) 각도를 조절합니다.
- Servo Motor (목 좌우): Servo Ch1을 통해 센서 및 소화 노즐의 팬(Pan) 각도를 조절합니다.
소화 시스템:Water Pump: Servo Ch4 채널에 연결되어 화재 탐지 시 소화액을 분사합니다.

라. 인터페이스 및 통신 구조
- I2C 통신: Raspberry Pi 5의 /dev/i2c-1 버스를 공유하며, PWM 드라이버($0x5F$)와 열화상 센서($0x33$)가 각각 독립적인 주소로 통신합니다.
- GPIO 제어: 초음파 센서는 HAT을 거치지 않고 Raspberry Pi의 40-Pin GPIO에 직접 연결되어 실시간성을 확보합니다.
