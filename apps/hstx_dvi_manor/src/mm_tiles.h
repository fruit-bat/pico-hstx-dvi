#pragma once

#include "pico/stdlib.h"
#include "game_render.h"

#ifdef __cplusplus
extern "C" {
#endif

// The house is drawn from 32 pixel square tiles, each made from a 16 pixel
// picture drawn twice the size, scrolling under the HUD
#define MM_TILE 32
#define MM_TILE_SHIFT 5
#define MM_HUD_H 32
#define MM_VIEW_W GAME_SCREEN_W
#define MM_VIEW_H (GAME_SCREEN_H - MM_HUD_H)
#define MM_VIEW_COLS (MM_VIEW_W / MM_TILE)
#define MM_VIEW_ROWS (MM_VIEW_H / MM_TILE)

// The wallpaper behind everything
enum {
	MM_STYLE_SKY = 0,
	MM_STYLE_ATTIC,
	MM_STYLE_UPPER,
	MM_STYLE_GROUND,
	MM_STYLE_CELLAR,
	MM_STYLE_COUNT
};

// What is drawn over the wallpaper
enum {
	MM_DECOR_NONE = 0,
	MM_DECOR_STONE,
	MM_DECOR_FLOOR,
	MM_DECOR_ROOF,
	MM_DECOR_GRASS,
	MM_DECOR_EARTH,
	MM_DECOR_SHELF,
	MM_DECOR_TABLE_L,
	MM_DECOR_TABLE_M,
	MM_DECOR_TABLE_R,
	MM_DECOR_BED_L,
	MM_DECOR_BED_M,
	MM_DECOR_BED_R,
	MM_DECOR_WINDOW_TOP,
	MM_DECOR_WINDOW_BOTTOM,
	MM_DECOR_PORTRAIT,
	MM_DECOR_COBWEB_L,
	MM_DECOR_COBWEB_R,
	MM_DECOR_BOOKS,
	MM_DECOR_DOORWAY_TOP,
	MM_DECOR_DOORWAY,
	MM_DECOR_LANTERN,
	MM_DECOR_LANTERN_LIT,
	MM_DECOR_CANDLE,
	MM_DECOR_CANDLE_LIT,
	MM_DECOR_LEVER,
	MM_DECOR_LEVER_ON,
	MM_DECOR_PLATE,
	MM_DECOR_PLATE_DOWN,
	MM_DECOR_BARS,
	MM_DECOR_TRAPDOOR,
	MM_DECOR_DOOR_TOP,
	MM_DECOR_DOOR_MID,
	MM_DECOR_DOOR_BOTTOM,
	MM_DECOR_COUNT
};

void mm_tiles_init(void);

// The picture for a tile, made the first time it is asked for. shadow darkens
// the top, for under a ceiling.
uint8_t mm_tiles_get(uint32_t style, uint32_t decor, bool shadow);

// Draws the tiles in gfx, a map stride tiles wide, with cam_x, cam_y the
// world pixel at the top left of the view. It draws straight into the rows,
// so has to go first.
void mm_tiles_draw(const uint8_t* gfx, uint32_t stride, int32_t cam_x, int32_t cam_y, uint32_t frame);

#ifdef __cplusplus
}
#endif
