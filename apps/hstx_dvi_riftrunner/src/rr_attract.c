// Copyright (c) 2024 fruit-bat.

// See rr_attract.h.

#include "rr_attract.h"
#include "rr_sound.h"
#include "rr_input.h"
#include "rr_world.h"
#include "rr_enemy.h"
#include "rr_player.h"
#include "rr_hud.h"
#include "rr_fx.h"
#include "rr_gfx.h"
#include "game_render.h"
#include "game_pallet.h"
#include "game_hiscore.h"
#include "game_initials.h"
#include "game_options.h"
#include "game_fx.h"
#include "game_util.h"
#include <stdio.h>
#include <string.h>

#define RR_TITLE "RIFT RUNNER"
#define RR_TITLE_LEN 11
#define RR_TITLE_SCALE 5
#define RR_TITLE_FRAMES 600
#define RR_ROLL_FRAMES 540
#define RR_HOWTO_FRAMES 600
#define RR_HISCORE_FRAMES 540
#define RR_HIGHLIGHT_FRAMES 840
// Give up waiting for initials after 30 seconds
#define RR_ENTRY_FRAMES (60 * 30)
#define RR_ROLL_START 40
#define RR_ROLL_GAP 30

static const uint8_t _roll[] = {
	RR_EN_DART, RR_EN_CARRIER, RR_EN_SAUCER, RR_EN_METEOR,
	RR_EN_ROCKET, RR_EN_FUEL, RR_EN_TURRET, RR_EN_CAPSULE,
};
#define RR_ROLL_COUNT (sizeof(_roll) / sizeof(_roll[0]))

// Text has to outlive the screen that shows it, as the renderer keeps
// drawing the last frame until the next screen has been set up
static GameText_t _text[16];
static GameText_t _names[RR_ROLL_COUNT];
static GameText_t _scores[RR_ROLL_COUNT];

static void attract_init(void) {
	rr_gfx_init();
	game_fx_init();
	rr_fx_init();
	// Forget about any buttons still held down from before
	game_input_pressed();
}

static void settings_back(uint32_t frame) {
	rr_world_draw_stars(frame * 2);
}

// Start shows the settings, and the screen goes on after
static void settings(void) {
	static const GameOptions_t o = {
		RR_INP_BIT_U, RR_INP_BIT_D, RR_INP_BIT_L, RR_INP_BIT_R, RR_INP_BIT_F, RR_INP_BIT_B, RR_INP_BIT_START,
		rr_wait_for_frame, settings_back
	};
	game_options_run(&o);
}

// Waits for the next frame. Returns true if the player wants to play.
static bool attract_frame(void) {
	rr_wait_for_frame();
	game_fx_update();
	rr_fx_update();
	const uint16_t pressed = game_input_pressed();
	if (pressed & RR_INP_BIT_START) {
		settings();
		return false;
	}
	return is_rr_input_play(pressed);
}

