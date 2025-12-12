#include <stdio.h>
#include <unistd.h>
#include "../src/embedded/pump_control.h"

int main(void)
{
    if (pump_init() != 0) {
        printf("pump_init failed\n");
        return 1;
    }

    printf("ON 2s\n");
    pump_on();
    sleep(2);

    printf("OFF 2s\n");
    pump_off();
    sleep(2);

    printf("ON 2s\n");
    pump_on();
    sleep(2);

    printf("OFF 2s\n");
    pump_off();
    sleep(2);

    printf("ON 2s\n");
    pump_on();
    sleep(2);

    printf("OFF 2s\n");
    pump_off();
    sleep(2);

    printf("Done\n");
    pump_off();
    return 0;
}
