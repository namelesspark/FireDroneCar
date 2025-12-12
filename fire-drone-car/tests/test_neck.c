// src/tests/test_neck.c

#include <stdio.h>
#include <unistd.h>   // sleep, usleep
#include "../embedded/servo_control.h"

static void move_neck_once(void)
{
    // 각도는 하드웨어에 맞게 조절 가능 (충돌 안 나게 주의!)
    float pan_center  = 90.0f;
    float pan_left    = 45.0f;
    float pan_right   = 135.0f;

    float tilt_center = 90.0f;
    float tilt_up     = 60.0f;
    float tilt_down   = 120.0f;

    printf("[test_neck] center...\n");
    servo_set_angle(SERVO_NECK_PAN,  pan_center);
    servo_set_angle(SERVO_NECK_TILT, tilt_center);
    sleep(1);

    printf("[test_neck] pan left...\n");
    servo_set_angle(SERVO_NECK_PAN, pan_left);
    sleep(1);

    printf("[test_neck] pan right...\n");
    servo_set_angle(SERVO_NECK_PAN, pan_right);
    sleep(1);

    printf("[test_neck] pan center...\n");
    servo_set_angle(SERVO_NECK_PAN, pan_center);
    sleep(1);

    printf("[test_neck] tilt up...\n");
    servo_set_angle(SERVO_NECK_TILT, tilt_up);
    sleep(1);

    printf("[test_neck] tilt down...\n");
    servo_set_angle(SERVO_NECK_TILT, tilt_down);
    sleep(1);

    printf("[test_neck] tilt center...\n");
    servo_set_angle(SERVO_NECK_TILT, tilt_center);
    sleep(1);
}

int main(void)
{
    printf("==== test_neck: neck pan/tilt servo test ====\n");

    if (servo_init() != 0) {
        fprintf(stderr, "[test_neck] servo_init failed\n");
        return 1;
    }

    // 초기 자세: 정면, 수평
    servo_set_angle(SERVO_NECK_PAN,  90.0f);
    servo_set_angle(SERVO_NECK_TILT, 90.0f);
    sleep(1);

    while (1) {
        move_neck_once();
        printf("[test_neck] loop done. Waiting 2s...\n\n");
        sleep(2);
    }

    return 0;
}
