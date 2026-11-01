// Copyright (c) 2024 fruit-bat.

// A game in the style of Elite, drawn in 3D with sprites on DVI.
//
// This example requires an external digital video connector connected to
// GPIOs 12 through 19 (the HSTX-capable GPIOs) with appropriate
// current-limiting resistors, e.g. 270 ohms. The pinout used in this example
// matches the Pico DVI Sock board, which can be soldered onto a Pico 2:
// https://github.com/Wren6991/Pico-DVI-Sock

#include "hstx_dvi_core.h"
#include "hstx_dvi_sprite.h"
#include "app_led.h"
#include "pico/stdlib.h"
#include <stdio.h>
#include "game_pallet.h"
#include "el_attract.h"
#include "el_game.h"
#include "el_hud.h"
#include "el_input.h"
#include "el_screen.h"
#include "el_space.h"

int main(void)
{
	stdio_init_all();
	app_led_on();

	static const uint16_t pad_map[EL_INP_GPIO_COUNT] = EL_INP_PAD_MAP;
	game_input_init(EL_INP_GPIO_START, EL_INP_GPIO_COUNT, pad_map);
	game_pallet_init();
	hud_init();
	screen_init();
	dust_init();

	// Initialize the HSTX DVI sprite system
	hstx_dvi_sprite_init_all();

	sleep_ms(2000); // Allow time for initialization

	printf("HSTX DVI Elite\n");

	for (;;) {
		attract_run();
		game_play();
	}
}
