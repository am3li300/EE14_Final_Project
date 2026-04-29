#include "ee14lib.h"
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#define I2C3_SCL A6
#define I2C3_SDA D12

#define DEVICE_ADDR 0x3C

#define COMPRESSION_ENABLE_ADDR 0x3821
#define COMPRESSION_ENABLE_POS 5

#define JPG_MODE_SELECT_ADDR 0x4713
#define JPEG_MODE_3 ((uint8_t)0b011)

#define FORMAT_CONTROL00_ADDR 0x4300
#define RGB565_3 ((uint8_t)0x61)

#define HREF D1
#define VSYNC D0
#define PCLK D3
#define SHUTTER D4

static EE14Lib_Pin DATA_PINS[] = { D7, D8, D9, D10, D11, D13, A1, A0 };

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

    gpio_config_mode(HREF, INPUT);
    gpio_config_pullup(HREF, PULL_DOWN);

    gpio_config_mode(VSYNC, INPUT);
    gpio_config_pullup(VSYNC, PULL_DOWN);

    gpio_config_mode(PCLK, INPUT);
    gpio_config_pullup(PCLK, PULL_DOWN);

    gpio_config_mode(HREF, INPUT);
    gpio_config_pullup(HREF, PULL_DOWN);

    gpio_config_mode(SHUTTER, OUTPUT);

    for (int i = 0; i < (sizeof(DATA_PINS) / sizeof(EE14Lib_Pin)); i++) {
        gpio_config_mode(DATA_PINS[i], INPUT);
        gpio_config_pullup(DATA_PINS[i], PULL_DOWN);
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

    while(1) {
        for (int i = 0; i < 1000000000; i++) {}

        gpio_write(SHUTTER, 1);
        gpio_write(SHUTTER, 0);

        printf("HREF: %d, VSYNC: %d, PCLK: %d\nD1: %d, D2: %d, D3: %d, D4: %d, D5: %d, D6: %d, D7: %d, D8: %d\n", 
               gpio_read(HREF), gpio_read(VSYNC), gpio_read(PCLK), 
               gpio_read(DATA_PINS[0]),
               gpio_read(DATA_PINS[1]),
               gpio_read(DATA_PINS[2]),
               gpio_read(DATA_PINS[3]),
               gpio_read(DATA_PINS[4]),
               gpio_read(DATA_PINS[5]),
               gpio_read(DATA_PINS[6]),
               gpio_read(DATA_PINS[7])
            );
    }


    
}