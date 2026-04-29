#ifndef SD_SPI_H
#define SD_SPI_H

#include <stdint.h>

void sd_spi_init(void);
int sd_init(void);
uint8_t sd_spi_txrx(uint8_t data);

void sd_select(void);
void sd_deselect(void);

int sd_read_block(uint32_t sector, uint8_t *buffer);
int sd_write_block(uint32_t sector, const uint8_t *buffer);

#endif