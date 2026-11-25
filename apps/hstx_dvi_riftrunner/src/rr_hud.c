// Copyright (c) 2024 fruit-bat.

// See rr_hud.h.

#include "rr_hud.h"
#include "rr_world.h"
#include "rr_player.h"
#include "rr_gfx.h"
#include "rr_sound.h"
#include "game_render.h"
#include "game_pallet.h"
#include "game_hiscore.h"
#include <stdio.h>

#define RR_HUD_START_LIVES 3
#define RR_HUD_MAX_LIVES 9
#define RR_HUD_ICONS 5

#define RR_SLOT_X 8
#define RR_SLOT_Y 452
#define RR_SLOT_W 84
#define RR_SLOT_H 24
#define RR_SLOT_GAP 4
#define RR_BEAM_X (RR_SLOT_X + RR_POWERS * (RR_SLOT_W + RR_SLOT_GAP))
#define RR_BEAM_W (GAME_SCREEN_W - 8 - RR_BEAM_X)
#define RR_FUEL_X 56
#define RR_FUEL_W 200
#define RR_PROGRESS_X 476
#define RR_PROGRESS_W 24

static uint32_t _score = 0;
static uint32_t _lives = 0;
static uint32_t _next_bonus = 0;
static uint32_t _loop = 0;

// Text has to outlive the frame that shows it
static GameText_t _score_text, _hi_text, _fuel_text, _beam_text, _section_text;
static GameText_t _slot_text[RR_POWERS];
static bool _made = false;

static const char* const _slot_names[RR_POWERS] = { "SPEED", "BOMB", "TWIN", "LASER", "DRONE", "ORB" };

void rr_hud_reset(void) {
	_score = 0;
	_lives = RR_HUD_START_LIVES;
	_next_bonus = RR_HUD_FIRST_BONUS;
}

void rr_hud_add_score(uint32_t points) {
	_score += points;
	while (_score >= _next_bonus) {
		_next_bonus += RR_HUD_BONUS;
		if (_lives < RR_HUD_MAX_LIVES) ++_lives;
		game_sound_play(RR_SFX_EXTRA_LIFE);
	}
}

void rr_hud_set_loop(uint32_t loop) {
	_loop = loop;
}

uint32_t rr_hud_score(void) {
	return _score;
}

void rr_hud_set_score(uint32_t score) {
	_score = score;
}

uint32_t rr_hud_lives(void) {
	return _lives;
}

bool rr_hud_take_life(void) {
	if (!_lives) return false;
	--_lives;
	return true;
}

static void make(void) {
	for (uint32_t i = 0; i < RR_POWERS; ++i) game_text_set(&_slot_text[i], _slot_names[i], 1);
	game_text_set(&_fuel_text, "FUEL", 1);
	game_text_set(&_beam_text, "BEAM", 1);
	_made = true;
}

// Text draws straight into the row, over whatever was drawn before it, so
// it goes after the boxes it sits in
// The boxes of the power bar and the beam gauge, as one sprite drawing runs
// side by side, as drawing over what is already drawn costs twice as much.
// d1 holds the lit box (7 for none), whether it is bright, the speed and
// drone levels, how far the beam is charged and its colour.
#define RR_BAR_NONE 7
static const hstx_dvi_pixel_t _beam_px[4] = { RR_RGB(0, 96, 160), RR_RGB(0, 224, 255), RR_RGB(255, 255, 255), RR_RGB(0, 0, 64) };
static SpriteSpan_t _bar[RR_POWERS * 12 + 8];

static __force_inline uint32_t bar_add(uint32_t n, int32_t x0, int32_t x1, hstx_dvi_pixel_t p) {
	_bar[n].x0 = x0;
	_bar[n].x1 = x1;
	_bar[n].p = p;
	return n + 1;
}

