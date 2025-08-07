#include "hstx_dvi_grid.h"
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

#define CHAR_ROWS (MODE_V_ACTIVE_LINES / FONT_CHAR_HEIGHT)
#define CHAR_COLS (MODE_H_ACTIVE_PIXELS / FONT_CHAR_WIDTH)

static uint32_t _screen[CHAR_ROWS][CHAR_COLS];
static hstx_dvi_pixel_t _pallet[256];

static inline uint32_t enc_char(
    const uint32_t c, 
    const uint32_t fgci,
    const uint32_t bgci
){
    return (c & 0xff) | (fgci << 8) | (bgci << 16);
}

static inline void set_char(
    const uint32_t y,
    const uint32_t x,
    const uint32_t s
) {
    _screen[y][x] = s;
}

static inline void set_char_full(
    const uint32_t y,
    const uint32_t x,
    const uint32_t c, 
    const uint32_t fgci,
    const uint32_t bgci
) {
    if (x < CHAR_COLS && y < CHAR_ROWS) {
        set_char(y, x, enc_char(c, fgci, bgci));
    }
}

static inline uint32_t decode_char(const uint32_t s) {
    const uint32_t d = (s & 0xff);
    return (d < FONT_FIRST_ASCII ? FONT_FIRST_ASCII : d) - FONT_FIRST_ASCII;
}

static inline hstx_dvi_pixel_t get_fg_color(const uint32_t s) {
    return _pallet[(s >> 8) & 0xff];
}

static inline hstx_dvi_pixel_t get_bg_color(const uint32_t s) {
    return _pallet[(s >> 16) & 0xff];
}

void __not_in_flash_func(hstx_dvi_grid_clear)() {
    const uint32_t s = enc_char(' ', 1, 0);
    for (uint32_t j = 0; j < CHAR_COLS; ++j) {
        for (uint32_t i = 0; i < CHAR_ROWS; ++i) {
            set_char(i, j, s);
        }
    }
}

void __not_in_flash_func(hstx_dvi_grid_set_pallet)(
    const uint8_t index,
    hstx_dvi_pixel_t color
) {
    _pallet[index] = color;
}

void __not_in_flash_func(hstx_dvi_grid_init)() {
    _pallet[0] = hstx_dvi_row_pixel_rgb(0,0,0);
    _pallet[1] = hstx_dvi_row_pixel_rgb(255,255,255);

    hstx_dvi_grid_clear();
}

void __not_in_flash_func(hstx_dvi_grid_render_frame)() {
    for(uint32_t k = 0; k < MODE_V_ACTIVE_LINES; k++) {
        hstx_dvi_row_t *r = hstx_dvi_row_buf_get();
        for (uint32_t j = 0; j < CHAR_COLS; j++) {
            const uint32_t s = _screen[k >> 3][j];
            const uint32_t e = decode_char(s);
            const hstx_dvi_pixel_t fgbg[2] = {get_fg_color(s), get_bg_color(s)};
            uint8_t f = font_8x8[(k & 7) + (e << 3)];
            for (uint32_t i = 0; i < 2; ++i) {
                uint32_t p1 = fgbg[(f >> 7) & 1];
                uint32_t p2 = fgbg[(f >> 6) & 1];
                uint32_t p3 = fgbg[(f >> 5) & 1];
                uint32_t p4 = fgbg[(f >> 4) & 1];
                hstx_dvi_row_set_pixel_quad(
                    r, 
                    (j<<1) + i, 
                    p1,p2,p3,p4);
                f <<= 4;
            }
        }
        hstx_dvi_row_fifo_put_blocking(r);
    }
}

void __not_in_flash_func(hstx_dvi_grid_write_ch)(
    const uint32_t y,
    const uint32_t x,
    const char c,
    const uint8_t fgi,
    const uint8_t bgi
) {
    set_char_full(y, x, c, fgi, bgi);
}

void __not_in_flash_func(hstx_dvi_grid_write_str)(
    const uint32_t y,
    const uint32_t x,
    const char *s,
    const uint8_t fgi,
    const uint8_t bgi
) {
    uint32_t j = y;
    uint32_t i = x;
    char *p = (char*)s;
    char c;
    while ((c = *p++)) {
        switch(c) {
            case '\n':
                i = 0;
                j = ++j < CHAR_ROWS ? j : 0;
                break;
            case '\r':
                break;
            default:
                set_char_full(j, i++, c, fgi, bgi);
                break;
        }
    }
}
