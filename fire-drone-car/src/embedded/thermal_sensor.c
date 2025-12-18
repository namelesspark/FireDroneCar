#include "thermal_sensor.h"

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <unistd.h>  // usleep 사용을 위해 추가

#include "mlx90641/MLX90641_API.h"
#include "mlx90641/MLX90641_I2C_Driver.h"

#define MLX90641_I2C_ADDR  0x33
#define TA_SHIFT 8

static paramsMLX90641 g_mlx_params;
static uint16_t       g_eeprom[832];
static uint16_t       g_frame[242];
static int            g_initialized = 0;

int thermal_sensor_init(void)
{
    int status;

    MLX90641_I2CInit();
    MLX90641_I2CFreqSet(100);

    // EEPROM dump
    status = MLX90641_DumpEE(MLX90641_I2C_ADDR, g_eeprom);
    if (status != 0) {
        fprintf(stderr, "[thermal_sensor] MLX90641_DumpEE failed: %d\n", status);
        return -1;
    }

    // 파라미터 추출
    status = MLX90641_ExtractParameters(g_eeprom, &g_mlx_params);
    if (status != 0) {
        fprintf(stderr, "[thermal_sensor] MLX90641_ExtractParameters failed: %d\n", status);
        return -2;
    }

    // 리프레시 레이트 설정 (예: 4Hz = 0x03, 필요에 따라 변경 가능)
    status = MLX90641_SetRefreshRate(MLX90641_I2C_ADDR, 0x03);
    if (status != 0) {
        fprintf(stderr, "[thermal_sensor] MLX90641_SetRefreshRate failed: %d\n", status);
        return -3;
    }

    g_initialized = 1;
    return 0;
}

int thermal_sensor_read(thermal_data_t *out)
{
    if (!g_initialized) {
        fprintf(stderr, "[thermal_sensor] not initialized\n");
        return -1;
    }
    if (out == NULL) {
        return -2;
    }

    //프레임 두번 읽어서 안정화 
    for (int i = 0; i < 2; ++i) {
            int status = -1;

        // 최대 ~1초(200 * 5ms) 기다리면서 프레임 준비될 때까지 재시도
        for (int tries = 0; tries < 200; ++tries) {
            status = MLX90641_GetFrameData(MLX90641_I2C_ADDR, g_frame);
            if (status == 0) break;
            usleep(5000); // 5ms
        }

        if (status != 0) {
            fprintf(stderr, "[thermal_sensor] GetFrameData timeout/fail: %d\n", status);
            return -3;
        }
    }

    float vdd = MLX90641_GetVdd(g_frame, &g_mlx_params);
    float Ta  = MLX90641_GetTa(g_frame, &g_mlx_params);

    float tr         = Ta - TA_SHIFT; // reflected temperature
    float emissivity = 0.95f;

    // 모든 픽셀 온도를 담을 버퍼
    float to[THERMAL_PIXELS];

    MLX90641_CalculateTo(g_frame, &g_mlx_params,
                         emissivity, tr, to);

    // out 구조체 채우기
    float sum   = 0.0f;
    float t_max = -1000.0f;
    int   max_r = 0;
    int   max_c = 0;

    // MLX90641To 배열 구조:
    // Arduino 코드에서 interpolate_image(MLX90641To, 12, 16, ...)
    // -> rows = 12, cols = 16, 인덱스 = row * 16 + col
    for (int row = 0; row < THERMAL_ROWS; ++row) {
        for (int col = 0; col < THERMAL_COLS; ++col) {
            int idx = row * THERMAL_COLS + col;
            float t = to[idx];

            // 전체 프레임 저장 (센서 계층이 갖고 있고, 위에서 필요하면 사용)
            out->frame[row][col] = t;

            sum += t;

            if (t > t_max) {
                t_max = t;
                max_r = row;
                max_c = col;
            }
        }
    }

    out->T_env   = sum / (float)THERMAL_PIXELS;
    out->T_fire  = t_max;
    if (t_max < 50.0f) {
        out->hot_row = -1;
        out->hot_col = -1;
    } else {
        out->hot_row = max_r;
        out->hot_col = max_c;
    }

    return 0;
}
