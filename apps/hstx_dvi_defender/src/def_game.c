#include "def_game.h"
#include "hstx_dvi_sprite.h"
#include "def_input.h"
#include "def_world.h"
#include "def_render.h"
#include "def_pallet.h"
#include "def_scanner.h"
#include "def_hud.h"
#include "def_fx.h"
#include "def_player.h"
#include "def_enemy.h"
#include "def_human.h"
#include "def_demo.h"
#include "def_util.h"
#include <stdio.h>

// Landers come down in batches
#define DEF_GAME_BATCH 5
#define DEF_GAME_BATCH_FRAMES 600
#define DEF_GAME_MAX_LANDERS 30
// Baiters are sent in if a wave takes too long
#define DEF_GAME_BAITER_START (60 * 40)
#define DEF_GAME_MAX_BAITERS 4
// The humanoids are replaced every few waves
#define DEF_GAME_RESTORE_WAVES 5
#define DEF_GAME_MESSAGE_FRAMES 150
#define DEF_GAME_WAVE_END_FRAMES 300
#define DEF_GAME_OVER_FRAMES 200
#define DEF_GAME_DEMO_FRAMES (60 * 30)

static uint32_t _wave = 1;
static uint32_t _wave_frame = 0;
static uint32_t _landers_left = 0;
static uint32_t _batch_timer = 0;
static uint32_t _baiter_timer = 0;
static uint32_t _message_timer = 0;
static DefText_t _message[3];

static void set_message(const uint32_t line, const char* s, const uint32_t scale) {
	def_text_set(&_message[line], s, scale);
}

static void clear_messages(void) {
	for (uint32_t i = 0; i < 3; ++i) _message[i].grid.w = 0;
}

// A world x position some way from the ship
static int32_t away_from_player(void) {
	return def_wrap(def_player_x() + DEF_PX(DEF_WORLD_W / 4 + def_rand() % (DEF_WORLD_W / 2)));
}

static void spawn_batch(void) {
	const uint32_t n = _landers_left < DEF_GAME_BATCH ? _landers_left : DEF_GAME_BATCH;
	for (uint32_t i = 0; i < n; ++i) {
		def_enemy_spawn(DEF_ENEMY_LANDER, away_from_player(), DEF_PX(DEF_PLAY_TOP + 10 + def_rand() % 60));
	}
	_landers_left -= n;
	_batch_timer = DEF_GAME_BATCH_FRAMES;
}

static void wave_start(void) {
	char t[DEF_TEXT_MAX];
	if ((_wave - 1) % DEF_GAME_RESTORE_WAVES == 0) {
		def_world_restore_planet();
		def_human_init(DEF_HUMAN_MAX);
	}
	def_enemy_init();
	def_enemy_set_wave(_wave);
	def_player_clear_lasers();

	_landers_left = 15 + 5 * (_wave - 1);
	if (_landers_left > DEF_GAME_MAX_LANDERS) _landers_left = DEF_GAME_MAX_LANDERS;
	spawn_batch();
	if (_wave >= 2) {
		const uint32_t bombers = _wave + 1 > 6 ? 6 : _wave + 1;
		for (uint32_t i = 0; i < bombers; ++i) {
			def_enemy_spawn(DEF_ENEMY_BOMBER, away_from_player(), DEF_PX(200));
		}
		const uint32_t pods = 1 + (_wave >> 1) > 4 ? 4 : 1 + (_wave >> 1);
		for (uint32_t i = 0; i < pods; ++i) {
			def_enemy_spawn(DEF_ENEMY_POD, away_from_player(), DEF_PX(DEF_PLAY_TOP + 40 + def_rand() % 200));
		}
	}
	_wave_frame = 0;
	_baiter_timer = 0;

	snprintf(t, sizeof(t), "ATTACK WAVE %lu", (unsigned long)_wave);
	clear_messages();
	set_message(0, t, 2);
	_message_timer = DEF_GAME_MESSAGE_FRAMES;
}

static void draw_messages(void) {
	if (_message[0].grid.w) def_spr_text_centre(&_message[0], 160, def_pallet(DEF_WHITE));
	if (_message[1].grid.w) def_spr_text_centre(&_message[1], 196, def_pallet(DEF_YELLOW));
	if (_message[2].grid.w) def_spr_text_centre(&_message[2], 250, def_pallet(DEF_CYAN));
}

static void draw_scene(const uint32_t frame, const bool demo) {
	def_spr_begin();
	def_fx_draw_flash(frame);
	def_hud_draw(_wave, true);
	def_scanner_begin(def_player_x());
	def_human_scan();
	def_enemy_scan();
	if (def_player_alive()) def_scanner_dot(def_player_x(), def_player_y(), DEF_WHITE);
	def_scanner_draw();
	draw_messages();
	def_world_draw_terrain();
	def_player_draw_lasers(frame);
	def_fx_draw_popups();
	def_player_draw(frame);
	def_enemy_draw(frame);
	def_human_draw(frame);
	def_enemy_draw_bullets(frame);
	def_fx_draw_particles();
	def_world_draw_stars(def_world_camera() >> (DEF_FP + 2), frame);
	def_spr_end();
}

