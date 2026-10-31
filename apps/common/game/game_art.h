#pragma once

#include "pico/stdlib.h"
#include "hstx_dvi_sprite.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GAME_ART_LAYERS 4
#define GAME_ART_MAX_H 16
#define GAME_ART_ROWS(rows) (sizeof(rows) / sizeof((rows)[0]))

// A picture made of up to GAME_ART_LAYERS sprites, each a different colour
typedef union {
	Tile16x16p2_t t16;
	Tile32x16p2_t t32;
} GameArtTile_t;

typedef struct {
	uint8_t w;
	uint8_t h;
	uint8_t layers;
	uint8_t colour[GAME_ART_LAYERS];
	GameArtTile_t tile[GAME_ART_LAYERS];
} GameArt_t;

// Makes a picture from rows of characters, 16 or 32 to a row. Each character
// in key is a layer drawn in the matching colour; anything else is see through.
void game_art_make(GameArt_t* a, const char* const* rows, uint32_t h, const char* key, const uint8_t* colours);

// Makes a left to right mirror image
void game_art_mirror(GameArt_t* dst, const GameArt_t* src);

// colours, one per layer, may be NULL to use the picture's own
void game_art_draw(const GameArt_t* a, int32_t sx, int32_t sy, const uint8_t* colours);

// Shows a picture on sprites first to first + n - 1, for a game that keeps
// its own sprites rather than having them handed out each frame. Layers the
// picture does not have are disabled.
void game_art_sprites(SpriteId first, uint32_t n, const GameArt_t* a, int32_t sx, int32_t sy, const uint8_t* colours);
void game_art_sprites_hide(SpriteId first, uint32_t n);

// For putting a picture together from parts before making it
typedef struct {
	uint32_t w;
	uint32_t h;
	char c[GAME_ART_MAX_H][33];
	const char* rows[GAME_ART_MAX_H];
} GameArtCanvas_t;

// Starts off see through, w up to 32 and h up to GAME_ART_MAX_H
void game_art_canvas_clear(GameArtCanvas_t* c, uint32_t w, uint32_t h);
// Copies rows of characters on to the canvas at x, y. '.' leaves the canvas
// as it was and ' ' makes it see through again. Anything off the canvas is lost.
void game_art_canvas_stamp(GameArtCanvas_t* c, const char* const* rows, uint32_t n, int32_t x, int32_t y);
void game_art_canvas_set(GameArtCanvas_t* c, int32_t x, int32_t y, char ch);
void game_art_canvas_make(GameArt_t* a, GameArtCanvas_t* c, const char* key, const uint8_t* colours);

#ifdef __cplusplus
}
#endif
