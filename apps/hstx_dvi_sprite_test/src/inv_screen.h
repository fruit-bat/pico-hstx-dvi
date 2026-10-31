#pragma once

#include "pico/stdlib.h"
#include "hstx_dvi_sprite.h"

#ifdef __cplusplus
extern "C" {
#endif

#define INV_SCREEN_MAX_SLOTS 16
#define INV_SCREEN_TEXT_MAX 40

// Disables all the sprites and clears their collision masks, ready to set up a new screen
void inv_screen_clear(void);

// Allocates a sprite for each of the text slots; they start hidden
SpriteId inv_screen_init(SpriteId start, uint32_t slots);

// Shows a line of text in a slot. Big text is drawn at twice the size.
void inv_screen_text(uint32_t slot, int32_t x, int32_t y, const char* s, hstx_dvi_pixel_t* p, bool big);

// As inv_screen_text but centred on the screen
void inv_screen_text_centre(uint32_t slot, int32_t y, const char* s, hstx_dvi_pixel_t* p, bool big);

// Shows just the first n characters of a slot's text, for typing it out
void inv_screen_text_reveal(uint32_t slot, uint32_t n);

uint32_t inv_screen_text_len(uint32_t slot);

void inv_screen_text_show(uint32_t slot, bool show);

void inv_screen_text_pallet(uint32_t slot, hstx_dvi_pixel_t* p);

#ifdef __cplusplus
}
#endif
