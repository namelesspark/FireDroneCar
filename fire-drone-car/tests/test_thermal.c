// tests/test_thermal_sensor.c

#include <stdio.h>
#include <unistd.h>   // sleep
#include "../src/embedded/thermal_sensor.h"

int main(void)
{
    printf("MLX90641 thermal sensor test\n");

    if (thermal_sensor_init() != 0) {
        printf("thermal_sensor_init failed\n");
        return 1;
    }

    printf("thermal_sensor_init OK. Reading frames...\n");
    printf("Press Ctrl+C to stop.\n\n");

    while (1) {
        thermal_data_t data;
        int ret = thermal_sensor_read(&data);
        if (ret != 0) {
            printf("thermal_sensor_read failed: %d\n", ret);
            sleep(1);
            continue;
        }

        printf("T_env=%.2f °C,  T_fire=%.2f °C  (hot at row=%d, col=%d)\n",
               data.T_env, data.T_fire, data.hot_row, data.hot_col);

        // 중앙 픽셀 온도 같은 것도 보고 싶으면:
        int center_row = THERMAL_ROWS / 2;
        int center_col = THERMAL_COLS / 2;
        printf("  center[%d,%d] = %.2f °C\n\n",
               center_row, center_col, data.frame[center_row][center_col]);

        usleep(250000); // 0.25초마다 한 프레임
    }

    return 0;
}
