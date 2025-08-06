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
#include "pico/stdio.h"
#include <stdio.h>

hstx_dvi_row_t _underflow_row;

int main(void)
{

    // Initialize stdio and GPIO 25 for the onboard LED
    stdio_init_all();
    gpio_init(25);
    gpio_set_dir(25, GPIO_OUT);
    gpio_put(25, 1); // Turn LED on

    // Initialize the row buffer
    hstx_dvi_row_buf_init();

    for (uint32_t j = 0; j < MODE_H_ACTIVE_PIXELS; ++j)
    {
        hstx_dvi_row_set_pixel(&_underflow_row, j, hstx_dvi_row_pixel_rgb(0,255,0));
    }

    // Initialize the HSTX DVI row FIFO. This also initializes the HSTX DVI once the FIFO is full.
    hstx_dvi_row_fifo_init1(pio0, &_underflow_row);

    sleep_ms(2000); // Allow time for initialization

    for (uint32_t j = 0; j < 1; ++j)
    {
        printf("HSTX DVI Row FIFO Test\n");
    }
    for (uint32_t j = 0; j < 8; ++j)
    {
        hstx_dvi_row_t *r = hstx_dvi_row_buf_get();
        hstx_dvi_row_fifo_put_blocking(r);
        sleep_ms(100); // Allow time for the FIFO to process
        hstx_dvi_row_t *f = hstx_dvi_row_fifo_get(j);
        printf("r=%ld 0x%08lx 0x%08lx\n", j, (uint32_t)r, (uint32_t)f);
    }

    uint32_t k = 0;
    uint32_t f = 200;
    while (1)
    {
        hstx_dvi_row_t *r = hstx_dvi_row_buf_get();
        const u_int32_t k1 = k & 0xff;
        for (uint32_t j = 0; j < MODE_H_ACTIVE_PIXELS >> 1; ++j)
        {
            // hstx_dvi_row_set_pixel(r, j, hstx_dvi_row_pixel_rgb(k&0xff,j&0xff,(k+j)&0xff));
            const u_int32_t j1 = j << 1;
            const u_int32_t j2 = j1 + 1;
            hstx_dvi_row_set_pixel_pair(
                r, j,
                hstx_dvi_row_pixel_rgb(k1,j1&0xff,(k+j1)&0xff),
                hstx_dvi_row_pixel_rgb(k1,j2&0xff,(k+j2)&0xff)    
            );
        }
        hstx_dvi_row_fifo_put_blocking(r);
        k++;
        // Test we can recover from a FIFO underflow
        if (k == 470 && f > 0) {
            f--;
            sleep_ms(10);
        }
        if (k >= MODE_V_ACTIVE_LINES) k = 0;
    }
}
