// Copyright (c) 2024 fruit-bat.

// A flip screen adventure in the style of the 1984 maze game, where a young
// witch helps the animals of the wood rather than fighting them.

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
#include "ww_input.h"
#include "game_pallet.h"
#include "ww_hiscore.h"
#include "ww_hud.h"
#include "ww_tiles.h"
#include "ww_witch.h"
#include "ww_animal.h"
#include "ww_items.h"
#include "ww_game.h"
#include "ww_attract.h"

// Cycles through the screens shown between games. Returns true when a player
// wants to play, or false once it has been round them all.
static bool attract_loop() {
	return ww_attract_title()
		|| ww_game_demo()
		|| ww_attract_hiscores(-1);
}

void main_loop() {
	bool play = false;
	while(1) {
		if (play || attract_loop()) {
			ww_game_play();
			// Pressing fire on the high score table goes straight into another game
			play = ww_attract_game_over();
		}
	}
}

int main(void)
{
	stdio_init_all();
	app_led_on();

	game_input_init(WW_INP_GPIO_START, WW_INP_GPIO_COUNT);
	game_pallet_init();
	game_hiscore_init(ww_hiscore_defaults());
	ww_hud_init();
	ww_hud_reset();
	ww_tiles_init();
	ww_witch_init();
	ww_animal_init();
	ww_items_init();

	// Initialize the HSTX DVI sprite system
	hstx_dvi_sprite_init_all();

	sleep_ms(2000); // Allow time for initialization

	printf("HSTX DVI Wildwood\n");

	main_loop();
}
