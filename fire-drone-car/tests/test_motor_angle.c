#include <stdio.h>
#include <unistd.h>   // sleep, usleep
#include "../embedded/servo_control.h"
#include "../embedded/motor_control.h"

static void steer_only_test_0_90_180(void)
{
    const float steer_center = 90.0f;
    const float steer_left   = 180.0f;
    const float steer_right  = 0.0f;

    printf("[steer] center (%.1f)\n", steer_center);
    servo_set_angle(SERVO_STEER, steer_center);
    sleep(1);

    printf("[steer] left (%.1f)\n", steer_left);
    servo_set_angle(SERVO_STEER, steer_left);
    sleep(1);

    printf("[steer] right (%.1f)\n", steer_right);
    servo_set_angle(SERVO_STEER, steer_right);
    sleep(1);

    printf("[steer] back to center (%.1f)\n", steer_center);
    servo_set_angle(SERVO_STEER, steer_center);
    sleep(1);
}

int main(void)
{
    printf("==== test_steer: steering servo + DC motor test ====\n");

    if (servo_init() != 0) {
        fprintf(stderr, "[test_steer] servo_init failed\n");
        return 1;
    }

    if (motor_init() != 0) {
        fprintf(stderr, "[test_steer] motor_init failed\n");
        return 1;
    }

    // 안전: 모터는 멈춰두고 조향만 테스트
    motor_stop();
    sleep(1);

    printf("\n=== 조향: 가운데(90) -> 왼쪽(180) -> 오른쪽(0) -> 가운데(90) ===\n");
    steer_only_test_0_90_180();

    return 0;
}
