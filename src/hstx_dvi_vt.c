#include "hstx_dvi_vt.h"
#include "hstx_dvi_core.h"
#include "hstx_dvi_row_fifo.h"
#include "hstx_dvi_row_buf.h"
#include "pico/multicore.h"
#include "pico/stdio.h"
#include "pico/stdlib.h"
#include <stdio.h>
#include <string.h>
#include "vt_emu.h"
#include "vt_pallet.h"
#include "vt_charmap.h"

// The smallest cell, the 8x8 font unscaled, sets how many characters there
// can be on the display
#define MIN_CELL_W 8
#define MIN_CELL_H 8
#define CHAR_ROWS (MODE_V_ACTIVE_LINES / MIN_CELL_H)
#define CHAR_COLS (MODE_H_ACTIVE_PIXELS / MIN_CELL_W)

// The most the terminal can be (its capacity): what fits on the display, as
// far as the emulator allows. The terminal can be resized up to this, or what
// fits with the font in use if less, and the rest of the display is drawn in
// the background colour.
#ifndef MIN
#define MIN(x, y) (((x) < (y)) ? (x) : (y))
#endif
#define CAP_ROWS MIN(CHAR_ROWS, VT_SCREEN_MAX_ROWS)
#define CAP_COLS MIN(CHAR_COLS, VT_SCREEN_MAX_COLS)

// The font in use is copied into RAM, so core 1 never waits for the flash:
// up to ASCII and the extras, at 16x32
#define FONT_RAM_BYTES ((HSTX_DVI_FONT_ASCII + HSTX_DVI_FONT_EXTRAS) * 32 * 2)
#define CELL_W_MAX 32

static hstx_dvi_pixel_t _pallet[256];
static vt_cell_t _screen[CAP_ROWS][CAP_COLS];
static vt_emu_t vt_emu;

// The font asked for by core 0, taken by core 1 at the start of a frame
typedef struct {
    const hstx_dvi_font_t* font;
    uint32_t sx, sy;
} vt_font_use_t;
static vt_font_use_t _want;
static volatile uint32_t _want_seq = 0;
// What fits with it, and whether it has the extras, for core 0
static uint32_t _fit_cols, _fit_rows;
static bool _extras = false;
// Whether a resize request picks the font
static bool _auto = false;

// The fonts tried for a resize request, biggest first, the first that fits
// the size asked for being used
typedef struct {
    const hstx_dvi_font_t* font;
    uint8_t sx, sy;
} vt_auto_font_t;
static const vt_auto_font_t _auto_fonts[] = {
    { &hstx_dvi_font_terminus_16x32, 2, 2 },
    { &hstx_dvi_font_terminus_12x24, 2, 2 },
    { &hstx_dvi_font_terminus_16x32, 1, 1 },
    { &hstx_dvi_font_terminus_12x24, 1, 1 },
    { &hstx_dvi_font_terminus_8x16, 1, 1 },
    { &hstx_dvi_font_terminus_8x14, 1, 1 },
    { &hstx_dvi_font_8x8, 1, 1 },
};

// The font in use, core 1's
static uint8_t __attribute__((aligned(4))) _font_ram[FONT_RAM_BYTES];
static uint32_t _seq = 0;
static uint32_t _first, _count, _fh, _bpr, _sx, _sy;
static uint32_t _cell_w, _cell_h, _cols, _rows;

// Each bit doubled, for twice as wide
static uint16_t _double_bits[256];
#if MODE_BYTES_PER_PIXEL == 1
// For 4 bits of a glyph, leftmost the top, the bytes of 4 pixels that are set
static uint32_t _nibble_mask[16];
#endif

vt_emu_t* __not_in_flash_func(hstx_dvi_vt_emu_get)() {
    return &vt_emu;
}

static void vt_pallet_make_cb(
    void *d,
    vt_cell_colour_t i,
    uint32_t r,
    uint32_t g,
    uint32_t b
) {
    _pallet[i] = hstx_dvi_pixel_rgb(r,g,b);
}

// Replies (e.g. size and cursor position reports) go back to the host
static void vt_reply_cb(void *ctx, const vt_char_t *s, uint32_t n) {
    (void)ctx;
    for (uint32_t i = 0; i < n; ++i) putchar_raw(s[i]);
}

static uint32_t font_bytes(const hstx_dvi_font_t* f) {
    return (uint32_t)f->count * f->h * ((f->w + 7) >> 3);
}

// Asks core 1 for the font, without resizing the terminal
static bool use_font(const hstx_dvi_font_t* font, uint32_t sx, uint32_t sy) {
    if (!font || sx < 1 || sx > 2 || sy < 1 || sy > 2) return false;
    const uint32_t cw = font->w * sx;
    if (cw & 3 || cw > CELL_W_MAX || font_bytes(font) > FONT_RAM_BYTES) return false;
    if (font != _want.font || sx != _want.sx || sy != _want.sy) {
        _want.font = font;
        _want.sx = sx;
        _want.sy = sy;
        __dmb();
        ++_want_seq;
    }
    _fit_cols = MIN(MODE_H_ACTIVE_PIXELS / cw, CAP_COLS);
    _fit_rows = MIN(MODE_V_ACTIVE_LINES / (font->h * sy), CAP_ROWS);
    _extras = hstx_dvi_font_has_extras(font);
    vt_emu_set_cell_size(&vt_emu, cw, font->h * sy);
    return true;
}

