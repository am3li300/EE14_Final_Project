#include "spi.h"
#include "ee14lib.h"
#include <stdint.h>

void spi_select(EE14Lib_Pin cs)
{
        // set CS pin low
        gpio_write(cs, 0);
}

void spi_deselect(EE14Lib_Pin cs)
{
        // set CS pin high
        gpio_write(cs, 1);
}


// configures spi channel for SPI mode 0
EE14Lib_Err spi_init(SPI_TypeDef *spi, EE14Lib_Pin sck, EE14Lib_Pin miso, EE14Lib_Pin mosi, EE14Lib_Pin cs)
{
    // CS pin
    gpio_config_mode(cs, OUTPUT);
    gpio_config_otype(cs, PUSH_PULL);
    gpio_config_ospeed(cs, HI_SPD);
    gpio_config_pullup(cs, PULL_OFF);
    spi_deselect(cs);

    // SPI pins
    if (spi == SPI1) {
        gpio_config_alternate_function(sck, SPI1_AF);
        gpio_config_alternate_function(sck, SPI1_AF);
        gpio_config_alternate_function(sck, SPI1_AF);

        // Enable SPI1 peripheral clock
        RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;
    }
    else if (spi == SPI3) {
        gpio_config_alternate_function(sck, SPI3_AF);
        gpio_config_alternate_function(sck, SPI3_AF);
        gpio_config_alternate_function(sck, SPI3_AF);

        // Enable SPI1 peripheral clock
        RCC->APB2ENR |= RCC_APB2ENR_SPI3EN;
    }
    else {
        return EE14Lib_ERR_INVALID_CONFIG; // Invalid SPI device
    }

    gpio_config_otype(sck, PUSH_PULL);
    gpio_config_otype(miso, PUSH_PULL);
    gpio_config_otype(mosi, PUSH_PULL);

    gpio_config_ospeed(sck, HI_SPD);
    gpio_config_ospeed(miso, HI_SPD);
    gpio_config_ospeed(mosi, HI_SPD);

    gpio_config_pullup(sck, PULL_OFF);
    gpio_config_pullup(miso, PULL_OFF);
    gpio_config_pullup(mosi, PULL_OFF);

    // Configure SPI1
    spi->CR1 &= ~SPI_CR1_SPE;

    spi->CR1 = 0;
    spi->CR2 = 0;

    spi->CR1 |= SPI_CR1_MSTR; // master mode
    spi->CR1 |= SPI_CR1_SSM;  // software slave management
    spi->CR1 |= SPI_CR1_SSI;  // internal NSS high

    // slow speed for SD init. raised later?
    spi->CR1 |= SPI_CR1_BR_0 | SPI_CR1_BR_1 | SPI_CR1_BR_2;

    // SPI mode 0
    spi->CR1 &= ~SPI_CR1_CPOL;
    spi->CR1 &= ~SPI_CR1_CPHA;

    // 8-bit data frame
    spi->CR2 |= (0b0111 << SPI_CR2_DS_Pos);

    // RXNE when 8 bits received
    spi->CR2 |= SPI_CR2_FRXTH;

    spi->CR1 |= SPI_CR1_SPE;
}

void spi_tx(SPI_TypeDef *spi, uint8_t *data, unsigned int len)
{
    while (int i = 0; i < len; i++) {
        while (!(spi->SR & SPI_SR_TXE)) {}
        *((volatile uint8_t *)&spi->DR) = data[i];
    }
}

void spi_rx(SPI_TypeDef *spi, uint8_t *buf, unsigned int len)
{
    while (int i = 0; i < len; i++) {
        while (!(spi->SR & SPI_SR_RXNE)) {}
        buf[i] = *((volatile uint8_t *)&spi->DR);
    }
}