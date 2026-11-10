#pragma once

#include "pico/stdlib.h"
#include "hstx_dvi_core.h"
#include "hstx_dvi_font.h"
#include "vt_emu.h"
#ifdef __cplusplus
extern "C" {
#endif

void hstx_dvi_vt_init_all();
void hstx_dvi_vt_render_loop();

void hstx_dvi_vt_init();
void hstx_dvi_vt_render_frame(uint32_t frame_index);
vt_emu_t* hstx_dvi_vt_emu_get();

// The font, drawn sx times as wide and sy times as tall (1 or 2 each), and so
// the size of the terminal: as many characters as fit on the display, which
// it is resized to, and the most the host can ask for. A cell, the font's
// width times sx, has to be a multiple of 4 pixels and no more than 32.
// Returns false, and changes nothing, if the font can't be used.
//
// Call it from the core that calls vt_emu_put_ch(). The display changes at
// the start of the next frame. At 640x480:
//
//   8x8             80 x 60    8x8 x2 tall   80 x 30    8x8 x2       40 x 30
//   terminus_8x14   80 x 34    terminus_8x16 80 x 30    vga_8x16     80 x 30
//   terminus_12x24  53 x 20    terminus_16x32 40 x 15   8x16 x2      40 x 15
//
// The 8x8 font, unscaled, is the default.
bool hstx_dvi_vt_set_font(const hstx_dvi_font_t* font, uint32_t sx, uint32_t sy);

// The characters that fit with the font in use
uint32_t hstx_dvi_vt_cols(void);
uint32_t hstx_dvi_vt_rows(void);

#ifdef __cplusplus
}
#endif