// The host asked for a new size (ESC [ 8 ; h ; w t): allow it, within what
// fits with the font in use, or first pick the biggest font it fits with
static void vt_resize_req_cb(void *ctx, vt_coord_t w, vt_coord_t h) {
    if (_auto) {
        for (uint32_t i = 0; i < sizeof(_auto_fonts) / sizeof(_auto_fonts[0]); ++i) {
            const vt_auto_font_t* a = &_auto_fonts[i];
            if ((uint32_t)w * a->font->w * a->sx <= MODE_H_ACTIVE_PIXELS
                && (uint32_t)h * a->font->h * a->sy <= MODE_V_ACTIVE_LINES) {
                use_font(a->font, a->sx, a->sy);
                break;
            }
        }
    }
    vt_emu_resize((vt_emu_t*)ctx, MIN(w, _fit_cols), MIN(h, _fit_rows));
}

// Unicode to the glyphs in the cells: ASCII as it is, and the extras, if the
// font has them, as 128 on. Anything else as near as ASCII gets.
static vt_char_t vt_charmap_cb(uint32_t cp) {
    if (cp >= 0x20 && cp < 0x7f) return (vt_char_t)cp;
    if (_extras) {
        for (uint32_t i = 0; i < HSTX_DVI_FONT_EXTRAS; ++i) {
            if (hstx_dvi_font_extras[i] == cp) return (vt_char_t)(128 + i);
        }
    }
    return vt_charmap_ascii(cp);
}

// Core 1 takes up the font asked for, if it has changed
static void __not_in_flash_func(take_font)(void) {
    const uint32_t seq = _want_seq;
    if (seq == _seq) return;
    __dmb();
    const hstx_dvi_font_t* f = _want.font;
    memcpy(_font_ram, f->bits, font_bytes(f));
    _first = f->first;
    _count = f->count;
    _fh = f->h;
    _bpr = (f->w + 7) >> 3;
    _sx = _want.sx;
    _sy = _want.sy;
    _cell_w = f->w * _sx;
    _cell_h = f->h * _sy;
    _cols = MIN(MODE_H_ACTIVE_PIXELS / _cell_w, CAP_COLS);
    _rows = MIN(MODE_V_ACTIVE_LINES / _cell_h, CAP_ROWS);
    _seq = seq;
}

bool hstx_dvi_vt_set_font(const hstx_dvi_font_t* font, uint32_t sx, uint32_t sy) {
    if (!use_font(font, sx, sy)) return false;
    vt_emu_resize(&vt_emu, _fit_cols, _fit_rows);
    return true;
}

void hstx_dvi_vt_set_font_auto(bool on) {
    _auto = on;
}

uint32_t hstx_dvi_vt_cols(void) {
    return _fit_cols;
}

uint32_t hstx_dvi_vt_rows(void) {
    return _fit_rows;
}

void hstx_dvi_vt_init() {

    // Setup the pallet
    vt_pallet_make_256_colours(0, vt_pallet_make_cb);

    for (uint32_t d = 0; d < 256; ++d) {
        uint32_t e = 0;
        for (uint32_t b = 0; b < 8; ++b) {
            if (d & (0x80 >> b)) e |= 0xc000u >> (b << 1);
        }
        _double_bits[d] = (uint16_t)e;
    }
#if MODE_BYTES_PER_PIXEL == 1
    for (uint32_t n = 0; n < 16; ++n) {
        uint32_t m = 0;
        for (uint32_t b = 0; b < 4; ++b) {
            if (n & (8 >> b)) m |= 0xffu << (b << 3);
        }
        _nibble_mask[n] = m;
    }
#endif

    // Setup the terminal emulator, filling the display to start with
    vt_emu_init_cap(&vt_emu, (vt_cell_t*)_screen, CAP_COLS, CAP_ROWS, CAP_COLS, CAP_ROWS);
    vt_emu_set_reply(&vt_emu, vt_reply_cb, NULL);
    vt_emu_set_resize_req(&vt_emu, vt_resize_req_cb, &vt_emu);
    vt_emu_set_charmap(&vt_emu, vt_charmap_cb);
    hstx_dvi_vt_set_font(&hstx_dvi_font_8x8, 1, 1);
    // Core 1 isn't running yet, so take it up here
    take_font();

    // Say hello
    vt_emu_put_str(&vt_emu, (vt_char_t*)"Pico virtual terminal v0.1\r\n");
    vt_emu_put_str(&vt_emu, (vt_char_t*)"Provided by fruit-bat 2025\r\n\r\n");
}

