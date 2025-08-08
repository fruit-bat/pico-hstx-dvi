// Copyright (c) 2024 Raspberry Pi (Trading) Ltd.

// Generate DVI output using the command expander and TMDS encoder in HSTX.

// This example requires an external digital video connector connected to
// GPIOs 12 through 19 (the HSTX-capable GPIOs) with appropriate
// current-limiting resistors, e.g. 270 ohms. The pinout used in this example
// matches the Pico DVI Sock board, which can be soldered onto a Pico 2:
// https://github.com/Wren6991/Pico-DVI-Sock

// TMT from https://github.com/MurphyMc/libtmt-revival

#include "hstx_dvi_core.h"
#include "hstx_dvi_row_fifo.h"
#include "hstx_dvi_row_buf.h"
#include "hstx_dvi_grid.h"
#include "pico/stdio.h"
#include "pico/stdlib.h"
#include <stdio.h>
#include <string.h>
#include "pico/multicore.h"

#include "libtmt/tmt.h"

static hstx_dvi_row_t _underflow_row;

void __not_in_flash_func(render_loop)() {
    while(1) {
        hstx_dvi_grid_render_frame();
    }
}

/* Forward declaration of a callback.
 * libtmt will call this function when the terminal's state changes.
 */
void callback(tmt_msg_t m, TMT *vt, const void *a, void *p);


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
        printf("HSTX DVI TMT Test\n");
    }

    for (uint32_t j = 0; j < HSTX_DVI_BYTES_PER_ROW; ++j)
    {
        _underflow_row.b[j] = 200;
    }

    hstx_dvi_grid_init();

    hstx_dvi_grid_set_pallet(TMT_COLOR_BLACK, hstx_dvi_pixel_rgb(0,0,0));
    hstx_dvi_grid_set_pallet(TMT_COLOR_RED, hstx_dvi_pixel_rgb(255,0,0));
    hstx_dvi_grid_set_pallet(TMT_COLOR_GREEN, hstx_dvi_pixel_rgb(0,255,0));
    hstx_dvi_grid_set_pallet(TMT_COLOR_YELLOW, hstx_dvi_pixel_rgb(255,255, 0));
    hstx_dvi_grid_set_pallet(TMT_COLOR_BLUE, hstx_dvi_pixel_rgb(0,0,255));
    hstx_dvi_grid_set_pallet(TMT_COLOR_MAGENTA, hstx_dvi_pixel_rgb(255,0,255));
    hstx_dvi_grid_set_pallet(TMT_COLOR_CYAN, hstx_dvi_pixel_rgb(0,255,255));
    hstx_dvi_grid_set_pallet(TMT_COLOR_WHITE, hstx_dvi_pixel_rgb(255,255,255));

    multicore_launch_core1(render_loop);

        /* Open a virtual terminal with 2 lines and 10 columns.
     * The first NULL is just a pointer that will be provided to the
     * callback; it can be anything. The second NULL specifies that
     * we want to use the default Alternate Character Set; this
     * could be a pointer to a wide string that has the desired
     * characters to be displayed when in ACS mode.
     */
    TMT *vt = tmt_open(60, 80, callback, NULL, NULL);
    if (!vt) {
        hstx_dvi_grid_write_str(5, 0, "Failed to start TMT", 5, 0);
    }
    else {
        hstx_dvi_grid_write_str(5, 0, "Started TMT", 5, 0);

        /* Write some text to the terminal, using escape sequences to
        * use a bold rendition.
        *
        * The final argument is the length of the input; 0 means that
        * libtmt will determine the length dynamically using strlen.
        */
       tmt_write(vt, "\033[1mhello, world (in bold!)\033[0m", 0);
    }

    while(1) {
        const int ch = getchar();
        tmt_write(vt, (const char *)&ch, 1);
    }
}

void
callback(tmt_msg_t m, TMT *vt, const void *a, void *p)
{
    /* grab a pointer to the virtual screen */
    const TMTSCREEN *s = tmt_screen(vt);
    const TMTPOINT *c = tmt_cursor(vt);

    switch (m){
        case TMT_MSG_BELL:
            /* the terminal is requesting that we ring the bell/flash the
             * screen/do whatever ^G is supposed to do; a is NULL
             */
            printf("bing!\n");
            break;

        case TMT_MSG_UPDATE:
            /* the screen image changed; a is a pointer to the TMTSCREEN */
            for (size_t r = 0; r < s->nline; r++){
                if (s->lines[r]->dirty){
                    for (size_t c = 0; c < s->ncol; c++){
                        const tmt_color_t fg = s->lines[r]->chars[c].a.fg;
                        const tmt_color_t bg = s->lines[r]->chars[c].a.bg;
                        const hstx_dvi_pixel_t fgci = fg == TMT_COLOR_DEFAULT ? TMT_COLOR_GREEN : fg;
                        const hstx_dvi_pixel_t bgci = bg == TMT_COLOR_DEFAULT ? TMT_COLOR_BLACK : bg;

                        hstx_dvi_grid_write_ch(
                            r, 
                            c, 
                            s->lines[r]->chars[c].c, 
                            fgci, 
                            bgci);

                        // printf("contents of %zd,%zd,%zd,%zd: '%lc' (%s bold)\n",
                        //     r, c,
                        //     fgci,bgci,
                        //     s->lines[r]->chars[c].c,
                        //     s->lines[r]->chars[c].a.bold? "is" : "is not");
                    }
                }
            }

            /* let tmt know we've redrawn the screen */
            tmt_clean(vt);
            break;

        case TMT_MSG_ANSWER:
            /* the terminal has a response to give to the program; a is a
             * pointer to a string */
            printf("terminal answered %s\n", (const char *)a);
            break;

        case TMT_MSG_MOVED:
            /* the cursor moved; a is a pointer to the cursor's TMTPOINT */
            printf("cursor is now at %zd,%zd\n", c->r, c->c);
            break;

        case TMT_MSG_TITLE:
            /* the terminal has a new title; a is a pointer to a wide string */
            printf("terminal title is now %ls\n", (const wchar_t *)a);
            break;
        case TMT_MSG_SETMODE:
            /* the terminal is requesting that we set a mode; a is a pointer
             * to an array of size_t that contains the mode numbers to set */
            printf("set mode %zd\n", ((const size_t *)a)[0]);
            break;
        case TMT_MSG_UNSETMODE:
            /* the terminal is requesting that we unset a mode; a is a pointer
             * to an array of size_t that contains the mode numbers to unset */
            break;
        case TMT_MSG_CURSOR:
            /* the terminal is requesting that we show or hide the cursor;
             * a is a string that is either "t" (show) or "f" (hide) */
            if (strcmp((const char *)a, "t") == 0) {
                printf("showing cursor\n");
            } else if (strcmp((const char *)a, "f") == 0) {
                printf("hiding cursor\n");
            }
            break;
    }
}