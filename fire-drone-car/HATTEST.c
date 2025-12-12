#include <stdio.h>
#include <wiringPiI2C.h>
#include <unistd.h>

#define PCA9685_ADDR 0x5F
#define MODE1 0x00
#define LED0_ON_L 0x06
#define PRESCALE 0xFE

#define PUMP_CHANNEL 4  // 서보 포트 4번

int pca_fd;

void pca9685_init() {
    pca_fd = wiringPiI2CSetup(PCA9685_ADDR);
    if (pca_fd < 0) {
        printf("I2C setup failed\n");
        return;
    }
    
    // Reset
    wiringPiI2CWriteReg8(pca_fd, MODE1, 0x00);
    usleep(5000);
    
    // Set PWM frequency to 50Hz
    wiringPiI2CWriteReg8(pca_fd, MODE1, 0x10); // Sleep
    wiringPiI2CWriteReg8(pca_fd, PRESCALE, 0x79); // 50Hz
    wiringPiI2CWriteReg8(pca_fd, MODE1, 0x00); // Wake
    usleep(5000);
    wiringPiI2CWriteReg8(pca_fd, MODE1, 0x80); // Restart
}

void pca9685_setPWM(int channel, int on, int off) {
    int base_reg = LED0_ON_L + 4 * channel;
    wiringPiI2CWriteReg8(pca_fd, base_reg,     on & 0xFF);
    wiringPiI2CWriteReg8(pca_fd, base_reg + 1, on >> 8);
    wiringPiI2CWriteReg8(pca_fd, base_reg + 2, off & 0xFF);
    wiringPiI2CWriteReg8(pca_fd, base_reg + 3, off >> 8);
}

void pump_on() {
    printf("Pump ON\n");
    pca9685_setPWM(PUMP_CHANNEL, 0, 4095);
}

void pump_off() {
    printf("Pump OFF\n");
    pca9685_setPWM(PUMP_CHANNEL, 0, 0);
}

int main() {
    printf("Initializing PCA9685...\n");
    pca9685_init();
    
    printf("Testing water pump...\n");
    
    // 3초간 펌프 작동
    pump_on();
    sleep(3);
    
    pump_off();
    
    printf("Test complete\n");
    return 0;
}
