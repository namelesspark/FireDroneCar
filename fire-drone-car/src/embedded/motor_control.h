#pragma once

#include <stdint.h>
#include <stdbool.h>

#define PUMP_GPIO      18      // 나중에 바꾸고 싶으면 여기만 수정
#define PUMP_PWMCHIP  "0"      // "/sys/class/pwm/pwmchip0"
#define PUMP_PWMCHAN  "0"      // "/sys/class/pwm/pwmchip0/pwm0"
#define PUMP_MAX_LEVEL 5   // 0~5단계로 세기 나눔 (필요하면 늘려도 됨)

int motor_init(void);

void motor_set_speed(float speed);

void motor_stop(void);
