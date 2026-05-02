#include "ee14lib.h"
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#define I2C3_SCL A6
#define I2C3_SDA D12

#define I2C1_SCL D1
#define I2C1_SDA D0

#define DEVICE_ADDR_W 0x60
#define DEVICE_ADDR_R 0x61

#define BUFSIZE 512

int _write(int file, char *data, int len) {
    serial_write(USART2, data, len);
    return len;
}

bool ov5640_write_reg(uint16_t reg, uint8_t value) 
{
   unsigned char buf[3];
   buf[0] = (reg >> 8) & 0xFF;   // register address high byte
   buf[1] = reg & 0xFF;          // register address low byte
   buf[2] = value;               // data byte
   return i2c_write(I2C3, DEVICE_ADDR, buf, 3);
}
bool ov5640_read_reg(uint16_t reg, uint8_t *value) 
{
   unsigned char addr[2];
   addr[0] = (reg >> 8) & 0xFF;
   addr[1] = reg & 0xFF;
   if (!i2c_write(I2C3, DEVICE_ADDR, addr, 2)) {
       return false;
   }
   return i2c_read(I2C3, DEVICE_ADDR, value, 1);
}

bool ov2460_init() 
{
    if (i2c_init(I2C1, I2C1_SCL, I2C1_SDA) != EE14Lib_Err_OK) {
        return false;
    }

    return true;
}

int main()
{
    //SysTick_initialize();
    host_serial_init(9600);
    if (!ov2460_init()) {
        printf("Error initializing camera\n");
    }

    for (int i = 0; i < 1000000; i++) {}

    // i2c configuration

    if (cam_spi_ok()) {
        printf("Arducam SPI bus active\n");
    }
    else {
        printf("Arducam SPI bus failed\n");
        return 0;
    }

    cam_clear_fifo_flag();
    cam_start_capture();
    cam_wait_capture();
    printf("Done capturing!\n");

    uint32_t length = cam_get_fifo_length();
    int i = 0;
    uint8_t buf[BUF_SIZE] = {0};
    uint8_t curr = 0, last = 0;

    while (length--) {
        last = curr;
        curr = cam_read_fifo();
        printf("%#04X\n", curr);

        if (i < BUFSIZE) {
            buf[i++] = curr;
        }
        else {
            // fwrite buf
            i = 0;
            buf[i++] = curr;
        }
    }

    printf("Done reading fifo\n");
}