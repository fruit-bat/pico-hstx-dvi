#pragma once

// Bitmap fonts for text: each glyph h rows of (w + 7) / 8 bytes, the leftmost
// pixel in the top bit of the first byte, for count characters from first.
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
	uint8_t first, count; // characters
	const uint8_t* bits;
} hstx_dvi_font_t;

// The 8x8 font of font_inv.c
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
