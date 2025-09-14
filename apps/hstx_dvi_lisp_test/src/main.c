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
#include "hstx_dvi_vt.h"
#include "vt/vt_emu.h"
#include "pico/stdio.h"
#include "pico/stdlib.h"
#include <stdio.h>
#include <string.h>
#include "pico/stdio.h"
#include "pico/stdio/driver.h"

static vt_emu_t* _emu;

// Our custom driver
static void vt_out_chars(const char *buf, int length) {
    for (int i = 0; i < length; i++) {
        vt_emu_put_ch(_emu, buf[i]);
    }
}

// No input override — return 0 so we don’t consume any stdin
static int vt_in_chars(char *buf, int length) {
    return 0;
}

static struct stdio_driver vt_driver = {
    .out_chars = vt_out_chars,
   // .crlf_enabled = PICO_STDIO_DEFAULT_CRLF

//    .in_chars = vt_in_chars
};

extern int lisp_main(int argc, char **argv);


int main(void)
{
    // Initialize stdio and GPIO 25 for the onboard LED
    gpio_init(25);
    gpio_set_dir(25, GPIO_OUT);
    gpio_put(25, 1); // Turn LED on

    hstx_dvi_vt_init_all();

    _emu = hstx_dvi_vt_emu_get();

    stdio_init_all();
    stdio_set_driver_enabled(&vt_driver, true);

    sleep_ms(2000); // Allow time for initialization

    printf("HSTX DVI Lisp Test\n");

    while(1) {

        lisp_main(0, NULL);

    }
}

