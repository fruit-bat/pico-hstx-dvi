// Copyright (c) 2024 fruit-bat.

// See rr_game.h. A game is the sections going by, one after the other, what
// comes at the ship in each sent in as it goes, and the warship at the end.
// Beating it goes round again, harder.

#include "rr_game.h"
#include "rr_sound.h"
#include "rr_input.h"
#include "rr_world.h"
#include "rr_player.h"
#include "rr_enemy.h"
#include "rr_boss.h"
#include "rr_hud.h"
#include "rr_fx.h"
#include "rr_gfx.h"
#include "rr_demo.h"
#include "rr_settings.h"
#include "rr_art.h"
#include "game_render.h"
#include "game_pallet.h"
#include "game_fx.h"
#include "game_util.h"
#include <stdio.h>

#define RR_GAME_MESSAGE_FRAMES 150
#define RR_GAME_OVER_FRAMES 240
#define RR_GAME_DEMO_FRAMES (60 * 40)
#define RR_GAME_CLEAR_FRAMES 360
// Fuel left after the warship is worth this much a frame of it
#define RR_GAME_FUEL_BONUS 2
// The sequencer's volume for music and sounds, as it starts
#define RR_VOLUME 100

typedef enum {
	RR_PHASE_FLY = 0,
	RR_PHASE_BOSS,
	RR_PHASE_CLEAR,
} RrPhase_e;

static GameText_t _message[3];
static uint32_t _message_timer = 0;
static uint32_t _loop = 0;
static uint32_t _section = 0;
static RrPhase_e _phase = RR_PHASE_FLY;
static uint32_t _phase_frame = 0;
static uint32_t _wave_timer = 0, _extra_timer = 0, _carrier_timer = 0, _giant_timer = 0;
static uint32_t _bonus = 0, _bonus_shown = 0;
// After the warship, until the land comes back
static bool _after_boss = false;

#if defined(HSTX_DVI_SPRITE_TIMING)
// How hard the renderer works: the least rows it kept ahead of the display in
// each second, printed, below 0 being a dropped frame
#define RR_LINE_CYCLES ((MODE_H_FRONT_PORCH + MODE_H_SYNC_WIDTH + MODE_H_BACK_PORCH + MODE_H_ACTIVE_PIXELS) * 10)
static int32_t _ahead = 0x7fffffff;
static uint32_t _line_max = 0;

static void timing(uint32_t frame) {
	const HstxDviSpriteTiming_t t = hstx_dvi_sprite_timing;
	if (!t.frame) return;
	if (t.ahead_min < _ahead) _ahead = t.ahead_min;
	if (t.line_max > _line_max) _line_max = t.line_max;
	if (frame % 60) return;
	const int32_t tenths = _ahead * 10 / RR_LINE_CYCLES;
	const int32_t a = tenths < 0 ? -tenths : tenths;
	printf("frame %lu section %lu-%lu ahead %s%d.%d rows, slowest row %lu%%, enemies %lu\n", (unsigned long)frame,
		(unsigned long)(_loop + 1), (unsigned long)(_section + 1), tenths < 0 ? "-" : "", (int)(a / 10), (int)(a % 10),
		(unsigned long)(_line_max * 100 / RR_LINE_CYCLES), (unsigned long)rr_enemy_count());
	_ahead = 0x7fffffff;
	_line_max = 0;
}
#else
static void timing(uint32_t frame) {
	(void)frame;
}
#endif

static void set_message(uint32_t line, const char* s) {
	game_text_set(&_message[line], s, line == 0 ? 3 : GAME_TEXT_BIG);
}

static void clear_messages(void) {
	for (uint32_t i = 0; i < 3; ++i) _message[i].grid.w = 0;
	_message_timer = 0;
}

static void show_section(uint32_t section) {
	char t[GAME_TEXT_MAX];
	clear_messages();
	snprintf(t, sizeof(t), "SECTION %lu-%lu", (unsigned long)(_loop + 1), (unsigned long)(section + 1));
	set_message(1, t);
	set_message(0, rr_world_section_name(section));
	_message_timer = RR_GAME_MESSAGE_FRAMES;
}

