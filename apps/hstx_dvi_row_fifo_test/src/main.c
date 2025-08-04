// Copyright (c) 2024 Raspberry Pi (Trading) Ltd.

// Generate DVI output using the command expander and TMDS encoder in HSTX.

// This example requires an external digital video connector connected to
// GPIOs 12 through 19 (the HSTX-capable GPIOs) with appropriate
// current-limiting resistors, e.g. 270 ohms. The pinout used in this example
// matches the Pico DVI Sock board, which can be soldered onto a Pico 2:
// https://github.com/Wren6991/Pico-DVI-Sock

#include "pico/sem.h"
#include "hstx_dvi_core.h"
#include "hstx_dvi_row_fifo.h"

#define HSTX_DVI_ROW_COUNT 4
hstx_dvi_row_t row[4];

int main(void) {
    hstx_dvi_row_fifo_init1(pio0, &row[0]);
    hstx_dvi_start(hstx_dvi_row_fifo_get);


    // Fill the FIFO with some data
    for (uint32_t i = 0; i < HSTX_DVI_ROW_COUNT; ++i) {
        hstx_dvi_row_t* r = &row[i];
        for (uint32_t j = 0; j < HSTX_DVI_BYTES_PER_ROW; ++j) {
            r->b[j] = (i * HSTX_DVI_BYTES_PER_ROW + j) % 256;
        }
    }
    
    uint32_t k = 0;
    while (1) {
        hstx_dvi_row_t* r = &row[k++ % HSTX_DVI_ROW_COUNT];
        hstx_dvi_row_fifo_put_blocking(r);
    }
}
