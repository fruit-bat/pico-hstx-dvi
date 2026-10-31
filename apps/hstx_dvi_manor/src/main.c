// Copyright (c) 2024 fruit-bat.

// A scrolling platform game in a haunted house, where a young witch cheers
// up the spooks with spells and solves puzzles to find the lost kittens.

// This example requires an external digital video connector connected to
// GPIOs 12 through 19 (the HSTX-capable GPIOs) with appropriate
// current-limiting resistors, e.g. 270 ohms. The pinout used in this example
// matches the Pico DVI Sock board, which can be soldered onto a Pico 2:
// https://github.com/Wren6991/Pico-DVI-Sock

#include "hstx_dvi_core.h"
#include "app_led.h"
#include "hstx_dvi_sprite.h"
#include "pico/stdio.h"
#include "pico/stdlib.h"
#include <stdio.h>
#include "mm_input.h"
#include "game_pallet.h"
#include "mm_hiscore.h"
#include "mm_hud.h"
#include "mm_tiles.h"
#include "mm_art.h"
#include "mm_witch.h"
#include "mm_game.h"
#include "mm_attract.h"

// Cycles through the screens shown between games. Returns true when a player
// wants to play, or false once it has been round them all.
static bool attract_loop() {
	return mm_attract_title()
		|| mm_attract_roll()
		|| mm_game_demo()
		|| mm_attract_hiscores(-1);
}

void main_loop() {
	bool play = false;
	while(1) {
		if (play || attract_loop()) {
			mm_game_play();
			// Pressing jump on the high score table goes straight into another game
			play = mm_attract_game_over();
		}
	}
}

int main(void)
{
	stdio_init_all();
	app_led_on();

	static const uint16_t pad_map[MM_INP_GPIO_COUNT] = MM_INP_PAD_MAP;
	game_input_init(MM_INP_GPIO_START, MM_INP_GPIO_COUNT, pad_map);
	game_pallet_init();
	game_hiscore_init(mm_hiscore_defaults());
	mm_hud_init();
	mm_hud_reset();
	mm_tiles_init();
	mm_art_init();
	mm_witch_init();

	// Initialize the HSTX DVI sprite system
	hstx_dvi_sprite_init_all();

	sleep_ms(2000); // Allow time for initialization

	printf("HSTX DVI Moonlit Manor\n");

	main_loop();
}
