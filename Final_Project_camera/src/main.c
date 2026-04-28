#include "ee14lib.h"
#include <stdio.h>
#include <stdint.h>

// systick stuff
#define INPUTCLOCK 4000000 // 4MHz
#define TARGETFREQ 1000 // 1kHz
#define RELOAD (INPUTCLOCK / TARGETFREQ - 1)
#define ONESEC TARGETFREQ

// i2c stuff
#define I2C1_SCL D5
#define I2C1_SDA D4

#define I2C3_SCL A6
#define I2C3_SDA D12

// device stuff
#define DEVICE_ADDR 0x3C
#define COMPRESSION_ENABLE ((char)0x3821)
#define NUM_DATA 7

#define BYTE 8
#define HALFBYTE 4

// static unsigned char STATUS_CHECK[] = { 0x71 };
// static unsigned char MEASURE_CMD[] = { 0xAC, 0x33, 0x00 };

// volatile unsigned int counter = 0;

// void  SysTick_Handler(void) {
//     counter++;
// }

// void SysTick_initialize(void) {
//     SysTick->CTRL = 0;
//     SysTick->LOAD = RELOAD;
//     // This sets the priority of the interrupt to 15 (2^4 - 1), which is the
//     // largest supported value (aka lowest priority)
//     NVIC_SetPriority (SysTick_IRQn, (1<<__NVIC_PRIO_BITS) - 1);
//     SysTick->VAL = 0;
//     SysTick->CTRL |= SysTick_CTRL_CLKSOURCE_Msk;
//     SysTick->CTRL |= SysTick_CTRL_TICKINT_Msk;
//     SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;
// }

// void delay_ms(unsigned int delay)
// {
//     volatile unsigned int target = counter + delay;
//     while (counter < target) {}
// }

int _write(int file, char *data, int len) {
    serial_write(USART2, data, len);
    return len;
}

// void print_data(unsigned char *data) {
//     uint32_t h_signal, t_signal;
//     float humidity, temp;
//     h_signal = 0;
//     t_signal = 0;

//     h_signal |= (uint32_t)data[3] >> HALFBYTE;
//     h_signal |= (uint32_t)data[2] << HALFBYTE;
//     h_signal |= (uint32_t)data[1] << (BYTE + HALFBYTE);

//     t_signal |= (uint32_t)data[5];
//     t_signal |= (uint32_t)data[4] << BYTE;
//     t_signal |= ((uint32_t)data[3] & 0b1111) << (2 * BYTE);

//     humidity = ((float)h_signal / (1UL << 20)) * 100;
//     temp = (((float)t_signal / (1UL << 20)) * 200) - 50;

//     printf("%d,%d\n", (int)humidity, (int)temp);
// }

int main()
{
    //SysTick_initialize();
    host_serial_init(9600);
    i2c_init(I2C3, I2C3_SCL, I2C3_SDA);

    for (volatile int i = 0; i < 500000; i++) {}

    // printf("Starting check.\n");
    // for (int i = 0; i < 128; i++) {
    //     if (i2c_write(I2C3, i, NULL, 0)) {
    //         printf("Address %d acked\n", i);
    //     }
    //     else {
    //         printf("%d\n", i);
    //     }
    // }
    // printf("Checked all addresses.\n");

    char buf = 1;
    i2c_write(I2C3, DEVICE_ADDR, COMPRESSION_ENABLE, 1);
    i2c_read(I2C3, DEVICE_ADDR, &buf, 1);
    printf("register value: %d\n", buf);

    
}