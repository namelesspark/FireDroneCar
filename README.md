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