static void draw_messages(uint32_t frame) {
	if (_message[0].grid.w) game_spr_text_centre(&_message[0], 160, game_pallet_rainbow(frame >> 3));
	if (_message[1].grid.w) game_spr_text_centre(&_message[1], 206, game_pallet(GAME_WHITE));
	if (_message[2].grid.w) game_spr_text_centre(&_message[2], 240, game_pallet(GAME_YELLOW));
}

// A screen y for fliers, inside the room ahead
static int32_t room_y(int32_t h) {
	const int32_t x = (rr_world_camera() >> RR_FP) + GAME_SCREEN_W;
	int32_t top = rr_world_ceil_max(x, x + 200), bottom = rr_world_floor_min(x, x + 200);
	if (top < RR_PLAY_TOP) top = RR_PLAY_TOP;
	if (bottom > RR_PLAY_BOTTOM) bottom = RR_PLAY_BOTTOM;
	top += 20;
	bottom -= 20 + h;
	if (bottom <= top) return (top + bottom) / 2;
	return top + (int32_t)(game_rand() % (uint32_t)(bottom - top));
}

// Sooner each time round
static uint32_t gap(uint32_t frames) {
	const uint32_t l = _loop > 4 ? 4 : _loop;
	return frames - frames * l / 10 + game_rand() % (frames / 3 + 1);
}

// Sends in what the section has, as the ship goes through it
static void spawn(uint32_t section) {
	if (!rr_player_alive()) return;
	if (_wave_timer) --_wave_timer;
	if (_extra_timer) --_extra_timer;
	if (_carrier_timer) --_carrier_timer;
	switch (section) {
		case RR_SEC_RIDGES:
			if (!_wave_timer) {
				rr_enemy_wave(RR_EN_DART, 5, room_y(112) + 56, RR_PAT_SINE);
				_wave_timer = gap(300);
			}
			break;
		case RR_SEC_CAVERNS:
			if (!_wave_timer) {
				rr_enemy_wave(RR_EN_DART, 5, room_y(18), RR_PAT_HAIRPIN);
				_wave_timer = gap(320);
			}
			break;
		case RR_SEC_DEBRIS:
			if (!_extra_timer) {
				rr_enemy_spawn(RR_EN_METEOR, (rr_world_camera() >> RR_FP) + GAME_SCREEN_W + 8, room_y(32));
				_extra_timer = gap(70);
			}
			if (!_wave_timer) {
				rr_enemy_wave(RR_EN_DART, 5, room_y(112) + 56, RR_PAT_SINE);
				_wave_timer = gap(480);
			}
			break;
		case RR_SEC_CITY:
			if (!_wave_timer) {
				const bool hairpin = game_rand() & 1;
				rr_enemy_wave(RR_EN_DART, 5, hairpin ? room_y(18) : room_y(112) + 56, hairpin ? RR_PAT_HAIRPIN : RR_PAT_SINE);
				_wave_timer = gap(260);
			}
			if (!_extra_timer) {
				rr_enemy_spawn(RR_EN_SAUCER, (rr_world_camera() >> RR_FP) + GAME_SCREEN_W + 8, room_y(12));
				_extra_timer = gap(220);
			}
			break;
		case RR_SEC_FORTRESS:
			if (!_extra_timer) {
				rr_enemy_spawn(RR_EN_SAUCER, (rr_world_camera() >> RR_FP) + GAME_SCREEN_W + 8, room_y(12));
				_extra_timer = gap(170);
			}
			if (!_wave_timer) {
				rr_enemy_wave(RR_EN_DART, 4, room_y(18), RR_PAT_HAIRPIN);
				_wave_timer = gap(420);
			}
			break;
		default:
			return;
	}
	// From the second time round, the giants, a kind or two to each section
	if (_loop >= 1) {
		if (_giant_timer) --_giant_timer;
		if (!_giant_timer) {
			static const uint8_t kinds[RR_SECTIONS][2] = {
				[RR_SEC_RIDGES] = { RR_EN_WASP, RR_EN_WASP },
				[RR_SEC_CAVERNS] = { RR_EN_JELLY, RR_EN_JELLY },
				[RR_SEC_DEBRIS] = { RR_EN_JELLY, RR_EN_MOSQUITO },
				[RR_SEC_CITY] = { RR_EN_MOSQUITO, RR_EN_WASP },
				[RR_SEC_FORTRESS] = { RR_EN_MOSQUITO, RR_EN_MOSQUITO },
			};
			const uint32_t kind = kinds[section][game_rand() & 1];
			rr_enemy_giants(kind, kind == RR_EN_MOSQUITO ? 4 : kind == RR_EN_JELLY ? 3 : 2, room_y(60) + 30);
			_giant_timer = gap(460);
		}
	}
	// Now and then, a carrier with a capsule
	if (!_carrier_timer) {
		rr_enemy_spawn(RR_EN_CARRIER, (rr_world_camera() >> RR_FP) + GAME_SCREEN_W + 8, room_y(18));
		_carrier_timer = gap(600);
	}
}

