// Copyright (c) 2024 fruit-bat.

// See rr_demo.h.

#include "rr_demo.h"
#include "rr_input.h"
#include "rr_player.h"
#include "rr_enemy.h"
#include "rr_boss.h"
#include "rr_world.h"
#include "game_util.h"

// How close to the land it lets itself go
#define RR_DEMO_MARGIN 34
// Lines up with a target to within this
#define RR_DEMO_LINE_UP 6
#define RR_DEMO_DODGE 56
#define RR_DEMO_DODGE_FRAMES 14

static uint16_t _dodge = 0;
static uint32_t _dodge_timer = 0;

uint16_t rr_demo_input(uint32_t frame) {
	if (!rr_player_alive()) return 0;
	const int32_t px = rr_player_px(), py = rr_player_py();
	const int32_t cam = rr_world_camera() >> RR_FP;
	const int32_t sx = px - cam;
	const int32_t cy = py + RR_SHIP_NOSE_Y;
	uint16_t input = 0;
	int32_t want_y = cy, want_x = 140;
	bool shoot = false;
	int32_t tx, ty;

	if (rr_boss_target(&tx, &ty)) {
		want_y = ty;
		want_x = 120;
		shoot = true;
		// Charges the beam now and then
		if ((frame % 200) < 110 && (frame % 200) > 10) input |= RR_INP_BIT_F;
	}
	else if (rr_enemy_capsule(&tx, &ty) && tx > px - 20) {
		want_y = ty;
		want_x = game_clamp(tx - cam - 30, 60, 400);
	}
	else if (rr_enemy_target(px + RR_SHIP_W, cy, &tx, &ty)) {
		want_y = ty;
		shoot = true;
	}
	else if (rr_enemy_ground_ahead(px + 20, 240, &tx, &ty)) {
		// Over it, to bomb it, and shoot what is in the way
		want_y = ty - 80;
		shoot = true;
	}
	else {
		const int32_t top = rr_world_ceil_max(px, px + 240);
		const int32_t bottom = rr_world_floor_min(px, px + 240);
		want_y = ((top < RR_PLAY_TOP ? RR_PLAY_TOP : top) + (bottom > RR_PLAY_BOTTOM ? RR_PLAY_BOTTOM : bottom)) / 2;
	}

	// Things on the ground just ahead get a bomb
	if (rr_enemy_ground_ahead(px + 30, 110, &tx, &ty) && (frame & 15) == 0) input |= RR_INP_BIT_B;

	// Keeping clear of the land just ahead
	const int32_t top = rr_world_ceil_max(px, px + RR_SHIP_W + 90);
	const int32_t bottom = rr_world_floor_min(px, px + RR_SHIP_W + 90);
	const int32_t lo = top + RR_DEMO_MARGIN, hi = bottom - RR_DEMO_MARGIN;
	if (lo < hi) want_y = game_clamp(want_y, lo, hi);
	else want_y = (top + bottom) / 2;

	int32_t bx, by;
	if (_dodge_timer) {
		--_dodge_timer;
		input |= _dodge;
	}
	else if (rr_enemy_danger_near(px + RR_SHIP_W / 2, cy, RR_DEMO_DODGE, &bx, &by)) {
		_dodge = (by < cy && cy < hi - 40) || cy < lo + 40 ? RR_INP_BIT_D : RR_INP_BIT_U;
		// Backing off as well, from what is in front
		if (bx > px + RR_SHIP_W / 2 && sx > 40) _dodge |= RR_INP_BIT_L;
		_dodge_timer = RR_DEMO_DODGE_FRAMES;
		input |= _dodge;
	}
	else if (cy < want_y - RR_DEMO_LINE_UP) {
		input |= RR_INP_BIT_D;
	}
	else if (cy > want_y + RR_DEMO_LINE_UP) {
		input |= RR_INP_BIT_U;
	}
	if (!(input & RR_INP_BIT_L)) {
		if (sx < want_x - 8) input |= RR_INP_BIT_R;
		else if (sx > want_x + 8) input |= RR_INP_BIT_L;
	}

	// Taps fire while lined up
	if (shoot && !(input & RR_INP_BIT_F) && game_abs(want_y - cy) < 24 && (frame & 7) < 2) input |= RR_INP_BIT_F;

	// Takes the power ups it likes
	const int32_t cursor = rr_player_power_cursor();
	if (cursor >= 0 && cursor != RR_POW_TWIN && !rr_player_power_full(cursor) && (frame & 7) == 0) {
		if (cursor != RR_POW_SPEED || rr_player_power_level(RR_POW_SPEED) < 2) input |= RR_INP_BIT_POWER;
	}
	return input;
}
