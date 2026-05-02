#ifndef CAM_H
#define CAM_H

#include <stdbool.h>
#include <stdint.h>

bool cam_spi_ok();
void cam_clear_fifo_flag();
void cam_start_capture();
void cam_wait_capture();
uint32_t cam_get_fifo_length();
uint8_t cam_read_fifo();

#endif