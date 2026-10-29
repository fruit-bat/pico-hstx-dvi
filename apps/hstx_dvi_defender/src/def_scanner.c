#include "def_scanner.h"
#include "def_world.h"
#include "game_pallet.h"
#include <string.h>

#define DEF_SCANNER_W 256
#define DEF_SCANNER_H 48
#define DEF_SCANNER_X ((GAME_SCREEN_W - DEF_SCANNER_W) >> 1)
#define DEF_SCANNER_Y 4
// World pixels per scanner pixel, as a shift
#define DEF_SCANNER_SHIFT 4
#define DEF_SCANNER_WORDS (DEF_SCANNER_W >> 2)

// Drawn into one while the other is being shown
static uint32_t _buf[2][DEF_SCANNER_H * DEF_SCANNER_WORDS];
static GamePixels_t _pixels[2] = {
	{ _buf[0], DEF_SCANNER_WORDS },
	{ _buf[1], DEF_SCANNER_WORDS }
};
static uint32_t _buf_i = 0;
static int32_t _centre = 0;

static __force_inline uint8_t* scanner_pixels(void) {
	return (uint8_t*)_buf[_buf_i];
}

static __force_inline int32_t scanner_x(const int32_t x) {
	return (def_dx(x, _centre) >> (DEF_FP + DEF_SCANNER_SHIFT)) + (DEF_SCANNER_W >> 1);
}

static void plot(const int32_t sx, const int32_t sy, const hstx_dvi_pixel_t p) {
	if (sx < 0 || sx >= DEF_SCANNER_W || sy < 0 || sy >= DEF_SCANNER_H) return;
	scanner_pixels()[sy * DEF_SCANNER_W + sx] = p;
}

void def_scanner_begin(int32_t centre) {
	_buf_i ^= 1;
	_centre = centre;
	memset(_buf[_buf_i], 0, sizeof(_buf[0]));

	// A frame round the outside
	const hstx_dvi_pixel_t frame = *game_pallet(GAME_BLUE);
	for (int32_t x = 0; x < DEF_SCANNER_W; ++x) {
		plot(x, 0, frame);
		plot(x, DEF_SCANNER_H - 1, frame);
	}
	for (int32_t y = 0; y < DEF_SCANNER_H; ++y) {
		plot(0, y, frame);
		plot(DEF_SCANNER_W - 1, y, frame);
	}

	// Brackets showing the part of the world on the screen
	const hstx_dvi_pixel_t white = *game_pallet(GAME_WHITE);
	const int32_t x0 = scanner_x(def_world_camera());
	const int32_t x1 = x0 + (GAME_SCREEN_W >> DEF_SCANNER_SHIFT);
	for (int32_t i = 0; i < 4; ++i) {
		plot(x0 + i, 1, white);
		plot(x1 - i, 1, white);
		plot(x0 + i, DEF_SCANNER_H - 2, white);
		plot(x1 - i, DEF_SCANNER_H - 2, white);
	}

	// The mountains
	if (def_world_planet_alive()) {
		const hstx_dvi_pixel_t brown = *game_pallet(GAME_BROWN);
		for (int32_t sx = 1; sx < DEF_SCANNER_W - 1; ++sx) {
			const int32_t x = def_wrap(_centre + DEF_PX((sx - (DEF_SCANNER_W >> 1)) << DEF_SCANNER_SHIFT));
			const int32_t y = def_world_ground_y(x);
			plot(sx, 1 + (y - DEF_PLAY_TOP) * (DEF_SCANNER_H - 3) / (GAME_SCREEN_H - DEF_PLAY_TOP), brown);
		}
	}
}

void def_scanner_dot(int32_t x, int32_t y, uint32_t colour) {
	const hstx_dvi_pixel_t p = *game_pallet(colour);
	const int32_t sx = scanner_x(x);
	const int32_t sy = 1 + (y - DEF_PLAY_TOP) * (DEF_SCANNER_H - 3) / (GAME_SCREEN_H - DEF_PLAY_TOP);
	plot(sx, sy, p);
	plot(sx + 1, sy, p);
	plot(sx, sy + 1, p);
	plot(sx + 1, sy + 1, p);
}

void def_scanner_draw(void) {
	game_spr_pixels(DEF_SCANNER_X, DEF_SCANNER_Y, DEF_SCANNER_W, DEF_SCANNER_H, &_pixels[_buf_i]);
}
