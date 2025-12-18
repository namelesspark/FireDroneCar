// tests/test_rear_motor.c
#include <stdio.h>
#include <unistd.h>   // sleep, usleep
#include "../embedded/motor_control.h"

static void ramp_forward(float start, float end, float step, int step_ms)
{
    for (float s = start; s <= end; s += step) {
        printf("[test_motor] forward speed=%.2f\n", s);
        motor_set_speed(s);
        usleep(step_ms * 1000);
    }
}

static void ramp_backward(float start, float end, float step, int step_ms)
{
    for (float s = start; s >= end; s -= step) {
        printf("[test_motor] backward speed=%.2f\n", s);
        motor_set_speed(s);
        usleep(step_ms * 1000);
    }
}

int main(void)
{
    printf("==== test_rear_motor: DC motor only (M1 assumed) ====\n");
    printf("⚠ 바퀴가 굴러가니, 로봇을 들어서 바퀴를 공중에 띄우고 테스트하세요!\n");

    if (motor_init() != 0) {
        fprintf(stderr, "[test_motor] motor_init failed\n");
        return 1;
    }

    ramp_forward(0.0f, 1.0f, 0.1f, 100);
    sleep(1);

    // 2-1) 풀 스피드 유지
    printf("[test_motor] forward 1.00 for 2s\n");
    motor_set_speed(1.0f);
    sleep(2);

    // 정지
    printf("[test_motor] stop\n");
    motor_stop();
    sleep(1);

    // 후진도 풀 스피드로
    printf("[test_motor] backward -1.00 for 2s\n");
    motor_set_speed(-1.0f);
    sleep(2);

    printf("[test_motor] stop\n");
    motor_stop();
    sleep(1);
}