// The pixels of a glyph's row, from the left in the top bit, sx times as wide
static inline uint32_t __not_in_flash_func(glyph_bits)(const uint8_t* g) {
    if (_bpr == 1) {
        return _sx == 2 ? (uint32_t)_double_bits[g[0]] << 16 : (uint32_t)g[0] << 24;
    }
    return _sx == 2
        ? ((uint32_t)_double_bits[g[0]] << 16) | _double_bits[g[1]]
        : ((uint32_t)g[0] << 24) | ((uint32_t)g[1] << 16);
}

void __not_in_flash_func(hstx_dvi_vt_render_frame)(uint32_t frame_index) {
    take_font();
    const bool blink = (frame_index & 63) < 32; // Blink every second for 32 frames
    vt_cell_t** srps = (vt_cell_t**)&vt_emu.term.rp;
    const vt_coord_t cc = vt_emu.term.c;
    const vt_coord_t cr = vt_emu.term.r;
    // The host can hide the cursor (DECTCEM, ESC [ ? 25 l)
    const bool cvis = vt_term_mode_get(&vt_emu.term, VT_TERM_MODE_CURSOR);
    // The terminal's size, read once a frame as core 0 may resize it. Every
    // row of the capacity is valid so a resize part done shows an odd frame
    // at worst; the clamps make sure of that, and that it fits the font.
    const uint32_t tw = MIN(vt_emu.term.w, _cols);
    const uint32_t th = MIN(vt_emu.term.h, _rows);
    const hstx_dvi_pixel_t border = _pallet[VT_TERM_DEFAULT_BG];
    const uint32_t quads = MODE_H_ACTIVE_PIXELS >> 2;
    const uint32_t cell_quads = _cell_w >> 2;
    uint32_t row = 0, in_cell = 0;
    for(uint32_t k = 0; k < MODE_V_ACTIVE_LINES; k++) {
        hstx_dvi_row_t *r = hstx_dvi_row_buf_get();
        const uint32_t cols = row < th ? tw : 0;
        // The row of the glyph, and whether it is the cell's last, where
        // underlines go
        const uint32_t gy = _sy == 2 ? in_cell >> 1 : in_cell;
        const bool bottom = in_cell == _cell_h - 1;
        vt_cell_t* srp = srps[MIN(row, CAP_ROWS - 1)];
        uint32_t q = 0;
        for (uint32_t j = 0; j < cols; j++) {
            const vt_cell_t s = srp[j];
            const vt_cell_attr_t attr = vt_cell_get_attr(s);
            const vt_cell_flags_t flags = vt_cell_flags_get(attr);
            const bool cup = cvis && row == cr && j == cc;
            const bool rev1 = ((flags & VT_CELL_FLAGS_BLINK) || cup) && blink;
            const bool rev2 = (flags & VT_CELL_FLAGS_REVERSE) || cup;
            hstx_dvi_pixel_t fg, bg;
            if (rev1 != rev2) {
                bg = _pallet[vt_cell_fg_get(s)];
                fg = _pallet[vt_cell_bg_get(s)];
            }
            else {
                bg = _pallet[vt_cell_bg_get(s)];
                fg = _pallet[vt_cell_fg_get(s)];
            }
            if (flags & VT_CELL_FLAGS_DIM) {
                bg = hstx_dvi_pixel_dim(bg);
                fg = hstx_dvi_pixel_dim(fg);
            }
            if (flags & VT_CELL_FLAGS_INVISIBLE) {
                fg = bg;
            }
            uint32_t m;
            if (bottom && (flags & VT_CELL_FLAGS_UNDERLINE)) {
                // A solid line along the bottom of the cell
                m = 0xffffffffu;
            }
            else {
                // ASCII, then the extras for 128 on
                const uint32_t c = vt_cell_get_char(s);
                uint32_t e = c < 128 ? c - _first : c - 128 + HSTX_DVI_FONT_ASCII;
                if (e >= _count) e = '?' - _first;
                m = glyph_bits(&_font_ram[(e * _fh + gy) * _bpr]);
            }
#if MODE_BYTES_PER_PIXEL == 1
            const uint32_t f4 = fg * 0x01010101u, b4 = bg * 0x01010101u;
            for (uint32_t i = 0; i < cell_quads; ++i) {
                const uint32_t n = _nibble_mask[m >> 28];
                r->w[q++] = (f4 & n) | (b4 & ~n);
                m <<= 4;
            }
#else
            for (uint32_t i = 0; i < cell_quads; ++i) {
                hstx_dvi_row_set_pixel_quad(r, q++,
                    (m >> 31) & 1 ? fg : bg, (m >> 30) & 1 ? fg : bg,
                    (m >> 29) & 1 ? fg : bg, (m >> 28) & 1 ? fg : bg);
                m <<= 4;
            }
#endif
        }
        // Outside the terminal is the background colour
        for (; q < quads; ++q) {
            hstx_dvi_row_set_pixel_quad(r, q, border, border, border, border);
        }
        hstx_dvi_row_fifo_put_blocking(r);
        if (++in_cell == _cell_h) {
            in_cell = 0;
            ++row;
        }
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
