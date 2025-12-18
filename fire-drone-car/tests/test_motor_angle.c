#include <stdio.h>
#include <unistd.h>   // sleep, usleep
#include "/home/pi/firedrone/FireDroneCar/fire-drone-car/src/embedded/servo_control.h"

static void steer_only_test_0_90_180(void)
{
    const float steer_center = 90.0f;
    const float steer_left  = 180.0f;
    const float steer_right  = 0.0f;

    printf("바퀴를 정면!!!!!!!!\n");
    servo_set_angle(SERVO_STEER, steer_center);
    sleep(1);

    printf("바퀴를 왼쪽!!\n");
    servo_set_angle(SERVO_STEER, steer_left);
    sleep(1);

    printf("바퀴를 정면!!!!!!!!\n");
    servo_set_angle(SERVO_STEER, steer_center);
    sleep(1);

    printf("바퀴를 오른쪽!!\n");
    servo_set_angle(SERVO_STEER, steer_right);
    sleep(1);

    printf("바퀴를 정면!!!!!!!!\n");
    servo_set_angle(SERVO_STEER, steer_center);
    sleep(1);
}

int main(void)
{
    if (servo_init() != 0) 
    { fprintf(stderr, "[test_steer] servo_init failed\n"); return 1; }

    printf("==== test_steer: steering servo + DC motor test ====\n");

    sleep(1);
    printf("\n/////테스트 시작///////\n");
    steer_only_test_0_90_180();

    return 0;
}
