#include <stdio.h>
#include <wiringPi.h>
#include "../src/embedded/ultrasonic.h"

#define US_TRIG_PIN 23
#define US_ECHO_PIN 24


int main(void)
{
    if (wiringPiSetupGpio() == -1) {
        printf("wiringPi setup failed\n");
        return 1;
    }

    ultrasonic_t us;

    if (ultrasonic_init(&us, US_TRIG_PIN, US_ECHO_PIN) != 0) {
        printf("ultrasonic_init failed\n");
        return 1;
    }

    printf("초음파 테스트를 시작하겠습니다 (TRIG=%d, ECHO=%d)\n",
           US_TRIG_PIN, US_ECHO_PIN);

    while (1) {
        us_err_t err;
        float d = ultrasonic_read_distance_cm_dbg(&us, 60000, &err);
        //float distance_cm = ultrasonic_read_distance_cm_avg(&us, 5, 60000);
        /*
        if (distance_cm < 0.0f) {
            printf("distance: error or timeout\n");
        } else {
            printf("거리: %.2f cm\n", distance_cm);
        }
        */
        if (d < 0.0f) {
            if (err == US_ERR_WAIT_ECHO_HIGH_TIMEOUT) printf("ECHO never went HIGH (stuck LOW)\n");
            else if (err == US_ERR_WAIT_ECHO_LOW_TIMEOUT) printf("ECHO never went LOW (stuck HIGH)\n");
            else printf("unknown error %d\n", err);
        } else {
            printf("거리: %.2f cm\n", d);
        }


        delay(200);
    }

    return 0;
}