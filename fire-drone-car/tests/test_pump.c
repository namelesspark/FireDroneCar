#include <stdio.h>
#include <unistd.h>
#include <signal.h>

#include "../src/embedded/pump_control.h"

// Ctrl+C 핸들러
void signal_handler(int sig) {
    (void)sig;
    printf("\n\n종료 중... 펌프 OFF\n");
    pump_off();
    _exit(0);
}

int main(void)
{
    int i;
    printf("Testing water pump...\n");

    // Ctrl+C 시그널 핸들러 등록
    signal(SIGINT, signal_handler);

    if (pump_init() != 0) {
        printf("pump_init failed\n");
        return 1;
    }

    while(1){
        printf("\n입력 (0=ON, 1=OFF): ");
        scanf("%d", &i);
        if (i == 0)
            pump_on();
        else
            pump_off();
    }
    
    printf("Test complete\n");
    return 0;
}