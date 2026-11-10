// See hstx_dvi_font.h.

#include "hstx_dvi_font.h"
#include "font_inv.h"
#include <string.h>
#include <stddef.h>

const hstx_dvi_font_t hstx_dvi_font_8x8 = { "8x8", 8, 8, 32, 95, (const uint8_t*)font_8x8 };

static const hstx_dvi_font_t* const _fonts[] = {
	&hstx_dvi_font_8x8,
	&hstx_dvi_font_terminus_8x14,
	&hstx_dvi_font_terminus_8x16,
	&hstx_dvi_font_terminus_12x24,
	&hstx_dvi_font_terminus_16x32,
	&hstx_dvi_font_vga_8x16,
};

const hstx_dvi_font_t* hstx_dvi_font_find(const char* name) {
	for (size_t i = 0; i < sizeof(_fonts) / sizeof(_fonts[0]); ++i) {
		if (!strcmp(_fonts[i]->name, name)) return _fonts[i];
	}
	return NULL;
}
