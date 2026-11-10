#pragma once

// Bitmap fonts for text: each glyph h rows of (w + 7) / 8 bytes, the leftmost
// pixel in the top bit of the first byte. The glyphs are for printable ASCII,
// from first (32) to 126, and in the larger fonts then the extras: one for
// each of hstx_dvi_font_extras' code points, for character codes 128 to 255.
//
// The larger ones are made by tools/psf2c.py from Linux console fonts (see
// hstx_dvi_fonts.c): Terminus, by Dimitar Zhekov, under the SIL Open Font
// License 1.1, and the VGA font of the IBM PC. Bitmap fonts are public domain
// by nature, as Debian's console-setup puts it, but Terminus's licence asks
// that its name and licence go with it, as here.

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	const char* name;
	uint8_t w, h;         // pixels
	uint8_t first, count; // first character, glyphs
	const uint8_t* bits;
} hstx_dvi_font_t;

#define HSTX_DVI_FONT_ASCII 95
#define HSTX_DVI_FONT_EXTRAS 128

// The extras' Unicode code points, the glyphs after ASCII for character codes
// 128 to 255: the DEC special graphics, double and rounded box lines, blocks,
// arrows, quotes and dashes, and Latin-1's accented letters
extern const uint16_t hstx_dvi_font_extras[HSTX_DVI_FONT_EXTRAS];

// Whether a font has the extras
static inline int hstx_dvi_font_has_extras(const hstx_dvi_font_t* f) {
	return f->count >= HSTX_DVI_FONT_ASCII + HSTX_DVI_FONT_EXTRAS;
}

// The 8x8 font of font_inv.c, ASCII only
extern const hstx_dvi_font_t hstx_dvi_font_8x8;
// Terminus, 8x14, 8x16, 12x24 and 16x32
extern const hstx_dvi_font_t hstx_dvi_font_terminus_8x14;
extern const hstx_dvi_font_t hstx_dvi_font_terminus_8x16;
extern const hstx_dvi_font_t hstx_dvi_font_terminus_12x24;
extern const hstx_dvi_font_t hstx_dvi_font_terminus_16x32;
// The IBM PC's VGA font, 8x16
extern const hstx_dvi_font_t hstx_dvi_font_vga_8x16;

// All of them, by name, e.g. "terminus_8x16", or NULL
const hstx_dvi_font_t* hstx_dvi_font_find(const char* name);

#ifdef __cplusplus
}
#endif
