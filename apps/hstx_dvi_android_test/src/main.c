// Copyright (c) 2024 fruit-bat.

// An android walking up and down, to try out sprites taller than 16 pixels.
//
// She is 32x48, with a sprite for each colour. Her hair is a separate set of
// sprites with its own animation, drawn over her body.
//
// This example requires an external digital video connector connected to
// GPIOs 12 through 19 (the HSTX-capable GPIOs) with appropriate
// current-limiting resistors, e.g. 270 ohms. The pinout used in this example
// matches the Pico DVI Sock board, which can be soldered onto a Pico 2:
// https://github.com/Wren6991/Pico-DVI-Sock

#include "hstx_dvi_core.h"
#include "hstx_dvi_sprite.h"
#include "app_led.h"
#include "pico/stdlib.h"
#include <stdio.h>
#include <string.h>
#include "game_pallet.h"
#include "game_render.h"
#include "game_util.h"
#include "android_art.h"

#define BODY_LAYERS 6
#define HAIR_LAYERS 2
// Video frames each walk and hair frame is shown for
#define WALK_RATE 6
#define HAIR_RATE 5
// Where her feet go
#define FLOOR_Y 400
// How close to the sides of the screen she turns round
#define TURN_MARGIN 40
// The frames laid out above her, 4 to a row
#define SHEET_X 224
#define SHEET_Y 96
#define SHEET_DX 64
#define SHEET_DY 72

// Not const so they live in RAM; the renderer reads them every sprite row
static hstx_dvi_pixel_t _body_colours[BODY_LAYERS] = {
	HSTX_DVI_PIXEL_RGB(0, 255, 255),   // Glow
	HSTX_DVI_PIXEL_RGB(255, 219, 180), // Skin
	HSTX_DVI_PIXEL_RGB(219, 219, 255), // Armour
	HSTX_DVI_PIXEL_RGB(73, 73, 170),   // Suit
	HSTX_DVI_PIXEL_RGB(36, 36, 85),    // The far arm and leg
	HSTX_DVI_PIXEL_RGB(0, 0, 0),       // Outline
};
static hstx_dvi_pixel_t _hair_colours[HAIR_LAYERS] = {
	HSTX_DVI_PIXEL_RGB(255, 182, 255), // Highlight
	HSTX_DVI_PIXEL_RGB(255, 0, 170),   // Hair
};
static hstx_dvi_pixel_t _sky = HSTX_DVI_PIXEL_RGB(36, 0, 85);
static hstx_dvi_pixel_t _floor = HSTX_DVI_PIXEL_RGB(255, 0, 170);
static hstx_dvi_pixel_t _floor_dim = HSTX_DVI_PIXEL_RGB(109, 0, 170);

// A row of bits for each layer, facing right then left
static uint32_t _body[2][ANDROID_WALK_FRAMES][BODY_LAYERS][ANDROID_BODY_H];
static uint32_t _hair[2][ANDROID_HAIR_FRAMES][HAIR_LAYERS][ANDROID_HAIR_H];

static GameText_t _title;
static GameText_t _subtitle;

// Turns rows of characters into a row of bits for each layer, as game_art_make
static void make_layers(uint32_t* d, const char* const* rows, uint32_t h, const char* key, uint32_t layers) {
	memset(d, 0, layers * h * sizeof(uint32_t));
	for (uint32_t r = 0; r < h; ++r) {
		for (uint32_t c = 0; c < ANDROID_W && rows[r][c]; ++c) {
			const char* k = strchr(key, rows[r][c]);
			if (k && (uint32_t)(k - key) < layers) d[(k - key) * h + r] |= 0x80000000u >> c;
		}
	}
}

static uint32_t reverse_bits(uint32_t v) {
	uint32_t m = 0;
	for (uint32_t i = 0; i < 32; ++i, v >>= 1) m = (m << 1) | (v & 1);
	return m;
}

static void mirror_layers(uint32_t* dst, const uint32_t* src, uint32_t n) {
	for (uint32_t i = 0; i < n; ++i) dst[i] = reverse_bits(src[i]);
}