// The tally between waves, with a bonus for each humanoid left alive
static bool wave_end(const bool demo) {
	char t[DEF_TEXT_MAX];
	const uint32_t humans = def_human_alive();
	const uint32_t bonus = 100 * (_wave < 5 ? _wave : 5);
	def_enemy_init();
	def_player_clear_lasers();
	clear_messages();
	snprintf(t, sizeof(t), "ATTACK WAVE %lu", (unsigned long)_wave);
	set_message(0, t, 2);
	set_message(1, "COMPLETED", 2);
	snprintf(t, sizeof(t), "BONUS X %lu", (unsigned long)bonus);
	set_message(2, t, 2);

	uint32_t shown = 0;
	for (uint32_t frame = 0; frame < DEF_GAME_WAVE_END_FRAMES; ++frame) {
		hstx_dvi_sprite_wait_for_frame();
		if (demo && is_def_input_play(def_input_pressed())) return true;
		// The humanoids are counted up one at a time
		if (frame >= 60 && shown < humans && (frame % 12) == 0) {
			++shown;
			def_hud_add_score(bonus);
		}
		def_fx_update();
		def_spr_begin();
		def_hud_draw(_wave, true);
		draw_messages();
		for (uint32_t i = 0; i < shown; ++i) {
			def_spr_add(DEF_SCREEN_W / 2 - DEF_HUMAN_MAX * 8 + i * 16, 300, 8, 8, def_human_icon(), def_pallet(DEF_PINK), sprite_renderer_sprite_8x8_p1);
		}
		def_world_draw_stars(frame, frame);
		def_spr_end();
	}
	++_wave;
	wave_start();
	return false;
}

// Keeps the wave going, sending in more enemies as needed
static void wave_update(void) {
	++_wave_frame;
	if (_landers_left && (--_batch_timer == 0 || def_enemy_count_hostile() == 0)) {
		spawn_batch();
	}
	if (_wave_frame > DEF_GAME_BAITER_START) {
		if (_baiter_timer) {
			--_baiter_timer;
		}
		else if (def_enemy_count(DEF_ENEMY_BAITER) < DEF_GAME_MAX_BAITERS) {
			const int32_t side = (def_rand() & 1) ? 1 : -1;
			def_enemy_spawn(DEF_ENEMY_BAITER, def_wrap(def_player_x() + side * DEF_PX(500)), DEF_PX(def_player_y()));
			_baiter_timer = _wave > 6 ? 300 : 600 - _wave * 50;
		}
	}
	// Losing all the humanoids takes the planet with them
	if (def_world_planet_alive() && def_human_alive() == 0) {
		def_world_destroy_planet();
		def_enemy_mutate_all();
		def_fx_flash(30);
		clear_messages();
		set_message(0, "PLANET DESTROYED", 2);
		_message_timer = DEF_GAME_MESSAGE_FRAMES;
	}
	if (_message_timer && --_message_timer == 0) clear_messages();
}

// Runs a game until it is over. Returns true if the demo is taken over.
static bool game_run(const bool demo) {
	def_rand_seed(time_us_32());
	def_hud_reset();
	def_fx_init();
	def_world_init();
	def_player_init();
	_wave = 1;
	wave_start();
	if (demo) def_input_pressed();

	uint8_t last = 0;
	uint32_t over = 0;
	for (uint32_t frame = 0; ; ++frame) {
		hstx_dvi_sprite_wait_for_frame();

		uint8_t input;
		if (demo) {
			if (is_def_input_play(def_input_pressed())) return true;
			if (frame >= DEF_GAME_DEMO_FRAMES) return false;
			input = def_demo_input(frame);
		}
		else {
			input = def_input_get();
		}
		const uint8_t pressed = input & ~last;
		last = input;

		def_player_update(frame, input, pressed);
		def_enemy_update(frame, !def_player_alive());
		def_human_update(frame);
		def_fx_update();

		if (def_player_over()) {
			if (demo) return false;
			if (over == 0) {
				clear_messages();
				set_message(0, "GAME OVER", 3);
			}
			if (++over >= DEF_GAME_OVER_FRAMES) return false;
		}
		else {
			wave_update();
			if (!_landers_left && def_enemy_count_hostile() == 0 && def_player_alive()) {
				if (wave_end(demo)) return true;
			}
		}

		if (demo && !_message_timer) {
			clear_messages();
			if ((frame >> 5) & 1) set_message(2, "DEMO - PRESS FIRE", 2);
		}
		draw_scene(frame, demo);
	}
}

void def_game_play(void) {
	game_run(false);
}

bool def_game_demo(void) {
	// The demo should not count against the last player's score
	const uint32_t score = def_hud_score();
	const bool taken_over = game_run(true);
	def_hud_set_score(score);
	return taken_over;
}
