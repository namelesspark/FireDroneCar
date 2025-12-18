// src/embedded/servo_control.c
//
// PCA9685 로 서보 3개 제어
//  - neck pan
//  - neck tilt
//  - steering (front wheel)
//
// motor_control.c 와 같은 PCA9685를 공유하므로
//   주파수는 50Hz 로 고정 (서보 기준)
// DC 모터는 50Hz PWM 으로도 충분히 동작 가능.

#include "servo_control.h"

#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <math.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>

#ifndef SERVO_I2C_DEV
#define SERVO_I2C_DEV "/dev/i2c-1"
#endif
#define PCA9685_ADDR       0x5f
#define PCA9685_MODE1      0x00
#define PCA9685_PRESCALE   0xFE
#define PCA9685_LED0_ON_L  0x06

#define PCA9685_FREQ_HZ    50.0f  // 서보 표준

#define SERVO_CH_NECK_PAN   1
#define SERVO_CH_NECK_TILT  0
#define SERVO_CH_STEER      2

// 서보 펄스 폭 설정 (us)
#define SERVO_MIN_PULSE_US  500.0f   // 0도 근처
#define SERVO_MAX_PULSE_US  2400.0f  // 180도 근처

static int g_i2c_fd = -1;

static int i2c_set_slave(uint8_t addr)
{
    if (g_i2c_fd < 0) {
        fprintf(stderr, "[servo] I2C not opened\n");
        return -1;
    }
    if (ioctl(g_i2c_fd, I2C_SLAVE, addr) < 0) {
        perror("[servo] ioctl(I2C_SLAVE)");
        return -1;
    }
    return 0;
}

static int i2c_write_reg8(uint8_t reg, uint8_t val)
{
    uint8_t buf[2] = { reg, val };
    if (i2c_set_slave(PCA9685_ADDR) < 0) return -1;

    if (write(g_i2c_fd, buf, 2) != 2) {
        perror("[servo] i2c_write_reg8");
        return -1;
    }
    return 0;
}

static int i2c_read_reg8(uint8_t reg, uint8_t *out)
{
    if (!out) return -1;
    if (i2c_set_slave(PCA9685_ADDR) < 0) return -1;

    if (write(g_i2c_fd, &reg, 1) != 1) {
        perror("[servo] i2c_read_reg8 write-reg");
        return -1;
    }
    if (read(g_i2c_fd, out, 1) != 1) {
        perror("[servo] i2c_read_reg8 read");
        return -1;
    }
    return 0;
}

static int pca9685_set_pwm(uint8_t channel, uint16_t on, uint16_t off)
{
    if (off > 4095) off = 4095;

    uint8_t reg = PCA9685_LED0_ON_L + 4 * channel;
    uint8_t buf[5];

    buf[0] = reg;
    buf[1] = on & 0xFF;
    buf[2] = (on >> 8) & 0x0F;
    buf[3] = off & 0xFF;
    buf[4] = (off >> 8) & 0x0F;

    if (i2c_set_slave(PCA9685_ADDR) < 0) return -1;
    if (write(g_i2c_fd, buf, sizeof(buf)) != (ssize_t)sizeof(buf)) {
        perror("[servo] pca9685_set_pwm");
        return -1;
    }
    return 0;
}

static int pca9685_set_pwm_freq(float freq_hz)
{
    if (freq_hz <= 0.0f) return -1;

    float prescale_f = 25000000.0f / (4096.0f * freq_hz) - 1.0f;
    uint8_t prescale = (uint8_t)floorf(prescale_f + 0.5f);

    uint8_t oldmode = 0;
    if (i2c_read_reg8(PCA9685_MODE1, &oldmode) < 0) return -1;

    uint8_t sleep_mode = (oldmode & 0x7F) | 0x10;
    if (i2c_write_reg8(PCA9685_MODE1, sleep_mode) < 0) return -1;
    if (i2c_write_reg8(PCA9685_PRESCALE, prescale) < 0) return -1;
    if (i2c_write_reg8(PCA9685_MODE1, oldmode) < 0) return -1;

    usleep(5000);
    if (i2c_write_reg8(PCA9685_MODE1, oldmode | 0xA1) < 0) return -1; // AI+restart
    return 0;
}


int servo_init(void)
{
    if (g_i2c_fd >= 0) {
        return 0;
    }

    g_i2c_fd = open(SERVO_I2C_DEV, O_RDWR);
    if (g_i2c_fd < 0) {
        perror("[servo] open(/dev/i2c-1)");
        return -1;
    }

    if (i2c_write_reg8(PCA9685_MODE1, 0x00) < 0) {
        return -1;
    }
    usleep(5000);

    if (pca9685_set_pwm_freq(PCA9685_FREQ_HZ) < 0) {
        fprintf(stderr, "[servo] set_pwm_freq(%.1f) failed\n", PCA9685_FREQ_HZ);
        return -1;
    }

    printf("[servo] init OK (PCA9685 @0x%02X, freq=%.1f Hz)\n",
           PCA9685_ADDR, PCA9685_FREQ_HZ);
    return 0;
}

// 각도 → 펄스폭(us) → 0~4095 카운트 변환
static uint16_t angle_to_count(float angle_deg)
{
    if (angle_deg < 0.0f)   angle_deg = 0.0f;
    if (angle_deg > 180.0f) angle_deg = 180.0f;

    float pulse = SERVO_MIN_PULSE_US +
                  (SERVO_MAX_PULSE_US - SERVO_MIN_PULSE_US) * (angle_deg / 180.0f);

    float period_us = 1000000.0f / PCA9685_FREQ_HZ;
    float duty = pulse / period_us;                 
    uint16_t count = (uint16_t)(duty * 4095.0f);
    if (count > 4095) count = 4095;
    return count;
}

int servo_set_angle(servo_id_t id, float angle_deg)
{
    if (g_i2c_fd < 0) {
        fprintf(stderr, "[servo] servo_set_angle before init\n");
        return -1;
    }
    if (id < 0 || id >= SERVO_COUNT) {
        return -2;
    }

    uint8_t ch;
    switch (id) {
    case SERVO_NECK_PAN:   ch = SERVO_CH_NECK_PAN;  break;
    case SERVO_NECK_TILT:  ch = SERVO_CH_NECK_TILT; break;
    case SERVO_STEER:      ch = SERVO_CH_STEER;     break;
    default: return -2;
    }

    uint16_t off = angle_to_count(angle_deg);
    return pca9685_set_pwm(ch, 0, off);
}