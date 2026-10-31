#include "mm_game.h"
#include "hstx_dvi_sprite.h"
#include "mm_input.h"
#include "mm_map.h"
#include "mm_camera.h"
#include "mm_witch.h"
#include "mm_thing.h"
#include "mm_hud.h"
#include "mm_art.h"
#include "mm_demo.h"
#include "game_render.h"
#include "game_pallet.h"
#include "game_fx.h"
#include "game_util.h"
#include <stdio.h>

#define MM_GAME_MESSAGE_FRAMES 240
#define MM_GAME_OVER_FRAMES 240
#define MM_GAME_DEMO_FRAMES (60 * 45)
#define MM_GAME_TALLY_FRAMES 600
#define MM_NIGHT_POINTS 2000
#define MM_HEART_POINTS 200
#define MM_CHEERED_POINTS 50
#define MM_SWEET_POINTS 10
// The moon over the garden, in pixels
#define MM_MOON_X 72
#define MM_MOON_Y (27 * MM_TILE)

static uint32_t _night = 1;
static uint32_t _message_timer = 0;
static GameText_t _message[3];
static uint32_t _frame = 0;
static int32_t _start_cx = 4, _start_cy = MM_MAP_GROUND - 1;

static void set_message(const uint32_t line, const char* s, const uint32_t scale) {
	game_text_set(&_message[line], s, scale);
}

static void clear_messages(void) {
	for (uint32_t i = 0; i < 3; ++i) _message[i].grid.w = 0;
}

static void draw_messages(void) {
	if (_message[0].grid.w) game_spr_text_centre(&_message[0], 150, game_pallet(GAME_WHITE));
	if (_message[1].grid.w) game_spr_text_centre(&_message[1], 190, game_pallet(GAME_YELLOW));
	if (_message[2].grid.w) game_spr_text_centre(&_message[2], 290, game_pallet(GAME_PINK));
}

uint32_t mm_game_night(void) {
	return _night;
}

static void spawn(char ch, int32_t cx, int32_t cy, uint32_t group) {
	if (ch == '@') {
		_start_cx = cx;
		_start_cy = cy;
	}
	else {
		mm_thing_spawn(ch, cx, cy, group);
	}
}

static void night_start(void) {
	char t[GAME_TEXT_MAX];
	mm_thing_reset(_night);
	mm_map_reset(spawn);
	mm_witch_reset(_start_cx, _start_cy);
	mm_camera_reset(mm_witch_x(), mm_witch_y());
	game_fx_init();
	mm_hud_say_clear();

	clear_messages();
	snprintf(t, sizeof(t), "NIGHT %lu", (unsigned long)_night);
	set_message(0, t, 3);
	set_message(1, "FIND THE LOST KITTENS", 2);
	set_message(2, "CHEER THE SPOOKS WITH SPELLS", 2);
	_message_timer = MM_GAME_MESSAGE_FRAMES;
}

static void draw_moon(void) {
	const int32_t sy = mm_screen_y(MM_MOON_Y);
	if (sy > -64 && sy < GAME_SCREEN_H) mm_gfx_draw(&mm_art_moon, mm_pal_moon, mm_screen_x(MM_MOON_X), sy, 2, 0);
}

static void draw_world(const uint32_t frame) {
	// The house goes straight into the rows, so everything else is drawn after
	mm_tiles_draw(mm_map_gfx(), MM_MAP_W, mm_camera_x(), mm_camera_y(), frame);
	draw_moon();
	mm_thing_draw(frame);
	mm_witch_draw(frame);
	game_fx_draw_particles();
}

static void draw_scene(const uint32_t frame) {
	game_spr_begin();
	draw_world(frame);
	mm_hud_draw(_night, frame, true);
	// Text goes over everything else
	mm_hud_say_draw();
	game_fx_draw_popups();
	draw_messages();
	game_spr_end();
}

void mm_game_tour_start(void) {
	mm_thing_reset(1);
	mm_map_reset(spawn);
	// Hazel waits out of the way, off the bottom of the house
	mm_witch_reset(0, MM_MAP_H + 4);
	mm_camera_reset(MM_VIEW_W / 2, MM_VIEW_H);
	game_fx_set_view(mm_camera_fx_view());
	game_fx_init();
	mm_hud_say_clear();
}

void mm_game_tour_frame(uint32_t frame) {
	mm_thing_update(frame);
	mm_hud_say_update();
	game_fx_update();
	mm_tiles_draw(mm_map_gfx(), MM_MAP_W, mm_camera_x(), mm_camera_y(), frame);
	draw_moon();
	mm_thing_draw(frame);
	game_fx_draw_particles();
}

