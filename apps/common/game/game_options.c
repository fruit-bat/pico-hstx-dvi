// See game_options.h.

#include "game_options.h"
#include "game_sound.h"
#include "game_input.h"
#include "game_render.h"
#include "game_pallet.h"
#include <stdio.h>

typedef enum {
	OPTION_MUSIC = 0,
	OPTION_SOUND,
	OPTIONS
} Option_e;

#define TOP 120
#define LINE 36

// Text has to outlive the screen that shows it, as the renderer keeps
// drawing the last frame until the next screen has been set up
static GameText_t _title, _lines[OPTIONS], _help[2];
static uint32_t _sound = 0;

void game_options_run(const GameOptions_t* o) {
	uint32_t option = OPTION_MUSIC;
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
		if ((pressed & (o->up | o->down)) && option == OPTION_SOUND) game_sound_test_stop();
		if (pressed & o->down) option = (option + 1) % OPTIONS;
		if (pressed & o->up) option = (option + OPTIONS - 1) % OPTIONS;
		const int32_t change = (pressed & o->right ? 1 : 0) - (pressed & o->left ? 1 : 0);
		switch (option) {
			case OPTION_MUSIC:
				if (change || (pressed & o->fire)) game_music_set_on(!game_music_on());
				break;
			case OPTION_SOUND:
				if (change && tests) {
					game_sound_test_stop();
					_sound = (_sound + tests + (uint32_t)change) % tests;
				}
				if ((pressed & o->fire) && tests) game_sound_test(_sound);
				break;
			default:
				break;
		}

		char s[GAME_TEXT_MAX];
		const bool flash = (frame >> 4) & 1;
		snprintf(s, sizeof(s), "MUSIC  %c %-15s %c", option == OPTION_MUSIC && flash ? '<' : ' ',
			game_music_on() ? "ON" : "OFF", option == OPTION_MUSIC && flash ? '>' : ' ');
		game_text_set(&_lines[OPTION_MUSIC], s, GAME_TEXT_BIG);
		snprintf(s, sizeof(s), "SOUND  %c %-15s %c", option == OPTION_SOUND && flash ? '<' : ' ',
			tests ? game_sound_test_name(_sound) : "NONE", option == OPTION_SOUND && flash ? '>' : ' ');
		game_text_set(&_lines[OPTION_SOUND], s, GAME_TEXT_BIG);

		game_spr_begin();
		game_spr_text_centre(&_title, 50, game_pallet_rainbow(frame >> 3));
		// The same width, so lined up
		const int32_t x = (GAME_SCREEN_W - game_text_width(&_lines[0])) >> 1;
		for (uint32_t i = 0; i < OPTIONS; ++i) {
			game_spr_text(&_lines[i], x, TOP + i * LINE, game_pallet(i == option ? GAME_YELLOW : GAME_WHITE));
		}
		game_spr_text_centre(&_help[0], 396, game_pallet(GAME_CYAN));
		game_spr_text_centre(&_help[1], 418, game_pallet(GAME_CYAN));
		// Added last, so behind
		if (o->draw_back) o->draw_back(frame);
		game_spr_end();
	}
	game_sound_test_stop();
	// Forget about the button that closed it
	game_input_pressed();
}