bool rr_attract_title(void) {
	attract_init();
	game_music_play(RR_TUNE_TITLE);
	// Land going by under the title
	game_rand_seed(0x7171);
	rr_world_init();
	rr_enemy_init();

	for (uint32_t i = 0; i < RR_TITLE_LEN; ++i) {
		const char s[2] = { RR_TITLE[i], 0 };
		game_text_set(&_text[i], s, RR_TITLE_SCALE);
	}
	game_text_set(&_text[11], "FLY THE RIFT  SINK THE WARSHIP", GAME_TEXT_BIG);
	game_text_set(&_text[12], "PRESS FIRE TO START", GAME_TEXT_BIG);
	game_text_set(&_text[13], "START FOR SETTINGS", GAME_TEXT_BIG);
	game_text_set(&_text[14], "RP2350 HSTX DVI SPRITES", 1);

	const int32_t tx = (GAME_SCREEN_W - RR_TITLE_LEN * 8 * RR_TITLE_SCALE) >> 1;
	for (uint32_t frame = 0; frame < RR_TITLE_FRAMES; ++frame) {
		if (attract_frame()) return true;
		rr_world_scroll(RR_SCROLL * 2);
		// The ship sweeps in, then bobs about
		const int32_t sx = frame < 120 ? -80 + (int32_t)frame * 2 : 160 + game_sin(frame) * 40 / 127;
		const int32_t sy = 250 + game_sin(frame * 2 + 64) * 20 / 127;

		game_spr_begin();
		rr_hud_draw(frame, false);
		for (uint32_t i = 0; i < RR_TITLE_LEN; ++i) {
			// A ripple of colour across the letters, which bob up and down
			const int32_t bob = game_sin((frame << 2) + i * 20) * 6 / 127;
			game_spr_text(&_text[i], tx + i * 8 * RR_TITLE_SCALE, 70 + bob, game_pallet_rainbow(i + (frame >> 3)));
		}
		game_spr_text_centre(&_text[11], 130, game_pallet(GAME_WHITE));
		if ((frame >> 5) & 1) game_spr_text_centre(&_text[12], 170, game_pallet(GAME_YELLOW));
		game_spr_text_centre(&_text[13], 196, game_pallet(GAME_CYAN));
		game_spr_text_centre(&_text[14], RR_HUD_BOTTOM + 12, game_pallet(GAME_GREY));
		rr_art_draw(&rr_gfx.ship, sx, sy);
		rr_art_draw(&rr_gfx.flame[(frame >> 1) & 1], sx - 16, sy + 12);
		rr_art_draw(&rr_gfx.drone[(frame >> 3) & 1], sx - 40, sy + 6 + game_sin(frame * 2 + 32) * 16 / 127);
		rr_world_draw_terrain();
		rr_world_draw_back();
		game_spr_end();
	}
	return false;
}

bool rr_attract_roll(void) {
	attract_init();
	char t[GAME_TEXT_MAX];
	game_text_set(&_text[0], "THE ENEMY", 4);
	game_text_set(&_text[1], "PRESS FIRE TO START", GAME_TEXT_BIG);
	for (uint32_t i = 0; i < RR_ROLL_COUNT; ++i) {
		game_text_set(&_names[i], _roll[i] == RR_EN_CAPSULE ? "CAPSULE" : rr_enemy_name(_roll[i]), GAME_TEXT_BIG);
		if (_roll[i] == RR_EN_CAPSULE) snprintf(t, sizeof(t), "POWER UP");
		else if (_roll[i] == RR_EN_FUEL) snprintf(t, sizeof(t), "%lu + FUEL", (unsigned long)rr_enemy_score(_roll[i]));
		else snprintf(t, sizeof(t), "%lu", (unsigned long)rr_enemy_score(_roll[i]));
		game_text_set(&_scores[i], t, GAME_TEXT_BIG);
	}

	for (uint32_t frame = 0; frame < RR_ROLL_FRAMES; ++frame) {
		if (attract_frame()) return true;
		game_spr_begin();
		rr_hud_draw(frame, false);
		game_spr_text_centre(&_text[0], 60, game_pallet_rainbow(frame >> 3));
		for (uint32_t i = 0; i < RR_ROLL_COUNT; ++i) {
			const uint32_t at = RR_ROLL_START + i * RR_ROLL_GAP;
			if (frame < at) continue;
			// Two columns, each thing sliding in from the right
			const int32_t col = i & 1, row = i >> 1;
			const int32_t x = 40 + col * 300;
			const int32_t y = 120 + row * 76;
			const int32_t slide = frame - at < 20 ? (20 - (int32_t)(frame - at)) * 20 : 0;
			const int32_t h = rr_enemy_height(_roll[i]);
			rr_enemy_draw_icon(_roll[i], x + slide + (72 - rr_enemy_width(_roll[i])) / 2, y + (40 - h) / 2, frame);
			game_spr_text(&_names[i], x + 88 + slide, y + 4, game_pallet(GAME_WHITE));
			game_spr_text(&_scores[i], x + 88 + slide, y + 24, game_pallet(GAME_YELLOW));
		}
		if ((frame >> 5) & 1) game_spr_text_centre(&_text[1], 420, game_pallet(GAME_YELLOW));
		rr_world_draw_stars(frame * 3);
		game_spr_end();
	}
	return false;
}

