#pragma once

#include "pico/stdlib.h"
#include "hstx_dvi_sprite.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DEF_ART_LAYERS 4
#define DEF_ART_MAX_H 16
#define DEF_ART_ROWS(rows) (sizeof(rows) / sizeof((rows)[0]))

// A picture made of up to DEF_ART_LAYERS sprites, each a different colour
typedef union {
	Tile16x16p2_t t16;
	Tile32x16p2_t t32;
} DefArtTile_t;

typedef struct {
	uint8_t w;
	uint8_t h;
	uint8_t layers;
	uint8_t colour[DEF_ART_LAYERS];
	DefArtTile_t tile[DEF_ART_LAYERS];
} DefArt_t;

// Makes a picture from rows of characters, 16 or 32 to a row. Each character
// in key is a layer drawn in the matching colour; anything else is see through.
void def_art_make(DefArt_t* a, const char* const* rows, uint32_t h, const char* key, const uint8_t* colours);

// Makes a left to right mirror image
void def_art_mirror(DefArt_t* dst, const DefArt_t* src);

// colours, one per layer, may be NULL to use the picture's own
void def_art_draw(const DefArt_t* a, int32_t sx, int32_t sy, const uint8_t* colours);

#ifdef __cplusplus
}
#endif
