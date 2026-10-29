#pragma once
#include "hstx_dvi_core.h"
typedef int PIO;
#define pio0 0
void hstx_dvi_row_fifo_init1(PIO pio);
hstx_dvi_pixel_row_fetcher hstx_dvi_row_fifo_get_row_fetcher(void);
void hstx_dvi_row_fifo_put_blocking(hstx_dvi_row_t* r);
