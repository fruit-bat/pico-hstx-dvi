// Copyright (c) 2024 fruit-bat.

// Rift Runner: a side scrolling shooter with big, bold sprites on DVI. Fly
// through the sections of the rift, keeping up the fuel, picking power ups
// from the bar, and sink the warship at the end.

// This example requires an external digital video connector connected to
// GPIOs 12 through 19 (the HSTX-capable GPIOs) with appropriate
// current-limiting resistors, e.g. 270 ohms. The pinout used in this example
// matches the Pico DVI Sock board, which can be soldered onto a Pico 2:
// https://github.com/Wren6991/Pico-DVI-Sock

#include "hstx_dvi_core.h"
#include "hstx_dvi_sprite.h"
#include "app_led.h"
#include "pico/stdio.h"
#include "pico/stdlib.h"
#include <stdio.h>
#include "rr_sound.h"
#include "rr_input.h"
#include "rr_gfx.h"
#include "rr_hud.h"
#include "rr_world.h"
#include "rr_game.h"
#include "rr_attract.h"
#include "game_pallet.h"
#include "game_hiscore.h"
#include "game_fx.h"

static const GameHiScore_t _hiscores[GAME_HISCORE_COUNT] = {
	{ 50000, "BAT" },
	{ 45000, "RFT" },
	{ 40000, "HST" },
	{ 35000, "DVI" },
	{ 30000, "RP2" },
	{ 25000, "ZAP" },
	{ 20000, "ACE" },
	{ 15000, "FUE" },
	{ 10000, "ORB" },
	{  5000, "BUG" },
};

// Goes round the screens shown between games. Returns true when a player
// wants to play, or false once it has been round them all.
static bool attract_loop(void) {
	return rr_attract_title()
		|| rr_attract_roll()
		|| rr_attract_giants()
		|| rr_game_demo()
		|| rr_attract_howto()
		|| rr_attract_hiscores(-1);
}

static void main_loop(void) {
	bool play = false;
	while (1) {
		if (play || attract_loop()) {
			rr_game_play();
			// Pressing fire on the high score table goes straight into another game
			play = rr_attract_game_over();
		}
	}
}

int main(void) {
	stdio_init_all();
	app_led_on();

	static const uint16_t pad_map[RR_INP_PAD_COUNT] = RR_INP_PAD_MAP;
	game_input_init_pad(RR_INP_GPIO_START, RR_INP_GPIO_COUNT, pad_map, RR_INP_PAD_COUNT);
	game_pallet_init();
	game_hiscore_init(_hiscores);
	game_fx_set_view(rr_world_fx_view());
	rr_hud_reset();
	rr_gfx_init();
	rr_world_init();

	// Initialize the HSTX DVI sprite system
	hstx_dvi_sprite_init_all();
	// After the display has set the clocks, which the MIDI UART's baud rate comes from
	rr_sound_init();

	sleep_ms(2000); // Allow time for initialization

	printf("HSTX DVI Rift Runner\n");

	main_loop();
}
