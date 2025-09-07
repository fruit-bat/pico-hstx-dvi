// Copyright (c) 2025 fruit-bat

// Generate DVI output using the command expander and TMDS encoder in HSTX.

// This example requires an external digital video connector connected to
// GPIOs 12 through 19 (the HSTX-capable GPIOs) with appropriate
// current-limiting resistors, e.g. 270 ohms. The pinout used in this example
// matches the Pico DVI Sock board, which can be soldered onto a Pico 2:
// https://github.com/Wren6991/Pico-DVI-Sock

#include "hstx_dvi_core.h"
#include "hstx_dvi_row_fifo.h"
#include "hstx_dvi_row_buf.h"
#include "hstx_dvi_vt.h"
#include "pico/stdio.h"
#include "pico/stdlib.h"
#include <stdio.h>
#include <string.h>
#include "pico/sem.h"

#include "vt/vt_pallet.h"


int main(void)
{
    // Initialize stdio and GPIO 25 for the onboard LED
    stdio_init_all();
    gpio_init(25);
    gpio_set_dir(25, GPIO_OUT);
    gpio_put(25, 1); // Turn LED on

    hstx_dvi_vt_init_all();

    sleep_ms(2000); // Allow time for initialization


    // 

    vt_emu_t* e = hstx_dvi_vt_emu_get();

    while(1) {
        char ch = getchar();
        if (ch) vt_emu_put_ch(e, ch); 
        //__wfi();
    }
}

