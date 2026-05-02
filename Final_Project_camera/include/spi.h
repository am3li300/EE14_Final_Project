#include "ee14lib.h"
#include <stdint.h>

void spi_select(EE14Lib_Pin cs);
void spi_deselect(EE14Lib_Pin cs);
EE14Lib_Err spi_init(SPI_TypeDef *spi, EE14Lib_Pin sck, EE14Lib_Pin miso, EE14Lib_Pin mosi, EE14Lib_Pin cs);
void spi_tx(SPI_TypeDef *spi, uint8_t *data, unsigned int len);
void spi_rx(SPI_TypeDef *spi, uint8_t *buf, unsigned int len);