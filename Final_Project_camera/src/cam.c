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
#define TRIGGER_SOURCE ((uint8_t)0x41)
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

bool cam_spi_ok()
{
    uint8_t test, res;
    test = 0x55;
    cam_write_reg(TEST_REG, test);
    res = cam_read_reg(TEST_REG);

    return test == res;
}




/*
cam_spi_ok()

flush_fifo/clear_fifo_flag by writing to fifo control

start capture by writing to fifo control

check if capture done flag set in trigger source (camera write done fifo flag)

get fifo length (ie image size)

get image data from single read register

*/