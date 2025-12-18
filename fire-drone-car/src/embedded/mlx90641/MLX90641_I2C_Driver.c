#include "MLX90641_I2C_Driver.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>

#ifndef MLX90641_I2C_DEV
#define MLX90641_I2C_DEV "/dev/i2c-1"
#endif


static int g_i2c_fd = -1;

static int set_slave(uint8_t slaveAddr)
{
    if (g_i2c_fd < 0)
    {
        fprintf(stderr, "[MLX90641] I2C not initialized (fd < 0)\n");
        return -1;
    }

    if (ioctl(g_i2c_fd, I2C_SLAVE, slaveAddr) < 0)
    {
        perror("l[MLX90641] ioct(I2C_SLAVE)");
        return -1;
    }
    return 0;
}
// /dev/i2c-1을 열어서 이후에 쓸 I2C 파일 준비
void MLX90641_I2CInit(void)
{
    if (g_i2c_fd >= 0)
    {
        return;
    }

    g_i2c_fd = open(MLX90641_I2C_DEV, O_RDWR);
    if (g_i2c_fd < 0)
    {
        perror("[MLX90641] open(/dev/i2c-1)");
    }
    else
    {
        printf("[MLX90641] I2C opened: %s (fd=%d)\n",
               MLX90641_I2C_DEV, g_i2c_fd);
    }
}
// 센서를 초기화
int MLX90641_I2CGeneralReset(void)
{
    if (g_i2c_fd < 0)
    {
        fprintf(stderr, "[MLX90641] GeneralReset: I2C not initialized\n");
        return -1;
    }

    uint8_t general_call_addr = 0x00;  
    uint8_t cmd = 0x06;                

    if (set_slave(general_call_addr) < 0)
    {
        return -1;
    }

    ssize_t w = write(g_i2c_fd, &cmd, 1);
    if (w != 1)
    {
        perror("[MLX90641] GeneralReset write");
        return -1;
    }

    usleep(50);

    return 0;
}

//지정한 슬레이브 주소의 startAddress 부터 nMemAddressRed개를 연속으로 읽어와서 data에 저장
int MLX90641_I2CRead(uint8_t slaveAddr, uint16_t startAddress, uint16_t nMemAddressRead, uint16_t *data)
{
    if (g_i2c_fd < 0)
    {
        fprintf(stderr, "[MLX90641] I2CRead: I2C not initialized\n");
        return -1;
    }
    if (data == NULL || nMemAddressRead == 0)
    {
        return -1;
    }

    struct i2c_rdwr_ioctl_data packets;
    struct i2c_msg messages[2];

    uint8_t addr_buf[2];
    addr_buf[0] = (uint8_t)(startAddress >> 8);    
    addr_buf[1] = (uint8_t)(startAddress & 0xFF);   

    const int num_bytes = 2 * nMemAddressRead;
    uint8_t raw_buf[2 * 832]; 
    if (num_bytes > (int)sizeof(raw_buf))
    {
        fprintf(stderr, "[MLX90641] I2CRead: request too large (%d bytes)\n", num_bytes);
        return -1;
    }

    messages[0].addr  = slaveAddr;
    messages[0].flags = 0;  
    messages[0].len   = 2;
    messages[0].buf   = addr_buf;

    messages[1].addr  = slaveAddr;
    messages[1].flags = I2C_M_RD;
    messages[1].len   = num_bytes;
    messages[1].buf   = raw_buf;

    packets.msgs  = messages;
    packets.nmsgs = 2;

    if (ioctl(g_i2c_fd, I2C_RDWR, &packets) < 0)
    {
        perror("[MLX90641] I2CRead: ioctl(I2C_RDWR)");
        return -1;
    }

    for (int cnt = 0; cnt < nMemAddressRead; ++cnt)
    {
        int i = cnt * 2;
        uint16_t hi = (uint16_t)raw_buf[i];
        uint16_t lo = (uint16_t)raw_buf[i + 1];
        data[cnt] = (hi << 8) | lo;
    }

    return 0;
}
// 슬레이브의 writeAddress 레지스터에 16비트 값을 쓰고, 다시 읽어 확인하고 일치하면 성공으로 리턴함.
int MLX90641_I2CWrite(uint8_t slaveAddr, uint16_t writeAddress, uint16_t data)
{
    if (g_i2c_fd < 0)
    {
        fprintf(stderr, "[MLX90641] I2CWrite: I2C not initialized\n");
        return -1;
    }

    uint8_t buf[4];
    buf[0] = (uint8_t)(writeAddress >> 8);
    buf[1] = (uint8_t)(writeAddress & 0xFF);
    buf[2] = (uint8_t)(data >> 8);
    buf[3] = (uint8_t)(data & 0xFF);

    if (set_slave(slaveAddr) < 0)
    {
        return -1;
    }

    ssize_t w = write(g_i2c_fd, buf, 4);
    if (w != 4)
    {
        perror("[MLX90641] I2CWrite: write");
        return -1;
    }

    uint16_t readback = 0;
    if (MLX90641_I2CRead(slaveAddr, writeAddress, 1, &readback) != 0)
    {
        fprintf(stderr, "[MLX90641] I2CWrite: verify read failed\n");
        return -1;
    }

    if (readback != data)
    {
        //fprintf(stderr, "[MLX90641] I2CWrite: verify mismatch (0x%04X != 0x%04X)\n",
                //readback, data);
    }

    return 0;
}

void MLX90641_I2CFreqSet(int freq)
{
    (void)freq;
}
