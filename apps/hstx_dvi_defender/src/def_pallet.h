#pragma once

#include "hstx_dvi_core.h"

#ifdef __cplusplus
extern "C" {
#endif

// The first 8 colours go round the rainbow
typedef enum {
	DEF_RED = 0,
	DEF_ORANGE,
	DEF_YELLOW,
	DEF_GREEN,
	DEF_CYAN,
	DEF_BLUE,
	DEF_PURPLE,
	DEF_PINK,
	DEF_WHITE,
	DEF_GREY,
	DEF_BROWN,
	DEF_NAVY,
	DEF_COLOUR_COUNT
} DefColour_e;

#define DEF_RAINBOW_COUNT 8
// Pass as a colour to get a random colour for each particle
#define DEF_COLOUR_RANDOM 0xff

void def_pallet_init(void);

hstx_dvi_pixel_t* def_pallet(uint32_t colour);
hstx_dvi_pixel_t* def_pallet_dim(uint32_t colour);
hstx_dvi_pixel_t* def_pallet_rainbow(uint32_t i);

#ifdef __cplusplus
}
#endif
