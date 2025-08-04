#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "hstx_dvi_core.h"
#include "hardware/pio.h"


void hstx_dvi_row_fifo_init(PIO pio, uint sm, hstx_dvi_row_t* underflow_row);
void hstx_dvi_row_fifo_init1(PIO pio, hstx_dvi_row_t* underflow_row);
void hstx_dvi_row_fifo_put_blocking(hstx_dvi_row_t* row);
hstx_dvi_row_t* hstx_dvi_row_fifo_get(uint32_t row_index);

#ifdef __cplusplus
} 
#endif
