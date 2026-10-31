// Copyright (c) 2024 fruit-bat.

// A game in the style of the 1981 arcade classic, using sprites on DVI.

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
#include "def_input.h"
#include "game_pallet.h"
#include "def_hiscore.h"
#include "def_hud.h"
#include "def_world.h"
#include "def_game.h"
#include "def_attract.h"

// Cycles through the screens shown between games. Returns true when a player
// wants to play, or false once it has been round them all.
static bool attract_loop() {
	return def_attract_title()
		|| def_game_demo()
		|| def_attract_hiscores(-1);
}

void main_loop() {
	bool play = false;
	while(1) {
		if (play || attract_loop()) {
			def_game_play();
			// Pressing fire on the high score table goes straight into another game
			play = def_attract_game_over();
		}
	}
}

int main(void)
{
	stdio_init_all();
	app_led_on();

	static const uint16_t pad_map[DEF_INP_GPIO_COUNT] = DEF_INP_PAD_MAP;
	game_input_init(DEF_INP_GPIO_START, DEF_INP_GPIO_COUNT, pad_map);
	game_pallet_init();
	game_hiscore_init(def_hiscore_defaults());
	game_fx_set_view(def_world_fx_view());
	def_hud_reset();
	def_world_init();

	// Initialize the HSTX DVI sprite system
	hstx_dvi_sprite_init_all();

	sleep_ms(2000); // Allow time for initialization

	printf("HSTX DVI Defender\n");

	main_loop();
}
