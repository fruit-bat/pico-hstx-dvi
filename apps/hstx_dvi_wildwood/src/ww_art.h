#pragma once

#include "pico/stdlib.h"
#include "game_art.h"

#ifdef __cplusplus
extern "C" {
#endif

// Pictures up to 32 pixels tall, made of a top and bottom half each with
// up to GAME_ART_LAYERS colours of its own
#define WW_ART_MAX_W 32
#define WW_ART_MAX_H 32
#define WW_ART_KEY_MAX 8

typedef struct {
	uint8_t w;
	uint8_t h;
	uint8_t parts;
	// Which character of the key each layer of each half was made from
	uint8_t key[2][GAME_ART_LAYERS];
	GameArt_t part[2];
} WwArt_t;

typedef struct {
	uint32_t w;
	uint32_t h;
	char c[WW_ART_MAX_H][WW_ART_MAX_W + 1];
} WwCanvas_t;

// As the game_art canvas: starts see through, '.' in a stamp leaves the
// canvas as it was and ' ' makes it see through again
void ww_canvas_clear(WwCanvas_t* c, uint32_t w, uint32_t h);
void ww_canvas_stamp(WwCanvas_t* c, const char* const* rows, uint32_t n, int32_t x, int32_t y);
void ww_canvas_set(WwCanvas_t* c, int32_t x, int32_t y, char ch);
// Flips the canvas left to right
void ww_canvas_mirror(WwCanvas_t* c);

// key has up to WW_ART_KEY_MAX layer characters, with a colour for each.
// Each half only uses the ones it has in it, up to GAME_ART_LAYERS.
void ww_art_make(WwArt_t* a, const WwCanvas_t* c, const char* key, const uint8_t* colours);

// Nothing is drawn above this screen row, so pictures can go under the HUD
void ww_art_set_clip(int32_t top);

// sx, sy is the top left. colours, one for each character of the key, may be
// NULL to use the picture's own.
void ww_art_draw(const WwArt_t* a, int32_t sx, int32_t sy, const uint8_t* colours);
// The same for an ordinary picture, clipped in the same way
void ww_art_draw_small(const GameArt_t* a, int32_t sx, int32_t sy, const uint8_t* colours);

#ifdef __cplusplus
}
#endif