static void __not_in_flash_func(bar_renderer)(
	const void* d1,
	const void* d2,
	hstx_dvi_row_t* r,
	const int32_t x,
	const int32_t row,
	const SpriteId spriteId
) {
	const uint32_t v = (uint32_t)(uintptr_t)d1;
	const uint32_t lit = v & 7;
	const bool bright = (v >> 3) & 1;
	const uint32_t speed = (v >> 4) & 7, drones = (v >> 7) & 3;
	const int32_t beam = (v >> 9) & 0xff;
	const hstx_dvi_pixel_t beam_p = _beam_px[(v >> 17) & 3];
	const bool edge = row < 2 || row >= RR_SLOT_H - 2;
	uint32_t n = 0;
	for (uint32_t i = 0; i < RR_POWERS; ++i) {
		const int32_t sx = RR_SLOT_X + i * (RR_SLOT_W + RR_SLOT_GAP);
		const hstx_dvi_pixel_t border = i == lit ? RR_RGB(255, 255, 255) : RR_RGB(64, 96, 255);
		if (edge) {
			n = bar_add(n, sx, sx + RR_SLOT_W, border);
			continue;
		}
		n = bar_add(n, sx, sx + 2, border);
		int32_t fx = sx + 2;
		// Pips for the levels of those that go up more than once
		const uint32_t pips = row >= 3 && row < 6 ? (i == RR_POW_SPEED ? speed : i == RR_POW_DRONE ? drones : 0) : 0;
		const hstx_dvi_pixel_t fill = i == lit ? (bright ? RR_RGB(255, 160, 0) : RR_RGB(255, 224, 0)) : RR_RGB(0, 0, 112);
		for (uint32_t k = 0; k < pips; ++k) {
			const int32_t px = sx + 4 + k * 6;
			n = bar_add(n, fx, px, fill);
			n = bar_add(n, px, px + 4, RR_RGB(0, 255, 128));
			fx = px + 4;
		}
		n = bar_add(n, fx, sx + RR_SLOT_W - 2, fill);
		n = bar_add(n, sx + RR_SLOT_W - 2, sx + RR_SLOT_W, border);
	}
	const hstx_dvi_pixel_t border = RR_RGB(0, 160, 255);
	if (edge) {
		n = bar_add(n, RR_BEAM_X, RR_BEAM_X + RR_BEAM_W, border);
	}
	else {
		n = bar_add(n, RR_BEAM_X, RR_BEAM_X + 2, border);
		n = bar_add(n, RR_BEAM_X + 2, RR_BEAM_X + 2 + beam, beam_p);
		n = bar_add(n, RR_BEAM_X + 2 + beam, RR_BEAM_X + RR_BEAM_W - 2, _beam_px[3]);
		n = bar_add(n, RR_BEAM_X + RR_BEAM_W - 2, RR_BEAM_X + RR_BEAM_W, border);
	}
	// Runs of nothing are left out by the renderer
	sprite_render_spans(r, _bar, n, spriteId);
}

// Text draws straight into the row, over whatever was drawn before it, so
// it goes after the boxes it sits in
static void draw_power_bar(uint32_t frame) {
	const int32_t cursor = rr_player_power_cursor();
	const uint32_t beam = rr_player_beam();
	const bool full = beam >= RR_BEAM_FULL;
	const uint32_t w = (RR_BEAM_W - 4) * beam / RR_BEAM_FULL;
	const uint32_t beam_c = full && (frame & 4) ? 2 : beam >= RR_BEAM_READY ? 1 : 0;
	const uint32_t v = (cursor < 0 ? RR_BAR_NONE : (uint32_t)cursor) | (((frame >> 3) & 1) << 3)
		| (rr_player_power_level(RR_POW_SPEED) << 4) | (rr_player_power_level(RR_POW_DRONE) << 7)
		| (w << 9) | (beam_c << 17);
	game_spr_add(0, RR_SLOT_Y, GAME_SCREEN_W, RR_SLOT_H, (void*)(uintptr_t)v, NULL, bar_renderer);

	for (uint32_t i = 0; i < RR_POWERS; ++i) {
		const int32_t x = RR_SLOT_X + i * (RR_SLOT_W + RR_SLOT_GAP);
		const bool lit = (int32_t)i == cursor;
		const bool full_power = rr_player_power_full(i);
		const int32_t tx = x + (RR_SLOT_W - game_text_width(&_slot_text[i])) / 2;
		game_spr_text(&_slot_text[i], tx, RR_SLOT_Y + 8,
			lit ? game_pallet(GAME_BLACK) : full_power ? game_pallet(GAME_GREY) : game_pallet(GAME_WHITE));
	}
	game_spr_text(&_beam_text, RR_BEAM_X + (RR_BEAM_W - 32) / 2, RR_SLOT_Y + 8,
		beam > RR_BEAM_FULL / 2 ? game_pallet(GAME_BLACK) : game_pallet(GAME_CYAN));
}

