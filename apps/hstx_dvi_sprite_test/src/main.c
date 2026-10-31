// Copyright (c) 2024 fruit-bat.

// Generate DVI output using the command expander and TMDS encoder in HSTX.

// This example requires an external digital video connector connected to
// GPIOs 12 through 19 (the HSTX-capable GPIOs) with appropriate
// current-limiting resistors, e.g. 270 ohms. The pinout used in this example
// matches the Pico DVI Sock board, which can be soldered onto a Pico 2:
// https://github.com/Wren6991/Pico-DVI-Sock

#include "hstx_dvi_core.h"
#include "app_led.h"
#include "hstx_dvi_row_fifo.h"
#include "hstx_dvi_row_buf.h"
#include "hstx_dvi_sprite.h"
#include "pico/stdio.h"
#include "pico/stdlib.h"
#include <stdio.h>
#include <string.h>
#include "pico/sem.h"
#include "inv_input.h"
#include "inv_hiscore.h"
#include "game_pallet.h"
#include "inv_game.h"
#include "inv_attract.h"

// Cycles through the screens shown between games. Returns true when a player
// wants to play, or false once it has been round them all.
static bool attract_loop() {
	return inv_attract_title()
		|| inv_attract_score_table()
		|| inv_game_demo()
		|| inv_attract_hiscores(-1);
}

void main_loop() {
	bool play = false;
	while(1) {
		if (play || attract_loop()) {
			inv_game_play();
			// Pressing fire on the high score table goes straight into another game
			play = inv_attract_game_over();
		}
	}
}

int main(void)
{
    stdio_init_all();
    app_led_on();

	// Initialize the gpio input
	static const uint16_t pad_map[INV_INP_GPIO_COUNT] = INV_INP_PAD_MAP;
	game_input_init(INV_INP_GPIO_START, INV_INP_GPIO_COUNT, pad_map);

	game_pallet_init();
	game_hiscore_init(inv_hiscore_defaults());

	// Initialize the HSTX DVI sprite system
    hstx_dvi_sprite_init_all();

    sleep_ms(2000); // Allow time for initialization

    printf("HSTX DVI Sprite Test - Invaders\n");

	main_loop();
}

