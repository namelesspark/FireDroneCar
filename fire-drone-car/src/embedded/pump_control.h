#pragma once

#include <stdbool.h>

#define PUMP_GPIO 18

#define PUMP_ACTIVE_HIGH 1

#define PUMP_MAX_LEVEL 5

int  pump_init(void);

// 펌프 ON/OFF
void pump_on(void);
void pump_off(void);

void pump_set_level(int level);

void pump_apply(bool emergency_stop, int water_level);
