#include "motor_control.h"
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <math.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>

#ifndef MOTOR_I2C_DEV
#define MOTOR_I2C_DEV "/dev/i2c-1"
#endif

#define PCA9685_ADDR       0x5f
#define PCA9685_MODE1      0x00
#define PCA9685_PRESCALE   0xFE
#define PCA9685_LED0_ON_L  0x06

// Adeept Robot HAT V3.x DC motor PWM frequency: 1000Hz
#define PCA9685_FREQ_HZ    1000.0f

#define MOTOR_M1_IN1_CH    15   // positive pole of M1
#define MOTOR_M1_IN2_CH    14   // negative pole of M1

static int g_i2c_fd = -1;

static int i2c_set_slave(uint8_t addr)
{
    if (g_i2c_fd < 0) return -1;
    if (ioctl(g_i2c_fd, I2C_SLAVE, addr) < 0) {
        perror("[motor] ioctl(I2C_SLAVE)");
        return -1;
    }
    return 0;
}

static int i2c_write_reg8(uint8_t reg, uint8_t val)
{
    uint8_t buf[2] = { reg, val };
    if (i2c_set_slave(PCA9685_ADDR) < 0) return -1;
    if (write(g_i2c_fd, buf, 2) != 2) {
        perror("[motor] i2c_write_reg8");
        return -1;
    }
    return 0;
}

static int i2c_read_reg8(uint8_t reg, uint8_t *out)
{
    if (!out) return -1;
    if (i2c_set_slave(PCA9685_ADDR) < 0) return -1;

    if (write(g_i2c_fd, &reg, 1) != 1) {
        perror("[motor] i2c_read_reg8 write-reg");
        return -1;
    }
    if (read(g_i2c_fd, out, 1) != 1) {
        perror("[motor] i2c_read_reg8 read");
        return -1;
    }
    return 0;
}

static int pca9685_set_pwm(uint8_t channel, uint16_t duty)
{
    if (duty > 4095) duty = 4095;

    uint8_t reg = PCA9685_LED0_ON_L + 4 * channel;
    uint8_t buf[5] = {
        reg,
        0, 0,                         // ON_L, ON_H
        (uint8_t)(duty & 0xFF),        // OFF_L
        (uint8_t)(duty >> 8)           // OFF_H
    };

    if (i2c_set_slave(PCA9685_ADDR) < 0) return -1;
    if (write(g_i2c_fd, buf, sizeof(buf)) != (ssize_t)sizeof(buf)) {
        perror("[motor] pca9685_set_pwm");
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

    uint8_t sleep_mode = (oldmode & 0x7F) | 0x10; // SLEEP=1
    if (i2c_write_reg8(PCA9685_MODE1, sleep_mode) < 0) return -1;
    if (i2c_write_reg8(PCA9685_PRESCALE, prescale) < 0) return -1;
    if (i2c_write_reg8(PCA9685_MODE1, oldmode) < 0) return -1;

    usleep(5000);
    if (i2c_write_reg8(PCA9685_MODE1, oldmode | 0xA1) < 0) return -1; // AI + RESTART
    return 0;
}

int motor_init(void)
{
    if (g_i2c_fd >= 0) return 0;

    g_i2c_fd = open(MOTOR_I2C_DEV, O_RDWR);
    if (g_i2c_fd < 0) {
        perror("[motor] open(/dev/i2c-1)");
        return -1;
    }

    if (i2c_write_reg8(PCA9685_MODE1, 0x00) < 0) {
        fprintf(stderr, "[motor] PCA9685 MODE1 init failed\n");
        return -1;
    }
    usleep(5000);

    if (pca9685_set_pwm_freq(PCA9685_FREQ_HZ) < 0) {
        fprintf(stderr, "[motor] set_pwm_freq(%.1f) failed\n", PCA9685_FREQ_HZ);
        return -1;
    }

    motor_stop();
    printf("[motor] init OK (PCA9685 @0x%02X, M1 in1=%d in2=%d, freq=%.0fHz)\n",
           PCA9685_ADDR, MOTOR_M1_IN1_CH, MOTOR_M1_IN2_CH, PCA9685_FREQ_HZ);
    return 0;
}

void motor_stop(void)
{
    if (g_i2c_fd < 0) return;
    pca9685_set_pwm(MOTOR_M1_IN1_CH, 0);
    pca9685_set_pwm(MOTOR_M1_IN2_CH, 0);
}

void motor_set_speed(float speed)
{
    if (g_i2c_fd < 0) return;

    if (speed > 1.0f)  speed = 1.0f;
    if (speed < -1.0f) speed = -1.0f;

    if (speed == 0.0f) {
        motor_stop();
        return;
    }

    uint16_t duty = (uint16_t)(fabsf(speed) * 4095.0f);

    if (speed > 0.0f) {
        pca9685_set_pwm(MOTOR_M1_IN2_CH, 0);
        pca9685_set_pwm(MOTOR_M1_IN1_CH, duty);
    } else {
        pca9685_set_pwm(MOTOR_M1_IN1_CH, 0);
        pca9685_set_pwm(MOTOR_M1_IN2_CH, duty);
    }
}
