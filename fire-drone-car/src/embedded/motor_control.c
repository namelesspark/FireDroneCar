<<<<<<< HEAD
//asdasd
=======
#include "motor_control.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <math.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>

//로봇에 맞는 하드웨어 설정하기 
#ifndef MOTOR_I2C_DEV
#define MOTOR_I2C_DEV "/dev/i2c-1"
#endif
#define PCA9685_ADDR       0x5f
#define PCA9685_MODE1      0x00
#define PCA9685_PRESCALE   0xFE
#define PCA9685_LED0_ON_L  0x06

// DC 모터 PWM용 PCA9685 채널 (EN 핀에 연결)
#define MOTOR_PWM_CH       8   // 예시: ch8 사용. 실제 연결 채널로 바꾸기.

// 서보와 같이 쓰므로 PCA9685 전체 주파수는 50Hz로 고정
#define PCA9685_FREQ_HZ    50.0f

// L298 IN1/IN2 에 연결된 라즈베리파이 GPIO 번호 (BCM 번호 기준이라고 가정)
#define MOTOR_IN1_GPIO     17  // 예시 (GPIO17)
#define MOTOR_IN2_GPIO     27  // 예시 (GPIO27)


static int g_i2c_fd      = -1;
static int g_gpio_in1_fd = -1;
static int g_gpio_in2_fd = -1;


static int gpio_export(int gpio)
{
    int fd = open("/sys/class/gpio/export", O_WRONLY);
    if (fd < 0) {
        perror("[motor] gpio_export open");
        return -1;
    }
    char buf[8];
    int len = snprintf(buf, sizeof(buf), "%d", gpio);
    if (write(fd, buf, len) != len) {
    }
    close(fd);
    return 0;
}

static int gpio_set_direction(int gpio, const char *dir)
{
    char path[64];
    snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/direction", gpio);
    int fd = open(path, O_WRONLY);
    if (fd < 0) {
        perror("[motor] gpio_set_direction open");
        return -1;
    }
    if (write(fd, dir, strlen(dir)) < 0) {
        perror("[motor] gpio_set_direction write");
        close(fd);
        return -1;
    }
    close(fd);
    return 0;
}

static int gpio_open_value_fd(int gpio)
{
    char path[64];
    snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/value", gpio);
    int fd = open(path, O_WRONLY);
    if (fd < 0) {
        perror("[motor] gpio_open_value_fd open");
    }
    return fd;
}

static int gpio_write_fd(int fd, int value)
{
    if (fd < 0) return -1;
    const char *s = value ? "1" : "0";
    if (write(fd, s, 1) != 1) {
        perror("[motor] gpio_write_fd");
        return -1;
    }
    return 0;
}


static int i2c_set_slave(uint8_t addr)
{
    if (g_i2c_fd < 0) {
        fprintf(stderr, "[motor] I2C not opened\n");
        return -1;
    }
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

    ssize_t w = write(g_i2c_fd, buf, 2);
    if (w != 2) {
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
    uint8_t buf[5];

    buf[0] = reg;
    buf[1] = 0;              // ON_L
    buf[2] = 0;              // ON_H
    buf[3] = duty & 0xFF;    // OFF_L
    buf[4] = duty >> 8;      // OFF_H

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

    uint8_t sleep_mode = (oldmode & 0x7F) | 0x10; 
    if (i2c_write_reg8(PCA9685_MODE1, sleep_mode) < 0) return -1;
    if (i2c_write_reg8(PCA9685_PRESCALE, prescale) < 0) return -1;
    if (i2c_write_reg8(PCA9685_MODE1, oldmode) < 0) return -1;

    usleep(5000);
    if (i2c_write_reg8(PCA9685_MODE1, oldmode | 0xA1) < 0) return -1;
    return 0;
}


int motor_init(void)
{
    if (g_i2c_fd >= 0) {
        return 0; 
    }

    // I2C 오픈
    g_i2c_fd = open(MOTOR_I2C_DEV, O_RDWR);
    if (g_i2c_fd < 0) {
        perror("[motor] open(/dev/i2c-1)");
        return -1;
    }

    // PCA9685 모드 초기화
    if (i2c_write_reg8(PCA9685_MODE1, 0x00) < 0) {
        return -1;
    }
    usleep(5000);

    if (pca9685_set_pwm_freq(PCA9685_FREQ_HZ) < 0) {
        fprintf(stderr, "[motor] set_pwm_freq(%.1f) failed\n", PCA9685_FREQ_HZ);
        return -1;
    }

    // GPIO export + direction 설정
    gpio_export(MOTOR_IN1_GPIO);
    gpio_export(MOTOR_IN2_GPIO);
    gpio_set_direction(MOTOR_IN1_GPIO, "out");
    gpio_set_direction(MOTOR_IN2_GPIO, "out");

    g_gpio_in1_fd = gpio_open_value_fd(MOTOR_IN1_GPIO);
    g_gpio_in2_fd = gpio_open_value_fd(MOTOR_IN2_GPIO);

    motor_stop();

    printf("[motor] init OK (PCA9685 @0x%02X, pwm_ch=%d, GPIO IN1=%d, IN2=%d)\n",
           PCA9685_ADDR, MOTOR_PWM_CH, MOTOR_IN1_GPIO, MOTOR_IN2_GPIO);

    return 0;
}

void motor_stop(void)
{
    if (g_i2c_fd < 0) return;
    // PWM 0, IN1/IN2 LOW
    pca9685_set_pwm(MOTOR_PWM_CH, 0);
    gpio_write_fd(g_gpio_in1_fd, 0);
    gpio_write_fd(g_gpio_in2_fd, 0);
}

void motor_set_speed(float speed)
{
    if (g_i2c_fd < 0) {
        fprintf(stderr, "[motor] motor_set_speed before init\n");
        return;
    }

    if (speed > 1.0f)  speed = 1.0f;
    if (speed < -1.0f) speed = -1.0f;

    // 방향 설정
    if (speed > 0.0f) {
        gpio_write_fd(g_gpio_in1_fd, 1);
        gpio_write_fd(g_gpio_in2_fd, 0);
    } else if (speed < 0.0f) {
        gpio_write_fd(g_gpio_in1_fd, 0);
        gpio_write_fd(g_gpio_in2_fd, 1);
    } else {
        // 정지
        motor_stop();
        return;
    }

    float mag = fabsf(speed);
    uint16_t duty = (uint16_t)(mag * 4095.0f);
    pca9685_set_pwm(MOTOR_PWM_CH, duty);
}
>>>>>>> c3c9a5a (임베디드 첫번째 업로드)