static void make_art(void) {
	for (uint32_t f = 0; f < ANDROID_WALK_FRAMES; ++f) {
		make_layers(&_body[0][f][0][0], android_body_rows(f), ANDROID_BODY_H, android_body_key, BODY_LAYERS);
		mirror_layers(&_body[1][f][0][0], &_body[0][f][0][0], BODY_LAYERS * ANDROID_BODY_H);
	}
	for (uint32_t f = 0; f < ANDROID_HAIR_FRAMES; ++f) {
		make_layers(&_hair[0][f][0][0], android_hair_rows(f), ANDROID_HAIR_H, android_hair_key, HAIR_LAYERS);
		mirror_layers(&_hair[1][f][0][0], &_hair[0][f][0][0], HAIR_LAYERS * ANDROID_HAIR_H);
	}
}

// x is where the middle of her body goes, whichever way she faces
static void draw_android(int32_t x, int32_t y, uint32_t walk, uint32_t hair, bool left) {
	const uint32_t f = left ? 1 : 0;
	const int32_t sx = x - (left ? ANDROID_W - 1 - android_cx : android_cx);
	// The hair first, so it is drawn over the body
	for (uint32_t l = 0; l < HAIR_LAYERS; ++l) {
		game_spr_add(sx, y + android_bob[walk], ANDROID_W, ANDROID_HAIR_H, _hair[f][hair][l], &_hair_colours[l], sprite_renderer_sprite_32xn_p1);
	}
	for (uint32_t l = 0; l < BODY_LAYERS; ++l) {
		game_spr_add(sx, y, ANDROID_W, ANDROID_BODY_H, _body[f][walk][l], &_body_colours[l], sprite_renderer_sprite_32xn_p1);
	}
}

// A floor of lines that close up towards the horizon
static void draw_floor(void) {
	static const uint8_t gaps[] = { 0, 6, 14, 26, 42, 64 };
	for (uint32_t i = 0; i < sizeof(gaps); ++i) {
		game_spr_hline(0, FLOOR_Y + gaps[i], GAME_SCREEN_W, i ? &_floor_dim : &_floor);
	}
}

void main_loop(void) {
	// Fixed point, the middle of her body
	int32_t x = GAME_PX(GAME_SCREEN_W / 2);
	bool left = false;
	uint32_t walk = 0;
	uint32_t hair = 0;
	for (uint32_t frame = 0; ; ++frame) {
		if (frame % WALK_RATE == 0 && frame) walk = (walk + 1) % ANDROID_WALK_FRAMES;
		if (frame % HAIR_RATE == 0 && frame) hair = (hair + 1) % ANDROID_HAIR_FRAMES;
		x += (left ? -1 : 1) * android_step[walk] / WALK_RATE;
		if (!left && x > GAME_PX(GAME_SCREEN_W - TURN_MARGIN)) left = true;
		if (left && x < GAME_PX(TURN_MARGIN)) left = false;

		game_spr_begin();
		game_spr_fill(&_sky);
		draw_floor();
		game_spr_text_centre(&_title, 24, game_pallet(GAME_CYAN));
		game_spr_text_centre(&_subtitle, 48, game_pallet(GAME_PINK));
		// Every frame of the walk, with the hair still flowing
		for (uint32_t f = 0; f < ANDROID_WALK_FRAMES; ++f) {
			draw_android(SHEET_X + (f & 3) * SHEET_DX, SHEET_Y + (f >> 2) * SHEET_DY, f, hair, false);
		}
		draw_android(x >> GAME_FP, FLOOR_Y - ANDROID_BODY_H + 1, walk, hair, left);
		game_spr_end();
		hstx_dvi_sprite_wait_for_frame();
	}
}

int main(void)
{
	stdio_init_all();
	app_led_on();

	game_pallet_init();
	game_text_set(&_title, "ANDROID WALK CYCLE", 2);
	game_text_set(&_subtitle, "32X48 SPRITES", 1);
	make_art();

	// Initialize the HSTX DVI sprite system
	hstx_dvi_sprite_init_all();

	sleep_ms(2000); // Allow time for initialization

	printf("HSTX DVI Android Test\n");

	main_loop();
}
