#pragma once

#include "hstx_dvi_core.h"

#ifdef __cplusplus
extern "C" {
#endif

// The first 8 colours go round the rainbow
typedef enum {
	GAME_RED = 0,
	GAME_ORANGE,
	GAME_YELLOW,
	GAME_GREEN,
	GAME_CYAN,
	GAME_BLUE,
	GAME_PURPLE,
	GAME_PINK,
	GAME_WHITE,
	GAME_GREY,
	GAME_BROWN,
	GAME_NAVY,
	GAME_BLACK,
	GAME_PEACH,
	GAME_DARK_BROWN,
	GAME_COLOUR_COUNT
} GameColour_e;

#define GAME_RAINBOW_COUNT 8
// Pass as a colour to get a random colour for each particle
#define GAME_COLOUR_RANDOM 0xff

void game_pallet_init(void);

hstx_dvi_pixel_t* game_pallet(uint32_t colour);
hstx_dvi_pixel_t* game_pallet_dim(uint32_t colour);
hstx_dvi_pixel_t* game_pallet_rainbow(uint32_t i);

#ifdef __cplusplus
}
#endif
