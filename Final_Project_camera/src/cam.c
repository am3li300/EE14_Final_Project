#include "cam.h"
#include "spi.h"
#include "ee14lib.h"
#include <stdint.h>
#include <stdbool.h>

#define SPI_CHANNEL SPI3
#define CAM_SCK D13
#define CAM_MISO D12
#define CAM_MOSI D11
#define CAM_CS A3

#define TEST_REG ((uint8_t) 0x00)

#define FIFO_CONTROL ((uint8_t)0x04)
#define FIFO_CLEAR_MASK 0x01
#define FIFO_START_MASK 0x02

#define TRIGGER_SOURCE ((uint8_t)0x41)
#define CAP_DONE_MASK 0x08

#define FIFO_SIZE1				0x42  // FIFO size[7:0]
#define FIFO_SIZE2				0x43  // FIFO size[15:8]
#define FIFO_SIZE3				0x44  // FIFO size[18:16]

#define SINGLE_FIFO_READ ((uint8_t)0x3D)


void cam_write_reg(uint8_t reg, uint8_t value)
{
    uint8_t buf[] = { reg, value };
    spi_select(CAM_CS);
    spi_tx(SPI_CHANNEL, buf, 2);
    spi_deselect(CAM_CS);
}

uint8_t cam_read_reg(uint8_t reg)
{
    uint8_t value = 0;
    spi_select(CAM_CS);
    spi_tx(SPI_CHANNEL, &reg, 1)
    spi_rx(SPI_CHANNEL, &value, 1);
    spi_deselect(CAM_CS);
    return value;
}

void cam_init()
{
    spi_init(SPI_CHANNEL, CAM_SCK, CAM_MISO, CAM_MOSI, CAM_CS);
    //i2c init
}

bool cam_spi_ok()
{
    uint8_t test, res;
    test = 0x55;
    cam_write_reg(TEST_REG, test);
    res = cam_read_reg(TEST_REG);

    return test == res;
}

void cam_clear_fifo_flag()
{
    cam_write_reg(FIFO_CONTROL, FIFO_CLEAR_MASK);
}

void cam_start_capture()
{
    cam_write_reg(FIFO_CONTROL, FIFO_START_MASK);
}

void cam_wait_capture()
{
    while (!(cam_read_reg(TRIGGER_SOURCE) & CAP_DONE_MASK)) {}
}

uint32_t cam_get_fifo_length()
{
    uint32_t len1, len2, len3, length = 0;
	len1 = cam_read_reg(FIFO_SIZE1);
    len2 = cam_read_reg(FIFO_SIZE2);
    len3 = cam_read_reg(FIFO_SIZE3) & 0x7f;
    length = ((len3 << 16) | (len2 << 8) | len1) & 0x07fffff;
	return length;	
}

uint8_t cam_read_fifo()
{
    return cam_read_reg(SINGLE_FIFO_READ);
}