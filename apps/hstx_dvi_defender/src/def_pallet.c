#include "def_pallet.h"

// Not const so the palettes live in RAM; the renderer reads them every sprite row
static hstx_dvi_pixel_t _bright[DEF_COLOUR_COUNT] = {
	HSTX_DVI_PIXEL_RGB(255, 0, 0),     // Red
	HSTX_DVI_PIXEL_RGB(255, 128, 0),   // Orange
	HSTX_DVI_PIXEL_RGB(255, 255, 0),   // Yellow
	HSTX_DVI_PIXEL_RGB(0, 255, 0),     // Green
	HSTX_DVI_PIXEL_RGB(0, 255, 255),   // Cyan
	HSTX_DVI_PIXEL_RGB(64, 96, 255),   // Blue
	HSTX_DVI_PIXEL_RGB(200, 0, 255),   // Purple
	HSTX_DVI_PIXEL_RGB(255, 96, 192),  // Pink
	HSTX_DVI_PIXEL_RGB(255, 255, 255), // White
	HSTX_DVI_PIXEL_RGB(128, 128, 128), // Grey
	HSTX_DVI_PIXEL_RGB(200, 100, 32),  // Brown
	HSTX_DVI_PIXEL_RGB(0, 0, 160),     // Navy
};

static hstx_dvi_pixel_t _dim[DEF_COLOUR_COUNT];

void def_pallet_init(void) {
	for (uint32_t i = 0; i < DEF_COLOUR_COUNT; ++i) {
		_dim[i] = hstx_dvi_pixel_dim(_bright[i]);
	}
}

hstx_dvi_pixel_t* __not_in_flash_func(def_pallet)(uint32_t colour) {
	return &_bright[colour < DEF_COLOUR_COUNT ? colour : DEF_WHITE];
}

hstx_dvi_pixel_t* __not_in_flash_func(def_pallet_dim)(uint32_t colour) {
	return &_dim[colour < DEF_COLOUR_COUNT ? colour : DEF_WHITE];
}

hstx_dvi_pixel_t* __not_in_flash_func(def_pallet_rainbow)(uint32_t i) {
	return &_bright[i % DEF_RAINBOW_COUNT];
}
