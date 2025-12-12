#include <wiringPiI2C.h>
#include <unistd.h>

#define PCA9685_ADDR 0x5F
#define PUMP_CHANNEL 4

int main() {
    int fd = wiringPiI2CSetup(PCA9685_ADDR);
    
    printf("Pump ON\n");
    wiringPiI2CWriteReg8(fd, 0x06 + 4*4 + 2, 0xFF);
    wiringPiI2CWriteReg8(fd, 0x06 + 4*4 + 3, 0x0F);
    sleep(3);
    
    printf("Pump OFF\n");
    wiringPiI2CWriteReg8(fd, 0x06 + 4*4 + 2, 0);
    wiringPiI2CWriteReg8(fd, 0x06 + 4*4 + 3, 0);
    
    return 0;
}
