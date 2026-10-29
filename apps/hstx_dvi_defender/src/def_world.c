#include "def_world.h"
#include "game_pallet.h"
#include "game_util.h"
#include <string.h>

#define DEF_TERRAIN_BASE (GAME_SCREEN_H - 4)
#define DEF_TERRAIN_MIN 4
#define DEF_TERRAIN_MAX 60
// Room left at the end of the world to bring the mountains back round to the start
#define DEF_TERRAIN_JOIN 96
#define DEF_STAR_COUNT 40
// The stars repeat every this many pixels
#define DEF_STAR_WRAP 1024

typedef struct {
	uint16_t x;
	uint16_t y;
	uint8_t c;
} DefStar_t;

static uint8_t _height[DEF_WORLD_W];
// Drawn into one while the other is being shown
static uint32_t _bits[2][DEF_TERRAIN_ROWS * GAME_BITMAP_WORDS];
static uint32_t _bits_i = 0;
static int32_t _camera = 0;
static bool _planet = true;
static DefStar_t _stars[DEF_STAR_COUNT];

static Tile8x8p2_t _tile_star = {{ 0b10000000 }};

void def_world_init(void) {
	// Mountains made of slopes and plateaus
	int32_t h = 20;
	uint32_t x = 0;
	while (x < DEF_WORLD_W - DEF_TERRAIN_JOIN) {
		const uint32_t r = game_rand() % 8;
		const int32_t slope = r < 3 ? 1 : r < 6 ? -1 : 0;
		const uint32_t len = 8 + game_rand() % 40;
		for (uint32_t i = 0; i < len && x < DEF_WORLD_W - DEF_TERRAIN_JOIN; ++i) {
			h = game_clamp(h + slope, DEF_TERRAIN_MIN, DEF_TERRAIN_MAX);
			_height[x++] = h;
		}
	}
	// Join back up with the start
	while (x < DEF_WORLD_W) {
		h += game_sign(_height[0] - h);
		_height[x++] = h;
	}

	for (uint32_t i = 0; i < DEF_STAR_COUNT; ++i) {
		_stars[i].x = game_rand() % DEF_STAR_WRAP;
		_stars[i].y = DEF_PLAY_TOP + game_rand() % (DEF_TERRAIN_TOP - DEF_PLAY_TOP - 40);
		_stars[i].c = game_rand();
	}
	_planet = true;
}

void def_world_set_camera(int32_t x) {
	_camera = def_wrap(x);
}

int32_t __not_in_flash_func(def_world_camera)(void) {
	return _camera;
}

int32_t __not_in_flash_func(def_world_screen_x)(int32_t x) {
	return def_dx(x, _camera) >> DEF_FP;
}

bool __not_in_flash_func(def_world_on_screen)(int32_t x, int32_t w) {
	const int32_t sx = def_world_screen_x(x);
	return sx > -w && sx < GAME_SCREEN_W;
}

int32_t __not_in_flash_func(def_world_ground_y)(int32_t x) {
	if (!_planet) return GAME_SCREEN_H - 2;
	return DEF_TERRAIN_BASE - _height[(def_wrap(x) >> DEF_FP) & (DEF_WORLD_W - 1)];
}

void def_world_destroy_planet(void) {
	_planet = false;
}

void def_world_restore_planet(void) {
	_planet = true;
}

bool def_world_planet_alive(void) {
	return _planet;
}

void __not_in_flash_func(def_world_draw_terrain)(void) {
	if (!_planet) return;
	_bits_i ^= 1;
	uint32_t* b = _bits[_bits_i];
	memset(b, 0, sizeof(_bits[0]));
	const uint32_t cx = _camera >> DEF_FP;
	for (uint32_t sx = 0; sx < GAME_SCREEN_W; ++sx) {
		const uint32_t row = DEF_TERRAIN_BASE - DEF_TERRAIN_TOP - _height[(cx + sx) & (DEF_WORLD_W - 1)];
		b[row * GAME_BITMAP_WORDS + (sx >> 5)] |= 0x80000000u >> (sx & 31);
	}
	game_spr_bitmap(DEF_TERRAIN_TOP, DEF_TERRAIN_ROWS, b, game_pallet(GAME_BROWN));
}

void __not_in_flash_func(def_world_draw_stars)(int32_t scroll, uint32_t frame) {
	for (uint32_t i = 0; i < DEF_STAR_COUNT; ++i) {
		const DefStar_t* star = &_stars[i];
		const uint32_t sx = (uint32_t)(star->x - scroll) & (DEF_STAR_WRAP - 1);
		if (sx >= GAME_SCREEN_W) continue;
		// Twinkle
		const uint32_t c = ((star->c + (frame >> 3)) & 15) == 0 ? GAME_WHITE : star->c & 1 ? GAME_GREY : GAME_NAVY;
		game_spr_add(sx, star->y, 8, 1, &_tile_star, game_pallet(c), sprite_renderer_sprite_8x8_p1);
	}
}

static int32_t fx_wrap(int32_t x) {
	return def_wrap(x);
}

static const GameFxView_t _fx_view = { def_world_screen_x, fx_wrap, DEF_HUD_H, GAME_SCREEN_H };

const GameFxView_t* def_world_fx_view(void) {
	return &_fx_view;
}
