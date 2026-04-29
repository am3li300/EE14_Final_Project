// sd_spi.c
//
// implements functions for initializing SPI & SD card
// including startup commands & also reading/writing to SD

#include "sd_spi.h"
#include "ee14lib.h"
#include <stdint.h>
#include <stm32l432xx.h>

#define SD_CMD0 0    /* GO_IDLE_STATE */
#define SD_CMD8 8    /* SEND_IF_COND */
#define SD_CMD55 55  /* APP_CMD */
#define SD_ACMD41 41 /* SEND_OP_COND (ACMD) */
#define SD_CMD58 58  /* READ_OCR */
#define SD_CMD17 17  /* READ */
#define SD_CMD24 24  /* WRITE*/

#define SD_R1_IDLE 0x01
#define SD_R1_READY 0x00

#define SD_CS A3   // A3 = PA4
#define SD_SCK A4  // A4 = PA5 = SPI1_SCK
#define SD_MISO A5 // A5 = PA6 = SPI1_MISO
#define SD_MOSI D2 // D2 = SPI1_MOSI

#define SPI1_AF 5

static int sd_is_sdhc = 0;

// sends a command. SD commands are bitpacked into 6 bytes:
// bits 0-7: CRC
// bits 8-39: argument
// bits 40-45: command ID
// bits 46 & 47: 10
// https://www.ece.tufts.edu/es/4/labs/lab7_sdcard.pdf
static uint8_t sd_send_cmd(uint8_t cmd, uint32_t arg, uint8_t crc)
{
        uint8_t response;

        sd_deselect();
        sd_spi_txrx(0xFF);

        sd_select();
        sd_spi_txrx(0xFF);

        sd_spi_txrx(0x40 | cmd);
        sd_spi_txrx((uint8_t)(arg >> 24));
        sd_spi_txrx((uint8_t)(arg >> 16));
        sd_spi_txrx((uint8_t)(arg >> 8));
        sd_spi_txrx((uint8_t)arg);
        sd_spi_txrx(crc);

        for (int i = 0; i < 10; i++) {
                response = sd_spi_txrx(0xFF);

                if (response != 0xFF) {
                        return response;
                }
        }

        return 0xFF;
}

// note should call sd_spi_init before sd init
// https://electronics.stackexchange.com/questions/77417/what-is-the-correct-command-sequence-for-microsd-card-initialization-in-spi

int sd_init(void)
{
        uint8_t r;

        sd_deselect();

        /*
         * SD card needs 74 clock cycles after power up (CS high)
         * before accepting commands.
         */
        for (int i = 0; i < 10; i++) {
                sd_spi_txrx(0xFF);
        }

        /*
         * CMD0: reset card into SPI idle mode.
         * Expected response: 0x01.
         */
        for (int tries = 0; tries < 100; tries++) {
                r = sd_send_cmd(SD_CMD0, 0x00000000, 0x95);

                if (r == SD_R1_IDLE) {
                        break;
                }

                if (tries == 99) {
                        sd_deselect();
                        return -1;
                }
        }

        /*
         * CMD8: check SD version and voltage range.
         * Want arg 0x1AA and response 0x01.
         */
        r = sd_send_cmd(SD_CMD8, 0x000001AA, 0x87);

        if (r != SD_R1_IDLE) {
                sd_deselect();
                return -2;
        }

        /*
         * CMD8 has a 5-byte response total:
         * We already read R1, now read the remaining 4 bytes.
         *
         * For a normal SDv2 card, these should end in:
         * 0x01 0xAA
         */
        uint8_t cmd8_resp[4];

        for (int i = 0; i < 4; i++) {
                cmd8_resp[i] = sd_spi_txrx(0xFF);
        }

        if (cmd8_resp[2] != 0x01 || cmd8_resp[3] != 0xAA) {
                sd_deselect();
                return -3;
        }

        /*
         * ACMD41 initializes the card.
         *
         * ACMD means "application command", so we must send CMD55 first,
         * then ACMD41.
         *
         * We repeat until the card leaves idle state and returns 0x00.
         */
        for (int tries = 0; tries < 10000; tries++) {
                r = sd_send_cmd(SD_CMD55, 0x00000000, 0x01);

                if (r > SD_R1_IDLE) {
                        sd_deselect();
                        return -4;
                }

                r = sd_send_cmd(SD_ACMD41, 0x40000000, 0x01);

                if (r == SD_R1_READY) {
                        break;
                }

                if (tries == 9999) {
                        sd_deselect();
                        return -5;
                }
        }

        /*
         * CMD58: read OCR register.
         * This helps confirm the card is ready and tells us card capacity
         * type.
         */
        r = sd_send_cmd(SD_CMD58, 0x00000000, 0x01);

        if (r != SD_R1_READY) {
                sd_deselect();
                return -6;
        }

        uint8_t ocr[4];

        for (int i = 0; i < 4; i++) {
                ocr[i] = sd_spi_txrx(0xFF);
        }

        sd_is_sdhc = (ocr[0] & 0x40) != 0;
        printf("SDHC: %d\n", sd_is_sdhc);

        sd_deselect();
        sd_spi_txrx(0xFF);

        return 0;
}