static void update_world(void) {
	mm_thing_update(_frame);
	mm_map_update_plates();
	mm_hud_say_update();
	game_fx_update();
	mm_camera_follow(mm_witch_x(), mm_witch_y(), mm_witch_facing(), mm_witch_grounded());
}

// All the kittens are found, so the night is over
static bool night_end(const bool demo) {
	char t[GAME_TEXT_MAX];
	const uint32_t night_bonus = MM_NIGHT_POINTS * _night;
	const uint32_t hearts = mm_witch_hearts();
	const uint32_t cheered = mm_thing_cheered();
	const uint32_t sweets = mm_thing_sweets_found();
	clear_messages();
	set_message(0, "ALL THE KITTENS ARE SAFE", 2);
	snprintf(t, sizeof(t), "NIGHT %lu IS OVER", (unsigned long)_night);
	set_message(1, t, 2);
	_message_timer = 0;
	mm_witch_set_cheer(true);

	for (uint32_t frame = 0; frame < MM_GAME_TALLY_FRAMES; ++frame, ++_frame) {
		hstx_dvi_sprite_wait_for_frame();
		if (demo && is_mm_input_play(game_input_pressed())) return true;
		if (frame == 120) {
			mm_hud_add_score(night_bonus);
			snprintf(t, sizeof(t), "NIGHT BONUS %lu", (unsigned long)night_bonus);
			set_message(2, t, 2);
		}
		if (frame == 240) {
			mm_hud_add_score(hearts * MM_HEART_POINTS);
			snprintf(t, sizeof(t), "HEARTS %lu X %u", (unsigned long)hearts, MM_HEART_POINTS);
			set_message(2, t, 2);
		}
		if (frame == 360) {
			mm_hud_add_score(cheered * MM_CHEERED_POINTS);
			snprintf(t, sizeof(t), "SPOOKS CHEERED %lu X %u", (unsigned long)cheered, MM_CHEERED_POINTS);
			set_message(2, t, 2);
		}
		if (frame == 480) {
			mm_hud_add_score(sweets * MM_SWEET_POINTS);
			snprintf(t, sizeof(t), "SWEETS %lu X %u", (unsigned long)sweets, MM_SWEET_POINTS);
			set_message(2, t, 2);
		}
		mm_witch_update(0, _frame);
		update_world();
		if ((frame & 7) == 0) {
			const int32_t x = mm_camera_x() + (int32_t)(game_rand() % MM_VIEW_W);
			const int32_t y = mm_camera_y() + (int32_t)(game_rand() % MM_VIEW_H);
			game_fx_explode(GAME_PX(x), GAME_PX(y), GAME_COLOUR_RANDOM, 6, GAME_PX(1));
		}
		draw_scene(_frame);
	}
	++_night;
	night_start();
	return false;
}

// Runs a game until it is over. Returns true if the demo is taken over.
static bool game_run(const bool demo) {
	game_rand_seed(demo ? 1 : time_us_32());
	game_fx_set_view(mm_camera_fx_view());
	mm_hud_reset();
	_night = 1;
	night_start();
	if (demo) {
		game_input_pressed();
		mm_demo_reset();
	}

	uint32_t over = 0;
	for (uint32_t frame = 0; ; ++frame, ++_frame) {
		hstx_dvi_sprite_wait_for_frame();

		uint8_t input;
		if (demo) {
			if (is_mm_input_play(game_input_pressed())) return true;
			if (frame >= MM_GAME_DEMO_FRAMES) return false;
			input = mm_demo_input(_frame);
		}
		else {
			input = game_input_get();
		}
		if (over) {
			if (++over >= MM_GAME_OVER_FRAMES) return false;
			input = 0;
		}

		mm_witch_update(input, _frame);
		update_world();

		if (mm_witch_fainted() && !over) {
			if (!mm_hud_take_life()) {
				if (demo) return false;
				clear_messages();
				set_message(0, "GAME OVER", 3);
				set_message(1, "SWEET DREAMS", 2);
				over = 1;
			}
			else {
				mm_witch_respawn();
				mm_camera_reset(mm_witch_x(), mm_witch_y());
			}
		}

		if (mm_thing_kittens_found() >= MM_KITTENS && !over) {
			if (night_end(demo)) return true;
		}

		if (_message_timer && --_message_timer == 0) clear_messages();
		if (demo && !_message_timer) {
			clear_messages();
			if ((frame >> 5) & 1) set_message(2, "DEMO - PRESS JUMP", 2);
		}
		draw_scene(_frame);
	}
}

void mm_game_play(void) {
	game_run(false);
}

bool mm_game_demo(void) {
	// The demo should not count against the last player's score
	const uint32_t score = mm_hud_score();
	const bool taken_over = game_run(true);
	mm_hud_set_score(score);
	return taken_over;
}
