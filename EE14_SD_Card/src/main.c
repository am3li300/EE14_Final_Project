#include "ee14lib.h"
#include "ff.h"
#include "sd_spi.h"
#include <stdint.h>
#include <stdio.h>
#include <stm32l432xx.h>
#include <string.h>

// global vars
volatile uint8_t armed = 0;
volatile uint8_t door_open = 0;
volatile uint8_t take_pic = 0;
volatile uint32_t ticks = 0;

// for printf
int _write(int file, char *data, int len)
{
        serial_write(USART2, data, len);
        return len;
}

/*                              TIMING STUFF                             */
void SysTick_Handler(void)
{
        ticks++;
}

void SysTick_initialize(void)
{
        SysTick->CTRL = 0;
        SysTick->LOAD = 4000;
        NVIC_SetPriority(SysTick_IRQn, (1 << __NVIC_PRIO_BITS) - 1);
        SysTick->VAL = 0;
        SysTick->CTRL |= SysTick_CTRL_CLKSOURCE_Msk;
        SysTick->CTRL |= SysTick_CTRL_TICKINT_Msk;
        SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;
}

void delay_ms(int ms)
{
        uint32_t start = ticks;
        while (ticks - start < ms) {
        }
}

// toggle switch
void config_toggle_interrupt(void)
{
        RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;

        // D5 is PB6 and EXTI6
        SYSCFG->EXTICR[1] &= ~(0xF << 8);
        SYSCFG->EXTICR[1] |= (0x1 << 8); // Port b

        EXTI->FTSR1 |= (1 << 6);
        EXTI->RTSR1 |= (1 << 6);

        EXTI->IMR1 |= (1 << 6);

        // switch should overide everything prolly
        NVIC_SetPriority(EXTI9_5_IRQn, 2);
        NVIC_EnableIRQ(EXTI9_5_IRQn);
}

// reed switch
void config_reed_interrupt(void)
{
        RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;

        // PB1 is EXTI1
        SYSCFG->EXTICR[0] &= ~(0xF << 4);
        SYSCFG->EXTICR[0] |= (0x1 << 4); // port B

        EXTI->RTSR1 |= (1 << 1);  // rising for door opening
        EXTI->FTSR1 &= ~(1 << 1); // disable falling

        EXTI->IMR1 |= (1 << 1);

        NVIC_SetPriority(EXTI1_IRQn, 2);
        NVIC_EnableIRQ(EXTI1_IRQn);
}

// handler for toggle interupt
void EXTI9_5_IRQHandler(void)
{
        if (EXTI->PR1 & (1 << 6)) {
                EXTI->PR1 |= (1 << 6);
                // high means switch opened and low closed, so flipping
                // opened means pin is 1, and it shouldnt be armed
                armed = !gpio_read(D5);

                // test
                // printf("TOGGLE ISR: armed = %d\r\n", armed);
        }
}

// handler for reed interupt
void EXTI1_IRQHandler(void)
{
        if (EXTI->PR1 & (1 << 1)) {
                EXTI->PR1 |= (1 << 1);

                // HIGH = door open
                // LOW  = door closed
                door_open = gpio_read(D6);

                // test
                printf("DOOR ISR triggered\r\n");

                if (door_open && armed) {
                        // pic here somehow idk
                        // prolly smarter to set a flag so not doing I2C in the
                        // interupt handler
                        take_pic = 1;
                }
        }
}

// STM32                PiCowBell
// =================================
// A4   (SPI1_SCK)   →  SCK
// D2   (SPI1_MOSI)  →  MOSI
// A5   (SPI1_MISO)  ←  MISO
// A3   (GPIO)       →  CS      (interchangeable)
// 3.3V              →  3V
// GND               →  GND

int main()
{
        SysTick_initialize();
        host_serial_init(9600);

        printf("Starting Program\n");

        // toggle switch
        gpio_config_mode(D5, INPUT);
        gpio_config_pullup(D5, PULL_UP);

        // reed switch
        gpio_config_mode(D6, INPUT);
        gpio_config_pullup(D6, PULL_UP);

        printf("finished interrupt config\n");

        sd_spi_init();
        int init = sd_init();
        printf("sd_init: %d\n", init);

        if (init != 0) {
                while (1) {
                        delay_ms(1000);
                        printf("SD init failed\n");
                }
        }

        FATFS fs;
        FIL file;
        FRESULT res;
        UINT written;

        // -------------------- TEST BLOCK -----------------------------
        uint8_t block[512];

        int rr = sd_read_block(0, block);
        printf("read block 0: %d\n", rr);

        uint32_t part_lba = block[454] | (block[455] << 8) |
                            (block[456] << 16) | (block[457] << 24);

        uint8_t part_type = block[450];

        printf("partition type: %02X\n", part_type);
        printf("partition start LBA: %lu\n", part_lba);

        sd_read_block(part_lba, block);

        printf("boot sector first bytes: %02X %02X %02X %02X\n", block[0],
               block[1], block[2], block[3]);
        printf("boot sector signature: %02X %02X\n", block[510], block[511]);
        printf("OEM: %.8s\n", &block[3]);

        uint16_t bytes_per_sector = block[11] | (block[12] << 8);
        printf("bytes/sector: %u\n", bytes_per_sector);
        // -------------------- TEST BLOCK END --------------------------

        // mount sd
        res = f_mount(&fs, "", 1);
        printf("f_mount: %d\n", res);

        if (res != FR_OK) {
                while (1) {
                        delay_ms(1000);
                        printf("Mounting failed\n");
                }
        }

        // Open file
        res = f_open(&file, "test.txt", FA_WRITE | FA_CREATE_ALWAYS);
        printf("f_open: %d\n", res);

        if (res != FR_OK) {
                while (1) {
                        delay_ms(1000);
                        printf("Opening file failed\n");
                }
        }

        // Write to file
        const char *msg = "im so hungry\n";
        res = f_write(&file, msg, strlen(msg), &written);
        printf("f_write: %d, written: %u\n", res, written);

        // Close file
        f_close(&file);
        printf("f_close: %d\n", res);

        while (1) {
                printf("success ? \n");
        }
}
