#pragma once

#include <stdint.h>

/**
 * PCA9685 기반 서보 3개 제어
 *  - SERVO_NECK_PAN   : 목 좌우
 *  - SERVO_NECK_TILT  : 목 위아래
 *  - SERVO_STEER      : 바퀴 좌우 조향
 */

typedef enum {
    SERVO_NECK_TILT = 0,
    SERVO_NECK_PAN = 1,
    SERVO_STEER = 2,
    SERVO_COUNT = 3       // 서보 개수
} servo_id_t;

// 채널 매핑 (내부용, servo_control.c에서 사용)
#define SERVO_CH_NECK_TILT  0
#define SERVO_CH_NECK_PAN   1
#define SERVO_CH_STEER      2

// 초기화 (PCA9685 @ /dev/i2c-1, addr 0x5f, 50Hz)
int servo_init(void);

// 특정 서보에 각도 설정 (deg)
int servo_set_angle(servo_id_t id, float angle_deg);