static void start_loop(void) {
	rr_enemy_set_loop(_loop);
	_phase = RR_PHASE_FLY;
	_phase_frame = 0;
	_wave_timer = 120;
	_extra_timer = 200;
	_carrier_timer = 400;
	_giant_timer = 300;
}

// The play area and the score, power bar and so on, without messages
static void draw_play(uint32_t frame) {
	game_fx_draw_flash(frame);
	rr_player_draw(frame);
	rr_enemy_draw_bullets(frame);
	rr_fx_draw();
	rr_player_draw_shots(frame);
	rr_enemy_draw(frame);
	rr_boss_draw(frame);
	game_fx_draw_particles();
	rr_world_draw_terrain();
	rr_world_draw_back();
	// Text draws straight into the row, so goes on top only if added last
	rr_hud_draw(frame, true);
}

static void draw_scene(uint32_t frame) {
	game_spr_begin();
	draw_play(frame);
	game_fx_draw_popups();
	draw_messages(frame);
	game_spr_end();
}

// Stars behind the settings while paused: the game itself behind them, with
// the settings' text over it, was more than the renderer could draw in time
static void paused_back(uint32_t frame) {
	rr_world_draw_stars(frame * 2);
}

// The ship passing into a section, the warship coming, and beating it
static void update_phase(uint32_t frame) {
	const int32_t cam = rr_world_camera() >> RR_FP;
	const uint32_t here = rr_world_section(cam + RR_SHIP_W + 96);
	++_phase_frame;
	switch (_phase) {
		case RR_PHASE_FLY:
			if (here != _section) {
				_section = here;
				if (_section != RR_SEC_WARSHIP) show_section(_section);
			}
			// Once the land has gone, the warship comes
			if (_after_boss && rr_world_section(cam) != RR_SEC_WARSHIP) {
				_after_boss = false;
				rr_hud_set_loop(_loop);
			}
			if (!_after_boss && rr_world_section(cam) == RR_SEC_WARSHIP) {
				_phase = RR_PHASE_BOSS;
				_phase_frame = 0;
				rr_boss_start(_loop);
				clear_messages();
				set_message(0, "WARNING");
				set_message(1, "WARSHIP APPROACHING");
				_message_timer = RR_GAME_MESSAGE_FRAMES;
				game_sound_play(RR_SFX_ALARM);
				rr_music_warship();
			}
			else {
				spawn(rr_world_section(cam + GAME_SCREEN_W));
			}
			break;
		case RR_PHASE_BOSS:
			if (rr_boss_done()) {
				_phase = RR_PHASE_CLEAR;
				_phase_frame = 0;
				_bonus = (uint32_t)rr_player_fuel() * RR_GAME_FUEL_BONUS;
				_bonus_shown = 0;
				clear_messages();
				set_message(0, "WARSHIP DOWN");
				rr_music_landed();
				game_music_play(RR_TUNE_CLEAR);
			}
			break;
		case RR_PHASE_CLEAR: {
			// The fuel left counted into the score
			if (_phase_frame > 60 && _bonus_shown < _bonus) {
				const uint32_t step = _bonus - _bonus_shown < 100 ? _bonus - _bonus_shown : 100;
				_bonus_shown += step;
				rr_hud_add_score(step);
				if ((_phase_frame & 3) == 0) game_sound_play(RR_SFX_TALLY);
			}
			char t[GAME_TEXT_MAX];
			snprintf(t, sizeof(t), "FUEL BONUS %lu", (unsigned long)_bonus_shown);
			set_message(1, t);
			if (_phase_frame >= RR_GAME_CLEAR_FRAMES) {
				++_loop;
				_after_boss = true;
				rr_boss_init();
				rr_world_release_boss();
				clear_messages();
				start_loop();
				rr_music_flying();
			}
			break;
		}
	}
	if (_message_timer && --_message_timer == 0) clear_messages();
	rr_player_set_burning(_phase == RR_PHASE_FLY);
}

