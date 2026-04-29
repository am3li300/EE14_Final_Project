#include "ee14lib.h"
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#define I2C3_SCL A6
#define I2C3_SDA D12

// device stuff
#define DEVICE_ADDR 0x3C

#define COMPRESSION_ENABLE_ADDR 0x3821
#define COMPRESSION_ENABLE_POS 5

#define JPG_MODE_SELECT_ADDR 0x4713
#define JPEG_MODE_3 ((uint8_t)0b011)

#define FORMAT_CONTROL00_ADDR 0x4300
#define RGB565_3 ((uint8_t)0x61)


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

bool ov5640_init() 
{
    i2c_init(I2C3, I2C3_SCL, I2C3_SDA);
    
    if (!ov5640_write_reg(COMPRESSION_ENABLE_ADDR, ((uint8_t)1 << COMPRESSION_ENABLE_POS))) {
        printf("Error configuring compression enable register\n");
        return false;
    }
    if (!ov5640_write_reg(JPG_MODE_SELECT_ADDR, JPEG_MODE_3)) {
        printf("Error configuring jpg mode select register\n");
        return false;
    }
    if (!ov5640_write_reg(FORMAT_CONTROL00_ADDR, RGB565_3)) {
        printf("Error configuring format control register\n");
        return false;
    }

    return true;
}

int main()
{
    //SysTick_initialize();
    host_serial_init(9600);
    if (!ov5640_init()) {
        printf("Error initializing camera\n");
    }
    
}