#include "pump_control.h"
#include <stdio.h>
#include <wiringPiI2C.h>
#include <unistd.h>

// 네 코드 그대로 유지
#define PCA9685_ADDR 0x5F
#define MODE1 0x00
#define LED0_ON_L 0x06
#define PRESCALE 0xFE

static int pca_fd = -1;

static void pca9685_setPWM(int channel, int on, int off)
{
    int base_reg = LED0_ON_L + 4 * channel;

    wiringPiI2CWriteReg8(pca_fd, base_reg,     on & 0xFF);
    wiringPiI2CWriteReg8(pca_fd, base_reg + 1, on >> 8);
    wiringPiI2CWriteReg8(pca_fd, base_reg + 2, off & 0xFF);
    wiringPiI2CWriteReg8(pca_fd, base_reg + 3, off >> 8);
}

static void pca9685_init(void)
{
    pca_fd = wiringPiI2CSetup(PCA9685_ADDR);
    if (pca_fd < 0) {
        printf("I2C setup failed\n");
        return;
    }

    // Reset
    wiringPiI2CWriteReg8(pca_fd, MODE1, 0x00);
    usleep(5000);

    // Set PWM frequency to 50Hz
    wiringPiI2CWriteReg8(pca_fd, MODE1, 0x10);     // Sleep
    wiringPiI2CWriteReg8(pca_fd, PRESCALE, 0x79);  // 50Hz
    wiringPiI2CWriteReg8(pca_fd, MODE1, 0x00);     // Wake
    usleep(5000);
    wiringPiI2CWriteReg8(pca_fd, MODE1, 0x80);     // Restart
}

int pump_init(void)
{
    if (pca_fd >= 0) return 0;

    printf("Initializing PCA9685...\n");
    pca9685_init();

    if (pca_fd < 0) {
        return -1;
    }
    return 0;
}

void pump_on(void)
{
    if (pca_fd < 0) return;
    printf("Pump ON\n");
    pca9685_setPWM(PUMP_CHANNEL, 0, 4095);
}

void pump_off(void)
{
    if (pca_fd < 0) return;
    printf("Pump OFF\n");
    pca9685_setPWM(PUMP_CHANNEL, 0, 0);
}
