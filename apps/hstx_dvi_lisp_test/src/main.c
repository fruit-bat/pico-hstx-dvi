// Copyright (c) 2024 Raspberry Pi (Trading) Ltd.

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
#include "vt_emu.h"
#include "pico/stdio.h"
#include "pico/stdlib.h"
#include <stdio.h>
#include <string.h>
#include "pico/stdio/driver.h"

static vt_emu_t* _emu;

// Output-only stdio driver that echoes stdout to the screen;
// input still comes from the other enabled drivers (USB)
static void vt_out_chars(const char *buf, int length) {
    for (int i = 0; i < length; i++) {
        vt_emu_put_ch(_emu, buf[i]);
    }
}

static struct stdio_driver vt_driver = {
    .out_chars = vt_out_chars,
#if PICO_STDIO_ENABLE_CRLF_SUPPORT
    // Send "\n" as "\r\n", as the USB driver does, or each line starts where the last one ended
    .crlf_enabled = PICO_STDIO_DEFAULT_CRLF,
#endif
};

extern int lisp_main(int argc, char **argv);

int main(void)
{
    app_led_on();

    hstx_dvi_vt_init_all();

    _emu = hstx_dvi_vt_emu_get();

    // The screen only mirrors stdout, so leave answering queries (e.g. ESC [ 6 n)
    // to the USB terminal. A reply from here would be printed back into the
    // emulator while it is still handling the query.
    vt_emu_set_reply(_emu, NULL, NULL);

    stdio_init_all();
    stdio_set_driver_enabled(&vt_driver, true);

    sleep_ms(2000); // Allow time for initialization

    printf("HSTX DVI Lisp Test\n");

    while(1) {
        lisp_main(0, NULL);
    }
}