void sd_select(void)
{
        // set CS pin low
        gpio_write(SD_CS, 0);
}

void sd_deselect(void)
{
        // set CS pin high
        gpio_write(SD_CS, 1);
}

void sd_spi_init(void)
{
        // CS pin
        gpio_config_mode(SD_CS, OUTPUT);
        gpio_config_otype(SD_CS, PUSH_PULL);
        gpio_config_ospeed(SD_CS, HI_SPD);
        gpio_config_pullup(SD_CS, PULL_OFF);
        sd_deselect();

        // SPI pins
        gpio_config_alternate_function(SD_SCK, SPI1_AF);
        gpio_config_alternate_function(SD_MISO, SPI1_AF);
        gpio_config_alternate_function(SD_MOSI, SPI1_AF);

        gpio_config_otype(SD_SCK, PUSH_PULL);
        gpio_config_otype(SD_MISO, PUSH_PULL);
        gpio_config_otype(SD_MOSI, PUSH_PULL);

        gpio_config_ospeed(SD_SCK, HI_SPD);
        gpio_config_ospeed(SD_MISO, HI_SPD);
        gpio_config_ospeed(SD_MOSI, HI_SPD);

        gpio_config_pullup(SD_SCK, PULL_OFF);
        gpio_config_pullup(SD_MISO, PULL_OFF);
        gpio_config_pullup(SD_MOSI, PULL_OFF);

        // Enable SPI1 peripheral clock
        RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;

        // Configure SPI1
        SPI1->CR1 &= ~SPI_CR1_SPE;

        SPI1->CR1 = 0;
        SPI1->CR2 = 0;

        SPI1->CR1 |= SPI_CR1_MSTR; // master mode
        SPI1->CR1 |= SPI_CR1_SSM;  // software slave management
        SPI1->CR1 |= SPI_CR1_SSI;  // internal NSS high

        // slow speed for SD init. raised later?
        SPI1->CR1 |= SPI_CR1_BR_0 | SPI_CR1_BR_1 | SPI_CR1_BR_2;

        // SPI mode 0
        SPI1->CR1 &= ~SPI_CR1_CPOL;
        SPI1->CR1 &= ~SPI_CR1_CPHA;

        // 8-bit data frame
        SPI1->CR2 |= (0b0111 << SPI_CR2_DS_Pos);

        // RXNE when 8 bits received
        SPI1->CR2 |= SPI_CR2_FRXTH;

        SPI1->CR1 |= SPI_CR1_SPE;
}

// handles read and write cuz full duplex
uint8_t sd_spi_txrx(uint8_t data)
{
        // write to SD
        while (!(SPI1->SR & SPI_SR_TXE)) {
        }

        *((volatile uint8_t *)&SPI1->DR) = data;

        // read from SD
        while (!(SPI1->SR & SPI_SR_RXNE)) {
        }

        return *((volatile uint8_t *)&SPI1->DR);
}

// writes 0xFF to SD, waiting until SD responds with token
static int sd_wait_token(uint8_t token)
{
        for (int i = 0; i < 100000; i++) {
                uint8_t r = sd_spi_txrx(0xFF);
                if (r == token)
                        return 0;
                if (r != 0xFF)
                        return -1;
        }
        return -1;
}

// reads a block (512 bytes) from SD
int sd_read_block(uint32_t sector, uint8_t *buffer)
{
        uint8_t r;

        uint32_t addr = sd_is_sdhc ? sector : sector * 512;
        r = sd_send_cmd(SD_CMD17, addr, 0x01);

        if (r != 0x00) {
                sd_deselect();
                return -1;
        }

        // sd_spi_txrx(0xFF);
        // sd_spi_txrx(0xFF);

        // Wait for data token 0xFE
        if (sd_wait_token(0xFE) != 0) {
                sd_deselect();
                return -2;
        }

        // Read 512 bytes
        for (int i = 0; i < 512; i++) {
                buffer[i] = sd_spi_txrx(0xFF);
        }

        // Ignore CRC (2 bytes)
        sd_spi_txrx(0xFF);
        sd_spi_txrx(0xFF);

        sd_deselect();
        sd_spi_txrx(0xFF);

        return 0;
}

// writes a block (512 bytes) to SD
int sd_write_block(uint32_t sector, const uint8_t *buffer)
{
        uint8_t r;

        uint32_t addr = sd_is_sdhc ? sector : sector * 512;
        r = sd_send_cmd(SD_CMD24, addr, 0x01);

        if (r != 0x00) {
                sd_deselect();
                return -1;
        }

        // Send start token
        sd_spi_txrx(0xFE);

        // Write 512 bytes
        for (int i = 0; i < 512; i++) {
                sd_spi_txrx(buffer[i]);
        }

        // Dummy CRC
        sd_spi_txrx(0xFF);
        sd_spi_txrx(0xFF);

        // Read data response
        uint8_t resp = sd_spi_txrx(0xFF);

        if ((resp & 0x1F) != 0x05) { // 0x05 = accepted
                sd_deselect();
                return -2;
        }

        // Wait while card is busy
        while (sd_spi_txrx(0xFF) == 0x00) {
        }

        sd_deselect();
        sd_spi_txrx(0xFF);

        return 0;
}