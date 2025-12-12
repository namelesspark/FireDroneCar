
#include "pump_control.h"

#include <stdio.h>
#include <wiringPi.h>

static int g_inited = 0;

static void write_relay(int on)
{
#if PUMP_ACTIVE_HIGH
    digitalWrite(PUMP_GPIO, on ? HIGH : LOW);
#else
    digitalWrite(PUMP_GPIO, on ? LOW : HIGH);
#endif
}

int pump_init(void)
{
    if (g_inited) return 0;

    if (wiringPiSetupGpio() != 0) {
        fprintf(stderr, "[pump] wiringPiSetupGpio() failed\n");
        return -1;
    }

    pinMode(PUMP_GPIO, OUTPUT);
    write_relay(0);

    g_inited = 1;

    printf("[pump] init OK (GPIO%d, relay %s)\n",
           PUMP_GPIO,
#if PUMP_ACTIVE_HIGH
           "active-high"
#else
           "active-low"
#endif
    );
    return 0;
}

void pump_on(void)
{
    if (!g_inited) return;
    write_relay(1);
}

void pump_off(void)
{
    if (!g_inited) return;
    write_relay(0);
}

void pump_set_level(int level)
{
    if (level <= 0) pump_off();
    else            pump_on();
}

void pump_apply(bool emergency_stop, int water_level)
{
    if (emergency_stop) {
        pump_off();
        return;
    }
    pump_set_level(water_level);
}
