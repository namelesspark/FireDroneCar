#include <stdio.h>
#include <unistd.h>

#include "../src/embedded/pump_control.h"

int main(void)
{
    printf("Testing water pump...\n");

    if (pump_init() != 0) {
        printf("pump_init failed\n");
        return 1;
    }

    pump_on();
    sleep(3);

    pump_off();

    printf("Test complete\n");
    return 0;
}
