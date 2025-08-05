// Copyright (c) 2024 Raspberry Pi (Trading) Ltd.

// Generate DVI output using the command expander and TMDS encoder in HSTX.

// This example requires an external digital video connector connected to
// GPIOs 12 through 19 (the HSTX-capable GPIOs) with appropriate
// current-limiting resistors, e.g. 270 ohms. The pinout used in this example
// matches the Pico DVI Sock board, which can be soldered onto a Pico 2:
// https://github.com/Wren6991/Pico-DVI-Sock

#include "pico/sem.h"
#include "hstx_dvi_core.h"
#include "mountains_640x480_rgb332.h"
#define framebuf mountains_640x480

hstx_dvi_row_t* __scratch_x("") hstx_dvi_get_pixel_row(uint32_t row_index) {
    return (hstx_dvi_row_t*)&framebuf[row_index * MODE_H_ACTIVE_PIXELS];
}

int main(void) {

    hstx_dvi_init(hstx_dvi_get_pixel_row);
    hstx_dvi_start();

    while (1)
        __wfi();
}

