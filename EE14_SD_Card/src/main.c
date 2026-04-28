#include "ee14lib.h"
#include <stdint.h>
#include <stdio.h>

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

        //EXTI->RTSR1 |= (1 << 1);  // rising for door opening
        //EXTI->FTSR1 &= ~(1 << 1); // disable falling

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
    if(gpio_read(D6)) {
        printf("working\r\n");
    }

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

int main()
{

        SysTick_initialize();
        host_serial_init(9600);

        printf("starting program\n");

        // toggle switch
        gpio_config_mode(D5, INPUT);
        gpio_config_pullup(D5, PULL_UP);

        // reed switch
        gpio_config_mode(D6, INPUT);
        gpio_config_pullup(D6, PULL_UP);

        printf("finish config\n");

        while (1) {
                printf("loop\n");
                delay_ms(1000);

                //EXTI1_IRQHandler();
        }
}
