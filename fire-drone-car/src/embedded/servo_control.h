#pragma once

#include <stdint.h>

/**
 * PCA9685 기반 서보 3개 제어
 *  - SERVO_NECK_PAN   : 목 좌우
 *  - SERVO_NECK_TILT  : 목 위아래
 *  - SERVO_STEER      : 바퀴 좌우 조향
 */

typedef enum {
    SERVO_NECK_PAN = 0,
    SERVO_NECK_TILT,
    SERVO_STEER,
    SERVO_COUNT
} servo_id_t;

// 초기화 (PCA9685 @ /dev/i2c-1, addr 0x5f, 50Hz)
//  0  성공
// <0 실패
int servo_init(void);

// 특정 서보에 각도 설정 (deg)
//  - 일반적으로 0~180 범위 사용
//  - 바퀴 조향은 예: 45~135 사이만 쓰도록 상위에서 제한 가능
int servo_set_angle(servo_id_t id, float angle_deg);
