// Copyright (c) 2024 Raspberry Pi (Trading) Ltd.

// Generate DVI output using the command expander and TMDS encoder in HSTX.

// This example requires an external digital video connector connected to
// GPIOs 12 through 19 (the HSTX-capable GPIOs) with appropriate
// current-limiting resistors, e.g. 270 ohms. The pinout used in this example
// matches the Pico DVI Sock board, which can be soldered onto a Pico 2:
// https://github.com/Wren6991/Pico-DVI-Sock

#include "hstx_dvi_core.h"
#include "hstx_dvi_row_fifo.h"
#include "hstx_dvi_row_buf.h"

hstx_dvi_row_t _underflow_row;

int main(void) {
    for (uint32_t j = 0; j < HSTX_DVI_BYTES_PER_ROW; ++j) {
        _underflow_row.b[j] =  100;
    }

    // Initialize the row buffer
    hstx_dvi_row_buf_init();

    // Initialize the HSTX DVI row FIFO. This also initializes the HSTX DVI once the FIFO is full.
    hstx_dvi_row_fifo_init1(pio0, &_underflow_row);

    uint32_t k = 0;
    while (1) {
        hstx_dvi_row_t* r = hstx_dvi_row_buf_get();
        for (uint32_t j = 0; j < HSTX_DVI_BYTES_PER_ROW; ++j) {
           // r->b[j] =  (j + (k++&32)) % 256;
            r->b[j] =  100; // Fill the row with a simple pattern
        }
        hstx_dvi_row_fifo_put_blocking(r);
        k++;
    }
}