void rr_hud_draw(uint32_t frame, bool in_game) {
	if (!_made) make();
	char t[GAME_TEXT_MAX];
	snprintf(t, sizeof(t), "%07lu", (unsigned long)(_score % 10000000));
	game_text_set(&_score_text, t, GAME_TEXT_BIG);
	game_spr_text(&_score_text, 12, 4, game_pallet(GAME_WHITE));

	const uint32_t top = game_hiscore_top();
	snprintf(t, sizeof(t), "HI %07lu", (unsigned long)((_score > top ? _score : top) % 10000000));
	game_text_set(&_hi_text, t, GAME_TEXT_BIG);
	game_spr_text(&_hi_text, 200, 4, game_pallet(GAME_YELLOW));

	if (in_game) {
		for (uint32_t i = 0; i < _lives && i < RR_HUD_ICONS; ++i) {
			rr_art_draw(&rr_gfx.ship_small, GAME_SCREEN_W - 40 - i * 36, 4);
		}

		// Fuel, red and flashing when low
		const int32_t fuel = rr_player_fuel();
		const bool low = fuel < RR_FUEL_LOW;
		const int32_t w = RR_FUEL_W * fuel / RR_FUEL_MAX;
		rr_spr_rect(RR_FUEL_X, 25, w, 10, low ? RR_RGB(255, 32, 0) : fuel < RR_FUEL_MAX / 2 ? RR_RGB(255, 224, 0) : RR_RGB(0, 224, 64));
		rr_spr_rect(RR_FUEL_X + w, 25, RR_FUEL_W - w, 10, RR_RGB(48, 48, 64));
		game_spr_text(&_fuel_text, 12, 26, low && (frame & 16) ? game_pallet(GAME_RED) : game_pallet(GAME_WHITE));

		// Where we are, a box for each section
		const uint32_t section = rr_world_section((rr_world_camera() >> RR_FP) + GAME_SCREEN_W / 2);
		snprintf(t, sizeof(t), "%lu-%lu %s", (unsigned long)(_loop + 1), (unsigned long)(section + 1),
			rr_world_section_name(section));
		game_text_set(&_section_text, t, 1);
		game_spr_text(&_section_text, 270, 26, game_pallet(GAME_CYAN));
		for (uint32_t i = 0; i < RR_SECTIONS; ++i) {
			const hstx_dvi_pixel_t p = i < section ? RR_RGB(0, 160, 255) : i == section ? ((frame & 16) ? RR_RGB(255, 255, 255) : RR_RGB(0, 255, 255)) : RR_RGB(48, 48, 96);
			rr_spr_rect(RR_PROGRESS_X + i * (RR_PROGRESS_W + 2), 26, RR_PROGRESS_W, 8, p);
		}

		draw_power_bar(frame);
		game_spr_hline(0, RR_HUD_BOTTOM - 2, GAME_SCREEN_W, game_pallet(GAME_BLUE));
	}
	game_spr_hline(0, RR_PLAY_TOP - 2, GAME_SCREEN_W, game_pallet(GAME_BLUE));
}
