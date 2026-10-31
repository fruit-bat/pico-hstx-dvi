#include "inv_game.h"
#include "hstx_dvi_sprite.h"
#include "inv_input.h"
#include "inv_invaders.h"
#include "inv_bullets.h"
#include "inv_bombs.h"
#include "inv_score.h"
#include "game_pallet.h"
#include "inv_mot.h"
#include "inv_gun.h"
#include "inv_base.h"
#include "inv_level.h"
#include "inv_screen.h"
#include "inv_demo.h"
#include "game_util.h"
#include "game_render.h"
#include "game_fx.h"
#include <stdio.h>

#define INV_GAME_TEXT_SLOTS 2
#define INV_GAME_SLOT_BIG 0
#define INV_GAME_SLOT_SMALL 1
#define INV_GAME_MESSAGE_Y 260
#define INV_GAME_READY_FRAMES 120
#define INV_GAME_WAVE_FRAMES 120
#define INV_GAME_OVER_FRAMES 180
#define INV_GAME_DEMO_FRAMES (60 * 30)

static SpriteId _invaders_si = 0;
static SpriteId _mot_si = 0;
static SpriteId _base_si = 0;
// Sprites from here up are handed out each frame for the explosions and scores
static SpriteId _fx_si = 0;

static void game_init(void) {
	inv_screen_clear();

	// Lower ids are drawn first, so the invaders take priority
	SpriteId si = 0;
	_invaders_si = si;
	si = inv_invaders_init(si);
	si = inv_bullets_init(si);
	si = inv_score_init(si, true);
	_mot_si = si;
	si = inv_mot_init(si);
	si = inv_gun_init(si);
	_base_si = si;
	si = inv_base_init(si);
	si = inv_bombs_init(si);
	si = inv_screen_init(si, INV_GAME_TEXT_SLOTS);
	_fx_si = si;
	game_fx_init();
}

static void draw_fx(void) {
	game_fx_update();
	game_spr_begin_at(_fx_si);
	game_fx_draw_popups();
	game_fx_draw_particles();
	game_spr_end();
}

static void show_message(const char* big, hstx_dvi_pixel_t* p, const char* small) {
	inv_screen_text_centre(INV_GAME_SLOT_BIG, INV_GAME_MESSAGE_Y, big, p, true);
	if (small) {
		inv_screen_text_centre(INV_GAME_SLOT_SMALL, INV_GAME_MESSAGE_Y + 28, small, game_pallet(GAME_WHITE), false);
	}
}

static void hide_message(void) {
	inv_screen_text_show(INV_GAME_SLOT_BIG, false);
	inv_screen_text_show(INV_GAME_SLOT_SMALL, false);
}

// Waits a number of frames with the game held still. Returns true if the
// demo is taken over.
static bool hold_frames(const uint32_t frames, const bool demo) {
	for (uint32_t i = 0; i < frames; ++i) {
		hstx_dvi_sprite_wait_for_frame();
		inv_score_update();
		draw_fx();
		if (demo && is_inv_input_play(game_input_pressed())) return true;
	}
	return false;
}

static bool next_wave(const bool demo) {
	char t[16];
	inv_bullets_clear();
	inv_bombs_clear();
	inv_level_next();
	snprintf(t, sizeof(t), "WAVE %lu", (unsigned long)inv_level_get());
	show_message(t, game_pallet_rainbow(inv_level_get()), "GET READY");
	if (hold_frames(INV_GAME_WAVE_FRAMES, demo)) return true;
	hide_message();
	inv_invaders_init(_invaders_si);
	inv_mot_init(_mot_si);
	inv_base_init(_base_si);
	return false;
}

// Runs the game until the gun has run out of lives. Returns true if the demo
// is taken over.
static bool game_run(const bool demo) {
	for(uint32_t frame = 0; inv_gun_get_lives(); ++frame) {
		hstx_dvi_sprite_wait_for_frame();

		uint8_t input;
		if (demo) {
			if (is_inv_input_play(game_input_pressed())) return true;
			if (frame >= INV_GAME_DEMO_FRAMES) return false;
			inv_screen_text_show(INV_GAME_SLOT_SMALL, (frame >> 5) & 1);
			input = inv_demo_input(frame);
		}
		else {
			input = game_input_get();
		}

		inv_invader_update(frame);
		inv_bullets_update();
		inv_bombs_update();
		inv_mot_update(frame);
		inv_base_update();
		inv_gun_update(frame, input);
		inv_score_update();
		draw_fx();

		if (inv_invaders_landed()) {
			inv_gun_destroy(frame);
		}
		else if (inv_invaders_cleared() && inv_gun_is_alive()) {
			if (next_wave(demo)) return true;
			if (demo) inv_screen_text_centre(INV_GAME_SLOT_SMALL, MODE_V_ACTIVE_LINES - 24, "DEMO - PRESS FIRE TO PLAY", game_pallet(GAME_YELLOW), false);
		}
	}
	return false;
}

void inv_game_play(void) {
	game_rand_seed(time_us_32());
	inv_level_reset();
	inv_score_reset();
	game_init();

	show_message("PLAYER 1", game_pallet(GAME_WHITE), "GET READY");
	hold_frames(INV_GAME_READY_FRAMES, false);
	hide_message();

	game_run(false);

	show_message("GAME OVER", game_pallet(GAME_RED), inv_invaders_landed() ? "THE INVADERS HAVE LANDED" : NULL);
	hold_frames(INV_GAME_OVER_FRAMES, false);
}

bool inv_game_demo(void) {
	// The demo should not count against the last player's score
	const uint32_t score = inv_score_get();

	inv_level_reset();
	inv_score_reset();
	inv_demo_init();
	game_init();
	game_input_pressed();

	inv_screen_text_centre(INV_GAME_SLOT_SMALL, MODE_V_ACTIVE_LINES - 24, "DEMO - PRESS FIRE TO PLAY", game_pallet(GAME_YELLOW), false);
	const bool taken_over = game_run(true);

	inv_score_set(score);
	return taken_over;
}
