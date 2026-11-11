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
//
// Each larger font can be left out when built, trading quality for memory:
// HSTX_DVI_FONT_TERMINUS_8X14 and the like are 1 (built) unless defined as 0,
// which the CMake options of the same names do (see CMakeLists.txt). The 8x8
// font is always there. A font built takes this much flash, with the extras:
//
//   terminus_8x14    3.0 KB      terminus_12x24  10.5 KB
//   terminus_8x16    3.5 KB      terminus_16x32  13.9 KB
//   vga_8x16         3.5 KB
//
// The virtual terminal also copies the font in use into RAM, so needs RAM
// for the biggest font built (HSTX_DVI_FONT_MAX_BYTES): 13.9 KB with
// terminus_16x32, 10.5 KB with terminus_12x24 the biggest, 3.5 KB with only
// the 8x16s, 0.7 KB with only the 8x8 font.

#include <stdint.h>

#ifndef HSTX_DVI_FONT_TERMINUS_8X14
#define HSTX_DVI_FONT_TERMINUS_8X14 1
#endif
#ifndef HSTX_DVI_FONT_TERMINUS_8X16
#define HSTX_DVI_FONT_TERMINUS_8X16 1
#endif
#ifndef HSTX_DVI_FONT_TERMINUS_12X24
#define HSTX_DVI_FONT_TERMINUS_12X24 1
#endif
#ifndef HSTX_DVI_FONT_TERMINUS_16X32
#define HSTX_DVI_FONT_TERMINUS_16X32 1
#endif
#ifndef HSTX_DVI_FONT_VGA_8X16
#define HSTX_DVI_FONT_VGA_8X16 1
#endif

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

// The bytes of a font's glyphs, w x h with count glyphs
#define HSTX_DVI_FONT_BYTES(w, h, count) ((count) * (h) * (((w) + 7) / 8))
// The bytes of the biggest font built
#if HSTX_DVI_FONT_TERMINUS_16X32
#define HSTX_DVI_FONT_MAX_BYTES HSTX_DVI_FONT_BYTES(16, 32, HSTX_DVI_FONT_ASCII + HSTX_DVI_FONT_EXTRAS)
#elif HSTX_DVI_FONT_TERMINUS_12X24
#define HSTX_DVI_FONT_MAX_BYTES HSTX_DVI_FONT_BYTES(12, 24, HSTX_DVI_FONT_ASCII + HSTX_DVI_FONT_EXTRAS)
#elif HSTX_DVI_FONT_TERMINUS_8X16 || HSTX_DVI_FONT_VGA_8X16
#define HSTX_DVI_FONT_MAX_BYTES HSTX_DVI_FONT_BYTES(8, 16, HSTX_DVI_FONT_ASCII + HSTX_DVI_FONT_EXTRAS)
#elif HSTX_DVI_FONT_TERMINUS_8X14
#define HSTX_DVI_FONT_MAX_BYTES HSTX_DVI_FONT_BYTES(8, 14, HSTX_DVI_FONT_ASCII + HSTX_DVI_FONT_EXTRAS)
#else
#define HSTX_DVI_FONT_MAX_BYTES HSTX_DVI_FONT_BYTES(8, 8, HSTX_DVI_FONT_ASCII)
#endif

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
// Terminus, 8x14, 8x16, 12x24 and 16x32, each if built
#if HSTX_DVI_FONT_TERMINUS_8X14
extern const hstx_dvi_font_t hstx_dvi_font_terminus_8x14;
#endif
#if HSTX_DVI_FONT_TERMINUS_8X16
extern const hstx_dvi_font_t hstx_dvi_font_terminus_8x16;
#endif
#if HSTX_DVI_FONT_TERMINUS_12X24
extern const hstx_dvi_font_t hstx_dvi_font_terminus_12x24;
#endif
#if HSTX_DVI_FONT_TERMINUS_16X32
extern const hstx_dvi_font_t hstx_dvi_font_terminus_16x32;
#endif
// The IBM PC's VGA font, 8x16, if built
#if HSTX_DVI_FONT_VGA_8X16
extern const hstx_dvi_font_t hstx_dvi_font_vga_8x16;
#endif

// Those built, by name, e.g. "terminus_8x16", or NULL if not built
const hstx_dvi_font_t* hstx_dvi_font_find(const char* name);

#ifdef __cplusplus
}
#endif
