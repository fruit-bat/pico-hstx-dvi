#pragma once

#include "pico/stdlib.h"
#include "mm_tiles.h"

#ifdef __cplusplus
extern "C" {
#endif

// The house, as tiles
#define MM_MAP_W 88
#define MM_MAP_H 45
#define MM_MAP_PX_W (MM_MAP_W * MM_TILE)
#define MM_MAP_PX_H (MM_MAP_H * MM_TILE)
// The rows of the floors, for the demo and the host
#define MM_MAP_GROUND 35

// What is in each tile
enum {
	MM_C_EMPTY = 0,
	MM_C_SOLID,
	MM_C_PLATFORM, // Can be jumped up through and stood on
	MM_C_GATE, // Solid until its group opens
	MM_C_LEVER,
	MM_C_CANDLE,
	MM_C_PLATE,
	MM_C_LANTERN,
};

// The puzzles, each a group of gates and what opens them
#define MM_GROUPS 10
enum {
	MM_GROUP_NONE = 0,
	MM_GROUP_LEVER, // Cast at the lever
	MM_GROUP_CANDLES, // Light all the candles
	MM_GROUP_PLATE, // Open while something is on the plate
	MM_GROUP_KEY, // Bring the key
};

// What casting at a tile did
enum {
	MM_CAST_NOTHING = 0,
	MM_CAST_LEVER,
	MM_CAST_CANDLE,
	MM_CAST_LAST_CANDLE,
};

// Called for each thing in the house as it is set up. group is set for keys.
typedef void (*MmSpawn_t)(char ch, int32_t cx, int32_t cy, uint32_t group);

// Puts the house back as it was, with every gate shut and candle out
void mm_map_reset(MmSpawn_t spawn);

const uint8_t* mm_map_gfx(void);

uint32_t mm_map_kind(int32_t cx, int32_t cy);
uint32_t mm_map_group(int32_t cx, int32_t cy);
uint32_t mm_map_group_type(uint32_t group);
bool mm_map_group_open(uint32_t group);

// Off the map counts as solid
bool mm_map_solid(int32_t cx, int32_t cy);
bool mm_map_platform(int32_t cx, int32_t cy);
// Whether any of the pixels x0 <= x < x1, y0 <= y < y1 are in a solid tile
bool mm_map_box_solid(int32_t x0, int32_t y0, int32_t x1, int32_t y1);

// Whether a lever has been pulled or a candle or lantern lit
bool mm_map_on(int32_t cx, int32_t cy);
// A spell hitting a tile
uint32_t mm_map_cast(int32_t cx, int32_t cy);
// Plates are only down while a crate is on them
void mm_map_update_plates(void);
// Returns true if the lantern was not lit before
bool mm_map_light_lantern(int32_t cx, int32_t cy);
// Opens a locked door with its key
void mm_map_unlock(uint32_t group);

#ifdef __cplusplus
}
#endif
