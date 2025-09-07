#include "hstx_dvi_vt.h"
#include "hstx_dvi_core.h"
#include "hstx_dvi_row_fifo.h"
#include "hstx_dvi_row_buf.h"
#include "pico/multicore.h"
#include "pico/stdio.h"
#include "pico/stdlib.h"
#include <stdio.h>
#include <string.h>
#include "vt/vt_emu.h"
#include "vt/vt_pallet.h"

#include "font_inv.h"
#define FONT_CHAR_WIDTH 8
#define FONT_CHAR_HEIGHT 8
#define FONT_N_CHARS 95
#define FONT_FIRST_ASCII 32

#define CHAR_ROWS (MODE_V_ACTIVE_LINES / FONT_CHAR_HEIGHT)
#define CHAR_COLS (MODE_H_ACTIVE_PIXELS / FONT_CHAR_WIDTH)

static hstx_dvi_pixel_t _pallet[256];
static vt_cell_t _screen[CHAR_ROWS][CHAR_COLS];
static vt_emu_t vt_emu;

static void vt_pallet_make_cb(
    void *d,
    vt_cell_colour_t i,
    uint32_t r,
    uint32_t g,
    uint32_t b
) {
    _pallet[i] = hstx_dvi_pixel_rgb(r,g,b);
}

void __not_in_flash_func(hstx_dvi_vt_init)() {

    // Setup the pallet
    vt_pallet_make_256_colours(0, vt_pallet_make_cb);

    // Setup the terminal emulator
    vt_emu_init(&vt_emu, (vt_cell_t*)_screen, CHAR_COLS, CHAR_ROWS);

    // Say hello
    vt_emu_put_str(&vt_emu, (vt_char_t*)"Pico virtual terminal v0.1\r\n");
    vt_emu_put_str(&vt_emu, (vt_char_t*)"Provided by fruit-bat 2025\r\n\r\n");
}

void __not_in_flash_func(hstx_dvi_vt_render_frame)(uint32_t frame_index) {
    const bool blink = (frame_index & 63) < 32; // Blink every second for 32 frames
    vt_cell_t** srps = (vt_cell_t**)&vt_emu.term.rp;
    hstx_dvi_pixel_t fgbg[2];
    for(uint32_t k = 0; k < MODE_V_ACTIVE_LINES; k++) {
        hstx_dvi_row_t *r = hstx_dvi_row_buf_get();
        vt_cell_t* srp = srps[k>>3];
        for (uint32_t j = 0; j < CHAR_COLS; j++) {
            const vt_cell_t s = srp[j];
            const uint32_t e = vt_cell_get_char(s) - FONT_FIRST_ASCII;
            const vt_cell_attr_t attr = vt_cell_get_attr(s);
            const vt_cell_flags_t flags = vt_cell_flags_get(attr);
            const bool rev1 = (flags & VT_CELL_FLAGS_BLINK) && blink;
            const bool rev2 = (flags & VT_CELL_FLAGS_REVERSE);
            if (rev1 != rev2) {
                fgbg[0] = _pallet[vt_cell_fg_get(s)];
                fgbg[1] = _pallet[vt_cell_bg_get(s)];
            }
            else {
                fgbg[0] = _pallet[vt_cell_bg_get(s)];
                fgbg[1] = _pallet[vt_cell_fg_get(s)];
            }
            if (flags & VT_CELL_FLAGS_DIM) {
                fgbg[0] = hstx_dvi_pixel_dim(fgbg[0]);
                fgbg[1] = hstx_dvi_pixel_dim(fgbg[1]);
            }
            if (flags & VT_CELL_FLAGS_INVISIBLE) {
                fgbg[1] = fgbg[0];
            }
            if (((k & 7) == (FONT_CHAR_HEIGHT-1)) && (flags & VT_CELL_FLAGS_UNDERLINE)) {
                // Underline is rendered as a solid line at the bottom of the
                // character cell, so we need to set the last row of pixels.
                const uint32_t p1 = fgbg[1];
                for (uint32_t i = 0; i < 2; ++i) {
                    hstx_dvi_row_set_pixel_quad(
                        r, 
                        (j<<1) + i, 
                        p1,p1,p1,p1);
                }
            }
            else {
                uint8_t f = font_8x8[(k & 7) + (e << 3)];
                for (uint32_t i = 0; i < 2; ++i) {
                    const uint32_t p1 = fgbg[(f >> 7) & 1];
                    const uint32_t p2 = fgbg[(f >> 6) & 1];
                    const uint32_t p3 = fgbg[(f >> 5) & 1];
                    const uint32_t p4 = fgbg[(f >> 4) & 1];
                    hstx_dvi_row_set_pixel_quad(
                        r, 
                        (j<<1) + i, 
                        p1,p2,p3,p4);
                    f <<= 4;
                }
            }
        }
        hstx_dvi_row_fifo_put_blocking(r);
    }
}

void hstx_dvi_vt_init_all() {
    // Initialize the row buffer
    hstx_dvi_row_buf_init();

    hstx_dvi_vt_init();

    // Initialize the HSTX DVI row FIFO.
    hstx_dvi_row_fifo_init1(pio0);

    multicore_launch_core1(hstx_dvi_vt_render_loop);
}

void __not_in_flash_func(hstx_dvi_vt_render_loop)() {

    hstx_dvi_init(hstx_dvi_row_fifo_get_row_fetcher());

    for(uint32_t frame_index = 0; true; ++frame_index) {
        hstx_dvi_vt_render_frame(frame_index);
    }
}
