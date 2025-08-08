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
#include "pico/multicore.h"

static hstx_dvi_row_t _underflow_row;

static const char* kubla = "In Xanadu did Kubla Khan \n\
A stately pleasure-dome decree: \n\
Where Alph, the sacred river, ran \n\
Through caverns measureless to man \n\
   Down to a sunless sea. \n\
So twice five miles of fertile ground \n\
With walls and towers were girdled round; \n\
And there were gardens bright with sinuous rills, \n\
Where blossomed many an incense-bearing tree; \n\
And here were forests ancient as the hills, \n\
Enfolding sunny spots of greenery."; 

void __not_in_flash_func(render_loop)() {

    hstx_dvi_init(hstx_dvi_row_fifo_get_row_fetcher(), &_underflow_row);

    for(uint32_t frame_index = 0; true; ++frame_index) {
        hstx_dvi_grid_render_frame(frame_index);
    }
}

int main(void)
{
    // Initialize stdio and GPIO 25 for the onboard LED
    stdio_init_all();
    gpio_init(25);
    gpio_set_dir(25, GPIO_OUT);
    gpio_put(25, 1); // Turn LED on
    
    for (uint32_t j = 0; j < MODE_H_ACTIVE_PIXELS; ++j)
    {
        hstx_dvi_row_set_pixel(&_underflow_row, j, hstx_dvi_pixel_rgb(0,255,0));
    }

    // Initialize the row buffer
    hstx_dvi_row_buf_init();

    sleep_ms(2000); // Allow time for initialization

    printf("HSTX DVI Text Test\n");

    hstx_dvi_grid_init();

    hstx_dvi_grid_set_pallet(0, hstx_dvi_pixel_rgb(0,0,0));
    hstx_dvi_grid_set_pallet(1, hstx_dvi_pixel_rgb(255,0,0));
    hstx_dvi_grid_set_pallet(2, hstx_dvi_pixel_rgb(0,255,0));
    hstx_dvi_grid_set_pallet(3, hstx_dvi_pixel_rgb(0,0,255));
    hstx_dvi_grid_set_pallet(4, hstx_dvi_pixel_rgb(255,255,0));
    hstx_dvi_grid_set_pallet(5, hstx_dvi_pixel_rgb(255,0,255));

    hstx_dvi_grid_write_str(5, 0, kubla, 5, 0, HSTX_DVI_GRID_ATTRS_REVERSE);
    hstx_dvi_grid_write_ch(0, 0, '0', 1, 0, HSTX_DVI_GRID_ATTRS_NORMAL);
    hstx_dvi_grid_write_ch(0, 79, '1', 2, 0, HSTX_DVI_GRID_ATTRS_NORMAL);
    hstx_dvi_grid_write_ch(59, 0, '2', 3, 0, HSTX_DVI_GRID_ATTRS_NORMAL);
    hstx_dvi_grid_write_ch(59, 79, '3', 0, 4, HSTX_DVI_GRID_ATTRS_NORMAL);
    hstx_dvi_grid_write_ch(20, 10, '4', 4, 0, HSTX_DVI_GRID_ATTRS_BLINK);
    hstx_dvi_grid_write_ch(20, 11, '5', 4, 0, HSTX_DVI_GRID_ATTRS_REVERSE | HSTX_DVI_GRID_ATTRS_BLINK);
    hstx_dvi_grid_write_ch(20, 12, '6', 4, 0, HSTX_DVI_GRID_ATTRS_REVERSE);
    hstx_dvi_grid_write_ch(20, 12, '6', 4, 0, HSTX_DVI_GRID_ATTRS_REVERSE);
    hstx_dvi_grid_write_str(22, 0, "Normal", 4, 0, HSTX_DVI_GRID_ATTRS_NORMAL);
    hstx_dvi_grid_write_str(23, 0, "Dim", 4, 0, HSTX_DVI_GRID_ATTRS_DIM);
    hstx_dvi_grid_write_str(25, 0, "Underlined", 4, 0, HSTX_DVI_GRID_ATTRS_UNDERLINE);
    
    // Initialize the HSTX DVI row FIFO.
    hstx_dvi_row_fifo_init1(pio0, &_underflow_row);

    multicore_launch_core1(render_loop);
   
    char buffer[64];
    uint32_t k = 0;
    while(1) {
        sleep_ms(100);
        sprintf(buffer, "HSTX DVI Text Test %ld", k++);
        hstx_dvi_grid_write_str(30, 0, buffer, 5, 0, HSTX_DVI_GRID_ATTRS_NORMAL);
    }
}

