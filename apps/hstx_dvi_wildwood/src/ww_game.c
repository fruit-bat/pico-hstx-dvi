#include "ww_game.h"
#include "hstx_dvi_sprite.h"
#include "ww_input.h"
#include "ww_world.h"
#include "ww_witch.h"
#include "ww_animal.h"
#include "ww_items.h"
#include "ww_hud.h"
#include "ww_art.h"
#include "ww_demo.h"
#include "game_render.h"
#include "game_pallet.h"
#include "game_fx.h"
#include "game_util.h"
#include <stdio.h>

#define WW_GAME_MESSAGE_FRAMES 240
#define WW_GAME_OVER_FRAMES 240
#define WW_GAME_DEMO_FRAMES (60 * 40)
// The wolf comes for her if she stays in one room for too long
#define WW_WOLF_DELAY (60 * 24)
#define WW_WOLF_DELAY_MIN (60 * 12)
#define WW_WOLF_DELAY_CHARM 90
#define WW_WOLF_WARNING 150
#define WW_EXPLORE_POINTS 10
#define WW_CREPT_POINTS 100
#define WW_ESCAPE_POINTS 100
#define WW_NIGHT_POINTS 1000
#define WW_HEART_POINTS 200
#define WW_HELPED_POINTS 50

static uint32_t _night = 1;
static uint32_t _room_frames = 0;
static uint32_t _message_timer = 0;
static GameText_t _message[3];
static uint32_t _frame = 0;

static void set_message(const uint32_t line, const char* s, const uint32_t scale) {
	game_text_set(&_message[line], s, scale);
}

static void clear_messages(void) {
	for (uint32_t i = 0; i < 3; ++i) _message[i].grid.w = 0;
}

static void draw_messages(void) {
	if (_message[0].grid.w) game_spr_text_centre(&_message[0], 150, game_pallet(GAME_WHITE));
	if (_message[1].grid.w) game_spr_text_centre(&_message[1], 190, game_pallet(GAME_YELLOW));
	if (_message[2].grid.w) game_spr_text_centre(&_message[2], 290, game_pallet(GAME_CYAN));
}

uint32_t ww_game_night(void) {
	return _night;
}

static void enter_room(uint32_t room) {
	ww_world_enter(room);
	ww_animal_enter(room, ww_witch_x(), ww_witch_y());
	game_fx_init();
	ww_hud_say_clear();
	_room_frames = 0;
}

static void night_start(void) {
	char t[GAME_TEXT_MAX];
	ww_world_new();
	ww_animal_populate(_night);
	uint32_t bear_room = WW_NO_ROOM;
	int32_t bx = 0, by = 0;
	ww_animal_bear(&bear_room, &bx, &by);
	ww_items_new_night(bear_room, bx, by);
	ww_hud_clear_pieces();
	ww_witch_reset(WW_ROOM_W / 2, 7 * WW_TILE + WW_TILE - 4);
	enter_room(ww_world_start());

	clear_messages();
	snprintf(t, sizeof(t), "NIGHT %lu", (unsigned long)_night);
	set_message(0, t, 3);
	set_message(1, "FIND THE MOON CHARM", 2);
	set_message(2, "HEAL  CALM  HELP  HUSH", 2);
	_message_timer = WW_GAME_MESSAGE_FRAMES;
}

static void change_room(int32_t dir) {
	const uint32_t next = ww_world_next(ww_world_room(), dir);
	if (next == WW_NO_ROOM) return;
	const bool wolf = ww_animal_wolf_here();
	const uint32_t crept = ww_animal_leave();
	const bool explored = ww_world_visits(next) == 0;
	ww_witch_enter(dir);
	enter_room(next);
	const int32_t x = ww_witch_x(), y = ww_witch_y() + WW_HUD_H - 50;
	if (explored) ww_hud_add_score(WW_EXPLORE_POINTS);
	if (crept) {
		ww_hud_add_score(WW_CREPT_POINTS * crept);
		ww_hud_say(x, y, "SHHH... CREPT PAST", GAME_CYAN);
	}
	else if (wolf) {
		ww_hud_add_score(WW_ESCAPE_POINTS);
		ww_hud_say(x, y, "PHEW! GOT AWAY", GAME_WHITE);
	}
}

// The wolf comes out of the way in furthest from the witch
static void wolf_arrives(void) {
	const uint32_t room = ww_world_room();
	static const int16_t gap[4][2] = {
		{ WW_ROOM_W / 2, 44 },
		{ WW_ROOM_W - 24, (WW_GAP_R0 + 2) * WW_TILE },
		{ WW_ROOM_W / 2, WW_ROOM_H - 6 },
		{ 24, (WW_GAP_R0 + 2) * WW_TILE },
	};
	int32_t best = -1, best_d = -1;
	for (uint32_t d = 0; d < 4; ++d) {
		if (ww_world_next(room, d) == WW_NO_ROOM) continue;
		const int32_t dist = game_abs(gap[d][0] - ww_witch_x()) + game_abs(gap[d][1] - ww_witch_y());
		if (dist > best_d) {
			best_d = dist;
			best = d;
		}
	}
	if (best < 0) return;
	ww_animal_wolf_spawn(gap[best][0], gap[best][1], _night);
}

static void wolf_update(void) {
	if (ww_animal_wolf_here() || !ww_witch_alive()) return;
	const uint32_t harder = _night > 5 ? 4 : _night - 1;
	uint32_t delay = WW_WOLF_DELAY - harder * 60 * 3;
	if (delay < WW_WOLF_DELAY_MIN) delay = WW_WOLF_DELAY_MIN;
	if (ww_hud_charm_whole() && !ww_items_flying()) delay = WW_WOLF_DELAY_CHARM;
	++_room_frames;
	if (_room_frames + WW_WOLF_WARNING == delay) {
		ww_hud_say(WW_ROOM_W / 2, WW_HUD_H + 8, "AWOOOOO!", GAME_BLUE);
	}
	if (_room_frames >= delay) wolf_arrives();
}

