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
#include "pico/stdlib.h"
#include <stdio.h>
#include <string.h>

#include "font_inv.h"
#define FONT_CHAR_WIDTH 8
#define FONT_CHAR_HEIGHT 8
#define FONT_N_CHARS 95
#define FONT_FIRST_ASCII 32

static hstx_dvi_row_t _underflow_row;

static uint16_t _screen[MODE_V_ACTIVE_LINES >> 3][MODE_H_ACTIVE_PIXELS >> 3];

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

int main(void)
{

    // Initialize stdio and GPIO 25 for the onboard LED
    stdio_init_all();
    gpio_init(25);
    gpio_set_dir(25, GPIO_OUT);
    gpio_put(25, 1); // Turn LED on

    // Initialize the row buffer
    hstx_dvi_row_buf_init();

    // Initialize the HSTX DVI row FIFO. This also initializes the HSTX DVI once the FIFO is full.
    hstx_dvi_row_fifo_init1(pio0, &_underflow_row);

    sleep_ms(2000); // Allow time for initialization

    for (uint32_t j = 0; j < 1; ++j)
    {
        printf("HSTX DVI Text Test\n");
    }

    for (uint32_t j = 0; j < HSTX_DVI_BYTES_PER_ROW; ++j)
    {
        _underflow_row.b[j] = 100;
    }

    {
        memset(_screen, 32, sizeof(_screen));
        uint32_t i = 0;
        uint32_t j = 0;
        char *p = (char*)kubla;
        char c;
        while ((c = *p++)) {
            switch(c) {
                case '\n':
                    i = 0;
                    j = j < (MODE_V_ACTIVE_LINES >> 3) ? j + 1 : 0;
                    break;
                case '\r':
                    break;
                default:
                    _screen[j][i++] = (uint16_t)c & 0xff;
                    break;
            }
        }
    }

    uint32_t k = 0;
    while (1)
    {
        hstx_dvi_row_t *r = hstx_dvi_row_buf_get();
        for (uint32_t j = 0; j < HSTX_DVI_BYTES_PER_ROW; j += FONT_CHAR_WIDTH)
        {
            const uint16_t s = _screen[k >> 3][j >> 3];
            const uint32_t d = ((uint32_t)s) & 0xff;
            const uint32_t e = (d < 32 ? 32 : d) - FONT_FIRST_ASCII;
            const uint8_t f = font_8x8[(k & 7) + (e << 3)];
            for (uint32_t i = 0; i < 8; ++i) {
                r->b[j + i] = f & (1 << (7-i)) ? 200 : 0;
            }
        }
        hstx_dvi_row_fifo_put_blocking(r);
        k++;
        if (k >= MODE_V_ACTIVE_LINES) k = 0;
    }
}
