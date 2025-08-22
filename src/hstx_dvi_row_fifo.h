#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "hstx_dvi_core.h"
#include "hardware/pio.h"

#define HSTX_DVI_ROW_FIFO_SIZE 8

hstx_dvi_pixel_row_fetcher hstx_dvi_row_fifo_init(PIO pio, uint sm);
hstx_dvi_pixel_row_fetcher hstx_dvi_row_fifo_init1(PIO pio);
void hstx_dvi_row_fifo_put_blocking(hstx_dvi_row_t* row);
hstx_dvi_row_t* hstx_dvi_row_fifo_get(uint32_t row_index);
hstx_dvi_pixel_row_fetcher hstx_dvi_row_fifo_get_row_fetcher();

#ifdef __cplusplus
} 
#endif