static void draw_scene(const uint32_t frame) {
	const int32_t wy = ww_witch_y();
	game_spr_begin();
	ww_art_set_clip(WW_HUD_H);
	// The room goes straight into the rows, so has to be first
	ww_world_draw(frame);
	ww_hud_draw(_night, frame, true);
	ww_items_draw_flying(frame);
	game_fx_draw_particles();
	ww_animal_draw_moods(frame);
	ww_witch_draw_bolt(frame);
	ww_animal_draw(frame, wy, true);
	ww_witch_draw(frame);
	ww_animal_draw(frame, wy, false);
	ww_items_draw(frame);
	// Text goes over everything else
	ww_hud_say_draw();
	game_fx_draw_popups();
	draw_messages();
	game_spr_end();
}

// The wolf has been calmed, so the night is over
static bool night_end(const bool demo) {
	char t[GAME_TEXT_MAX];
	const uint32_t night_bonus = WW_NIGHT_POINTS * _night;
	const uint32_t hearts = ww_witch_hearts();
	const uint32_t helped = ww_animal_helped();
	clear_messages();
	set_message(0, "THE OLD WOLF IS CALM", 2);
	snprintf(t, sizeof(t), "NIGHT %lu IS OVER", (unsigned long)_night);
	set_message(1, t, 2);
	_message_timer = 0;

	for (uint32_t frame = 0; frame < 480; ++frame, ++_frame) {
		hstx_dvi_sprite_wait_for_frame();
		if (demo && is_ww_input_play(game_input_pressed())) return true;
		if (frame == 120) {
			ww_hud_add_score(night_bonus);
			snprintf(t, sizeof(t), "NIGHT BONUS %lu", (unsigned long)night_bonus);
			set_message(2, t, 2);
		}
		if (frame == 240) {
			ww_hud_add_score(hearts * WW_HEART_POINTS);
			snprintf(t, sizeof(t), "HEARTS %lu X %u", (unsigned long)hearts, WW_HEART_POINTS);
			set_message(2, t, 2);
		}
		if (frame == 360) {
			ww_hud_add_score(helped * WW_HELPED_POINTS);
			snprintf(t, sizeof(t), "FRIENDS HELPED %lu X %u", (unsigned long)helped, WW_HELPED_POINTS);
			set_message(2, t, 2);
		}
		// The wolf and witch have a moment under the moon
		ww_animal_update(_frame);
		ww_items_update(_frame);
		ww_hud_say_update();
		game_fx_update();
		if ((frame & 7) == 0) {
			game_fx_explode(GAME_PX(game_rand() % GAME_SCREEN_W), GAME_PX(WW_HUD_H + game_rand() % WW_ROOM_H), GAME_COLOUR_RANDOM, 6, GAME_PX(1));
		}
		draw_scene(_frame);
	}
	++_night;
	night_start();
	return false;
}

// Runs a game until it is over. Returns true if the demo is taken over.
static bool game_run(const bool demo) {
	game_rand_seed(time_us_32());
	ww_hud_reset();
	game_fx_init();
	ww_hud_say_clear();
	_night = 1;
	night_start();
	if (demo) {
		game_input_pressed();
		ww_demo_reset();
	}

	uint8_t last = 0;
	uint32_t over = 0;
	for (uint32_t frame = 0; ; ++frame, ++_frame) {
		hstx_dvi_sprite_wait_for_frame();

		uint8_t input;
		if (demo) {
			if (is_ww_input_play(game_input_pressed())) return true;
			if (frame >= WW_GAME_DEMO_FRAMES) return false;
			input = ww_demo_input(_frame);
		}
		else {
			input = game_input_get();
		}
		const uint8_t pressed = input & ~last;
		last = input;

		if (over) {
			if (++over >= WW_GAME_OVER_FRAMES) return false;
			input = 0;
		}

		const int32_t dir = over ? -1 : ww_witch_update(input, pressed, _frame);
		if (dir >= 0) change_room(dir);

		int32_t bx, by;
		if (ww_witch_bolt(&bx, &by) && ww_animal_spell(bx, by)) ww_witch_bolt_end();
		ww_animal_update(_frame);
		ww_items_update(_frame);
		ww_hud_say_update();
		game_fx_update();
		if (!over) wolf_update();

		if (ww_witch_lost() && !over) {
			if (!ww_hud_take_life()) {
				if (demo) return false;
				clear_messages();
				set_message(0, "GAME OVER", 3);
				set_message(1, "SWEET DREAMS", 2);
				over = 1;
			}
			else {
				// Back to where she came in, with the room settled down
				int32_t ex, ey;
				ww_witch_entry(&ex, &ey);
				ww_witch_reset(ex, ey);
				ww_animal_reset_room();
				_room_frames = 0;
			}
		}

		if (ww_animal_wolf_calmed() && !over) {
			if (night_end(demo)) return true;
		}

		if (_message_timer && --_message_timer == 0) clear_messages();
		if (demo && !_message_timer) {
			clear_messages();
			if ((frame >> 5) & 1) set_message(2, "DEMO - PRESS FIRE", 2);
		}
		draw_scene(_frame);
	}
}

void ww_game_play(void) {
	game_run(false);
}

bool ww_game_demo(void) {
	// The demo should not count against the last player's score
	const uint32_t score = ww_hud_score();
	const bool taken_over = game_run(true);
	ww_hud_set_score(score);
	return taken_over;
}