// Runs a game until it is over. Returns true if the demo is taken over.
static bool game_run(const bool demo) {
	game_rand_seed(demo ? 0x5eed1234 : time_us_32());
	rr_gfx_init();
	rr_hud_reset();
	game_fx_init();
	rr_fx_init();
	rr_world_init();
	rr_enemy_init();
	rr_boss_init();
	rr_player_init();
	// Started later on, from the settings, but not the demo
	_loop = demo ? 0 : rr_settings_start_round();
	_after_boss = false;
	rr_hud_set_loop(_loop);
	_section = RR_SEC_RIDGES;
	start_loop();
	show_section(_section);
	rr_music_flying();
	if (demo) game_input_pressed();

	uint16_t last = 0;
	uint32_t over = 0;
	for (uint32_t frame = 0; ; ++frame) {
		rr_wait_for_frame();

		uint16_t input;
		if (demo) {
			if (is_rr_input_play(game_input_pressed())) return true;
			if (frame >= RR_GAME_DEMO_FRAMES) return false;
			input = rr_demo_input(frame);
		}
		else {
			const uint16_t pressed = game_input_pressed();
			// Start pauses, with the settings over the game
			if (pressed & RR_INP_BIT_START) {
				game_sound_hold(RR_SFX_CHARGE, false);
				rr_settings_run(paused_back);
				last = game_input_get();
			}
			input = game_input_get();
		}
		const uint16_t pressed = input & ~last;
		last = input;

		rr_world_scroll(RR_SCROLL);
		rr_player_update(frame, input, pressed);
		rr_enemy_update(frame);
		rr_boss_update(frame);
		rr_fx_update();
		game_fx_update();

		if (rr_player_over()) {
			if (demo) return false;
			if (over == 0) {
				clear_messages();
				set_message(0, "GAME OVER");
				rr_music_landed();
				game_music_play(RR_TUNE_GAME_OVER);
			}
			if (++over >= RR_GAME_OVER_FRAMES) return false;
		}
		else {
			update_phase(frame);
		}

		if (demo && !_message_timer && _phase != RR_PHASE_CLEAR) {
			clear_messages();
			if ((frame >> 5) & 1) set_message(2, "DEMO - PRESS FIRE");
		}
		draw_scene(frame);
		timing(frame);
	}
}

void rr_game_play(void) {
	game_run(false);
	rr_music_landed();
	game_sound_hold(RR_SFX_CHARGE, false);
}

bool rr_game_demo(void) {
	// The demo should not count against the last player's score
	const uint32_t score = rr_hud_score();
	// Silent, if so set, until a player takes it over or it ends
	if (rr_settings_demo_silent()) game_seq_volume(0, 0);
	const bool taken_over = game_run(true);
	rr_music_landed();
	// Back to the title's tune, the demo's stopped while still silent
	game_music_stop(0);
	game_seq_volume(RR_VOLUME, RR_VOLUME);
	if (!taken_over) game_music_play(RR_TUNE_TITLE);
	rr_hud_set_score(score);
	game_sound_hold(RR_SFX_CHARGE, false);
	return taken_over;
}