bool rr_attract_howto(void) {
	attract_init();
	static const char* const lines[] = {
		"FIRE        SHOOT",
		"HOLD FIRE   CHARGE THE BEAM",
		"B           DROP A BOMB",
		"SELECT,L,R  TAKE A POWER UP",
		"",
		"CAPSULES LIGHT UP THE POWER BAR",
		"SHOOT FUEL TANKS TO REFUEL",
		"WALLS FIRST, THEN THE CORE",
	};
	const uint32_t n = sizeof(lines) / sizeof(lines[0]);
	game_text_set(&_text[0], "HOW TO FLY", 4);
	for (uint32_t i = 0; i < n; ++i) game_text_set(&_text[1 + i], lines[i], GAME_TEXT_BIG);
	game_text_set(&_text[12], "PRESS FIRE TO START", GAME_TEXT_BIG);

	for (uint32_t frame = 0; frame < RR_HOWTO_FRAMES; ++frame) {
		if (attract_frame()) return true;
		game_spr_begin();
		rr_hud_draw(frame, false);
		game_spr_text_centre(&_text[0], 60, game_pallet_rainbow(frame >> 3));
		for (uint32_t i = 0; i < n; ++i) {
			if (frame < 20 + i * 12) continue;
			game_spr_text(&_text[1 + i], 64, 130 + i * 30, game_pallet(i < 4 ? GAME_WHITE : GAME_CYAN));
		}
		if ((frame >> 5) & 1) game_spr_text_centre(&_text[12], 420, game_pallet(GAME_YELLOW));
		// The ship, charging its beam and letting go, as in the game
		const uint32_t f = frame % 160;
		const int32_t sx = 48, sy = 376;
		const int32_t nx = sx + RR_SHIP_NOSE_X, ny = sy + RR_SHIP_NOSE_Y;
		if (f < 100) {
			if (f > 10) {
				const uint32_t charge = (f - 10) * RR_BEAM_FULL / 90;
				const bool full = charge >= RR_BEAM_FULL;
				rr_fx_disc(nx + 4, ny, 3 + charge * 12 / RR_BEAM_FULL + (full && (frame & 2) ? 3 : 0), 0,
					(frame & 4) ? RR_RGB(255, 255, 255) : full ? RR_RGB(255, 96, 255) : RR_RGB(0, 224, 255));
			}
		}
		else {
			// The head going on, the tail no further back than the nose
			const int32_t head = nx + ((int32_t)f - 100) * RR_BEAM_SPEED;
			const int32_t tail = head - RR_BEAM_LEN > nx ? head - RR_BEAM_LEN : nx;
			rr_player_draw_beam(tail, ny, head - tail, 40, frame);
		}
		rr_art_draw(&rr_gfx.ship, sx, sy);
		rr_art_draw(&rr_gfx.flame[(frame >> 1) & 1], sx - 16, sy + 12);
		rr_world_draw_stars(frame * 3);
		game_spr_end();
	}
	return false;
}

static uint32_t rank_colour(const uint32_t rank) {
	if (rank == 0) return GAME_YELLOW;
	if (rank < 3) return GAME_WHITE;
	if (rank < 6) return GAME_CYAN;
	return GAME_GREEN;
}

