// src/common/common.h
#pragma once

#include <stdbool.h>

// 로봇 동작 모드
typedef enum
{
    MODE_IDLE = 0,   // 정지/대기
    MODE_SEARCH,     // 불 탐색 중
    MODE_APPROACH,   // 열원 접근 중
    MODE_EXTINGUISH, // 진화 중
    MODE_SAFE_STOP   // 비상 정지/안전 정지
} robot_mode_t;

// 외부/사용자 명령 모드
typedef enum
{
    CMD_MODE_NONE = 0, // 명령 없음 (자동 동작)
    CMD_MODE_START,    // 자율 운전 시작
    CMD_MODE_STOP,     // 일반 정지
    CMD_MODE_MANUAL    // 수동 모드 (원격 제어용)
} cmd_mode_t;

// 통신/알고리즘/모터 스레드가 같이 보는 공유 상태 구조체
typedef struct
{
    // 기본 상태
    robot_mode_t mode;   // 현재 로봇 모드 (SEARCH/APPROACH/EXT/SAFE 등)
    cmd_mode_t cmd_mode; // 외부에서 들어온 명령 상태

    // 진화 관련
    int water_level;           // 물/펌프 강도 단계 (0 ~ N)
    bool water_level_override; // 외부에서 강도 직접 지정했는지 여부

    // 센서 데이터
    float t_fire;   // 열원 온도 추정값 (°C)
    float dT;       // 주변 대비 온도 차이 (°C)
    float distance; // 초음파 거리 (m 또는 cm, 너희 정의대로)

    int hot_row; // 열화상 센서에서 가장 뜨거운 픽셀의 행
    int hot_col; // 열화상 센서에서 가장 뜨거운 픽셀의 열

    // 안전 관련
    bool emergency_stop; // 비상 정지 플래그 (E-STOP)
} shared_state_t;
