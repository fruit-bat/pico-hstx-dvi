// Copyright (c) 2025 fruit-bat

// Generate DVI output using the command expander and TMDS encoder in HSTX.

// This example requires an external digital video connector connected to
// GPIOs 12 through 19 (the HSTX-capable GPIOs) with appropriate
// current-limiting resistors, e.g. 270 ohms. The pinout used in this example
// matches the Pico DVI Sock board, which can be soldered onto a Pico 2:
// https://github.com/Wren6991/Pico-DVI-Sock

#include "hstx_dvi_core.h"
#include "app_led.h"
#include "hstx_dvi_row_fifo.h"
#include "hstx_dvi_row_buf.h"
#include "hstx_dvi_vt.h"
#include "pico/stdio.h"
#include "pico/stdlib.h"
#include <stdio.h>
#include <string.h>

int main(void)
{
    stdio_init_all();
    app_led_on();

    hstx_dvi_vt_init_all();

    // The font chosen when built
    if (!hstx_dvi_vt_set_font(hstx_dvi_font_find(VT_FONT), VT_SCALE_X, VT_SCALE_Y)) {
        printf("Can't use font %s x%d x%d\n", VT_FONT, VT_SCALE_X, VT_SCALE_Y);
    }
#ifdef VT_FONT_AUTO
    // The host asking for a size picks the font
    hstx_dvi_vt_set_font_auto(true);
#endif

    sleep_ms(2000); // Allow time for initialization

    vt_emu_t* e = hstx_dvi_vt_emu_get();

    // Echo whatever arrives on stdio to the screen
    while(1) {
        char ch = getchar();
        if (ch) vt_emu_put_ch(e, ch);
    }
}

