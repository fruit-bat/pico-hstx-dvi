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
#include "hstx_dvi_grid.h"
#include "pico/stdio.h"
#include "pico/stdlib.h"
#include <stdio.h>
#include <string.h>

extern int lisp_main(int argc, char **argv);


int main(void)
{
    // Initialize stdio and GPIO 25 for the onboard LED
    gpio_init(25);
    gpio_set_dir(25, GPIO_OUT);
    gpio_put(25, 1); // Turn LED on

    hstx_dvi_grid_init_all();

    stdio_init_all();

    sleep_ms(2000); // Allow time for initialization

    printf("HSTX DVI Lisp Test\n");

    hstx_dvi_grid_set_pallet(0, hstx_dvi_pixel_rgb(0,0,0));
    hstx_dvi_grid_set_pallet(1, hstx_dvi_pixel_rgb(255,0,0));
    hstx_dvi_grid_set_pallet(2, hstx_dvi_pixel_rgb(0,255,0));
    hstx_dvi_grid_set_pallet(3, hstx_dvi_pixel_rgb(0,0,255));
    hstx_dvi_grid_set_pallet(4, hstx_dvi_pixel_rgb(255,255,0));
    hstx_dvi_grid_set_pallet(5, hstx_dvi_pixel_rgb(255,0,255));

    while(1) {

        lisp_main(0, NULL);

    }
}