bool rr_attract_hiscores(int32_t highlight) {
	attract_init();
	// After a new high score its tune goes on
	if (highlight < 0) game_music_play(RR_TUNE_TITLE);
	char rows[GAME_HISCORE_COUNT][GAME_TEXT_MAX];
	game_text_set(&_text[0], "HALL OF FAME", 4);
	for (uint32_t i = 0; i < GAME_HISCORE_COUNT; ++i) {
		const GameHiScore_t *e = game_hiscore_get(i);
		snprintf(rows[i], GAME_TEXT_MAX, "%-4s  %07lu  %s", game_hiscore_rank_name(i), (unsigned long)e->score, e->name);
	}
	game_text_set(&_text[11], "PRESS FIRE TO START", GAME_TEXT_BIG);

	const uint32_t frames = highlight >= 0 ? RR_HIGHLIGHT_FRAMES : RR_HISCORE_FRAMES;
	for (uint32_t frame = 0; frame < frames; ++frame) {
		if (attract_frame()) return true;

		game_spr_begin();
		rr_hud_draw(frame, false);
		game_spr_text_centre(&_text[0], 60, game_pallet_rainbow(frame >> 3));
		// Typed out a line at a time
		int32_t n = (int32_t)frame - 30;
		for (uint32_t i = 0; i < GAME_HISCORE_COUNT; ++i) {
			game_text_set_n(&_text[1 + i], rows[i], n < 0 ? 0 : n, GAME_TEXT_BIG);
			n -= strlen(rows[i]);
			const uint32_t c = (int32_t)i == highlight && ((frame >> 3) & 1) ? GAME_RED : rank_colour(i);
			game_spr_text_centre(&_text[1 + i], 130 + i * 26, game_pallet(c));
		}
		if (frame > 260 && ((frame >> 5) & 1)) game_spr_text_centre(&_text[11], 420, game_pallet(GAME_YELLOW));
		rr_world_draw_stars(frame * 2);
		game_spr_end();
	}
	return false;
}

bool rr_attract_game_over(void) {
	const uint32_t score = rr_hud_score();
	const int32_t rank = game_hiscore_rank(score);
	if (rank < 0) return false;

	attract_init();
	game_music_play(RR_TUNE_HISCORE);
	char t[GAME_TEXT_MAX];
	game_text_set(&_text[0], "NEW HIGH SCORE", 4);
	snprintf(t, sizeof(t), "SCORE %07lu   %s PLACE", (unsigned long)score, game_hiscore_rank_name(rank));
	game_text_set(&_text[1], t, GAME_TEXT_BIG);
	game_text_set(&_text[2], "ENTER YOUR INITIALS", GAME_TEXT_BIG);
	game_text_set(&_text[6], "LEFT/RIGHT OR UP/DOWN TO CHOOSE", GAME_TEXT_BIG);
	game_text_set(&_text[7], "FIRE TO ENTER  BOMB WHEN DONE", GAME_TEXT_BIG);

	GameInitials_t g;
	game_initials_reset(&g);
	for (uint32_t frame = 0; frame < RR_ENTRY_FRAMES; ++frame) {
		rr_wait_for_frame();

		const uint16_t pressed = game_input_pressed();
		const uint16_t input = game_input_get();
		const int32_t dir = (input & (RR_INP_BIT_L | RR_INP_BIT_D)) ? -1 : (input & (RR_INP_BIT_R | RR_INP_BIT_U)) ? 1 : 0;
		const bool stepped = (pressed & (RR_INP_BIT_L | RR_INP_BIT_R | RR_INP_BIT_U | RR_INP_BIT_D)) != 0;
		if (game_initials_update(&g, dir, stepped, pressed & RR_INP_BIT_F) || (pressed & RR_INP_BIT_B)) break;

		game_spr_begin();
		rr_hud_draw(frame, false);
		game_spr_text_centre(&_text[0], 70, game_pallet_rainbow(frame >> 3));
		game_spr_text_centre(&_text[1], 130, game_pallet(GAME_WHITE));
		game_spr_text_centre(&_text[2], 170, game_pallet(GAME_GREEN));
		for (uint32_t j = 0; j < GAME_HISCORE_NAME_LEN; ++j) {
			const char s[2] = { game_initials_char(&g, j), 0 };
			const uint32_t c = j < g.pos ? GAME_WHITE : j == g.pos && ((frame >> 3) & 1) ? GAME_YELLOW : GAME_GREY;
			game_text_set(&_text[3 + j], s, 5);
			game_spr_text(&_text[3 + j], (GAME_SCREEN_W >> 1) - 84 + j * 64, 220, game_pallet(c));
		}
		game_spr_text_centre(&_text[6], 320, game_pallet(GAME_CYAN));
		game_spr_text_centre(&_text[7], 344, game_pallet(GAME_CYAN));
		rr_world_draw_stars(frame * 2);
		game_spr_end();
	}

	char name[GAME_HISCORE_NAME_LEN + 1];
	game_initials_name(&g, name);

	return rr_attract_hiscores(game_hiscore_insert(score, name));
}
