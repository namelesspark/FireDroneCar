#ifndef THERMAL_SENSOR_H
#define THERMAL_SENSOR_H

#include <stdint.h>

#define THERMAL_ROWS 12
#define THERMAL_COLS 16
#define THERMAL_PIXELS (THERMAL_ROWS * THERMAL_COLS)

typedef struct {
    float frame[THERMAL_ROWS][THERMAL_COLS];

    float T_env;      // 주변 평균 온도 (°C)
    float T_fire;     // 가장 뜨거운 픽셀 온도 (°C)

    int   hot_row;    // 가장 뜨거운 픽셀의 행 인덱스 [0..THERMAL_ROWS-1]
    int   hot_col;    // 가장 뜨거운 픽셀의 열 인덱스 [0..THERMAL_COLS-1]
} thermal_data_t;

/**
 * MLX90641 센서 초기화
 *
 * - I2C(0x33) 통신 준비
 * - EEPROM dump + 파라미터 추출
 * - 리프레시 레이트 설정(예: 4Hz)
 *
 * @return 0  성공
 *        <0  실패
 *
 * ⚠ MLX90641_API.c / MLX9064X_I2C_Driver.c 가 프로젝트에 함께 빌드되어 있어야 함
 *    (Melexis / Seeed 에서 제공하는 라이브러리)
 */
int thermal_sensor_init(void);

/**
 * MLX90641 한 프레임 읽고 요약 정보 계산
 *
 * @param out  출력 구조체 (NULL 이면 -1 반환)
 * @return 0  성공
 *        <0  실패
 *
 * 성공 시:
 *  - out->frame[row][col] 에 각 픽셀 온도(°C)
 *  - out->T_env  : 프레임 전체의 평균 온도
 *  - out->T_fire : 가장 뜨거운 픽셀 온도
 *  - out->hot_row / hot_col : 그 픽셀 위치
 */
int thermal_sensor_read(thermal_data_t *out);

#endif // THERMAL_SENSOR_H
