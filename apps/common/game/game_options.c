// See game_options.h.

#include "game_options.h"
#include "game_sound.h"
#include "game_input.h"
#include "game_render.h"
#include "game_pallet.h"
#include <stdio.h>

#define TOP 120
#define LINE 36
// Music, the game's own, and the sound test
#define MAX_EXTRA 6
#define MAX_LINES (MAX_EXTRA + 2)

// Text has to outlive the screen that shows it, as the renderer keeps
// drawing the last frame until the next screen has been set up
static GameText_t _title, _lines[MAX_LINES], _help[2];
static uint32_t _sound = 0;

void game_options_run(const GameOptions_t* o) {
	const uint32_t extra = o->extra ? (o->extra_n < MAX_EXTRA ? o->extra_n : MAX_EXTRA) : 0;
	// Music first, then the game's own, then the sound test
	const uint32_t lines = extra + 2;
	const uint32_t sound_line = lines - 1;
	uint32_t option = 0;
	const uint32_t tests = game_sound_tests();
	game_text_set(&_title, "SETTINGS", 3);
	game_text_set(&_help[0], "UP/DOWN CHOOSE  LEFT/RIGHT CHANGE", GAME_TEXT_BIG);
	game_text_set(&_help[1], "FIRE PLAYS A SOUND  START GOES BACK", GAME_TEXT_BIG);
	// Forget about the button that opened it
	game_input_pressed();

	for (uint32_t frame = 0; ; ++frame) {
		o->wait();
		const uint16_t pressed = game_input_pressed();
		if (pressed & (o->start | o->back)) break;
		if ((pressed & (o->up | o->down)) && option == sound_line) game_sound_test_stop();
		if (pressed & o->down) option = (option + 1) % lines;
		if (pressed & o->up) option = (option + lines - 1) % lines;
		const int32_t change = (pressed & o->right ? 1 : 0) - (pressed & o->left ? 1 : 0);
		if (option == 0) {
			if (change || (pressed & o->fire)) game_music_set_on(!game_music_on());
		}
		else if (option == sound_line) {
			if (change && tests) {
				game_sound_test_stop();
				_sound = (_sound + tests + (uint32_t)change) % tests;
			}
			if ((pressed & o->fire) && tests) game_sound_test(_sound);
		}
		else {
			const GameOption_t* e = &o->extra[option - 1];
			const int32_t step = change ? change : (pressed & o->fire) ? 1 : 0;
			if (step && e->count) {
				*e->value = (uint8_t)((*e->value + e->count + step) % e->count);
				if (e->changed) e->changed();
			}
		}

		char s[GAME_TEXT_MAX];
		const bool flash = (frame >> 4) & 1;
		for (uint32_t i = 0; i < lines; ++i) {
			const char* name;
			const char* value;
			if (i == 0) {
				name = "MUSIC";
				value = game_music_on() ? "ON" : "OFF";
			}
			else if (i == sound_line) {
				name = "SOUND";
				value = tests ? game_sound_test_name(_sound) : "NONE";
			}
			else {
				const GameOption_t* e = &o->extra[i - 1];
				name = e->name;
				value = e->count ? e->choices[*e->value % e->count] : "";
			}
			// Lined up with the longest name when the game has its own
			const char l = option == i && flash ? '<' : ' ', r = option == i && flash ? '>' : ' ';
			if (extra) snprintf(s, sizeof(s), "%-12s %c %-15s %c", name, l, value, r);
			else snprintf(s, sizeof(s), "%-6s %c %-15s %c", name, l, value, r);
			game_text_set(&_lines[i], s, GAME_TEXT_BIG);
		}

		game_spr_begin();
		// First, as the text draws straight into the row over what is before it
		if (o->draw_back) o->draw_back(frame);
		game_spr_text_centre(&_title, 50, game_pallet_rainbow(frame >> 3));
		// The same width, so lined up
		const int32_t x = (GAME_SCREEN_W - game_text_width(&_lines[0])) >> 1;
		for (uint32_t i = 0; i < lines; ++i) {
			game_spr_text(&_lines[i], x, TOP + i * LINE, game_pallet(i == option ? GAME_YELLOW : GAME_WHITE));
		}
		game_spr_text_centre(&_help[0], 396, game_pallet(GAME_CYAN));
		game_spr_text_centre(&_help[1], 418, game_pallet(GAME_CYAN));
		game_spr_end();
	}
	game_sound_test_stop();
	// Forget about the button that closed it
	game_input_pressed();
}
