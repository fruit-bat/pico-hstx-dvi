// Copyright (c) 2024 fruit-bat.

// Generate DVI output using the command expander and TMDS encoder in HSTX.

// This example requires an external digital video connector connected to
// GPIOs 12 through 19 (the HSTX-capable GPIOs) with appropriate
// current-limiting resistors, e.g. 270 ohms. The pinout used in this example
// matches the Pico DVI Sock board, which can be soldered onto a Pico 2:
// https://github.com/Wren6991/Pico-DVI-Sock

#include "hstx_dvi_core.h"
#include "hstx_dvi_row_fifo.h"
#include "hstx_dvi_row_buf.h"
#include "hstx_dvi_sprite.h"
#include "pico/stdio.h"
#include "pico/stdlib.h"
#include <stdio.h>
#include <string.h>
#include "pico/sem.h" 
#include "font_inv.h"
#include "inv_input.h"
#include "inv_invaders.h"
#include "inv_bullets.h"
#include "inv_score.h"
#include "inv_pallet.h"
#include "inv_mot.h"
#include "inv_gun.h"

Tile32x16p2_t tile32x16p2_base = {
	{
	//   0123456789012345 
		0b00000000011111111111111000000000,
		0b00000000111111111111111100000000,
		0b00000001111111111111111110000000,
		0b00000011111111111111111111000000,
		0b00000111111111111111111111100000,
		0b00000111111111111111111111100000,
		0b00000111111111111111111111100000,
		0b00000111111111111111111111100000,
		0b00000111111111111111111111100000,
		0b00000111111111111111111111100000,
		0b00000111111111111111111111100000,
		0b00000111111111111111111111100000,
		0b00000111111100000000111111100000,
		0b00000111111000000000011111100000,
		0b00000111110000000000001111100000,
		0b00000111110000000000001111100000,
	}
};

void init_game() {

	uint32_t si = 0;

	const uint nb = 6;
	for(uint i = 0; i < nb; ++i){
		uint w1 = MODE_H_ACTIVE_PIXELS / nb;
		uint s = w1 * i;
		uint p = s + ((w1 - 32) / 2);
		init_sprite(si++, p, MODE_V_ACTIVE_LINES - 88, 32, 16, SF_ENABLE, &tile32x16p2_base, inv_pallet_green(), sprite_renderer_sprite_32x16_p1);
    	hstx_dvi_sprite_set_sprite_collision_mask(2, (SpriteCollisionMask)1);
	}

	si = inv_invaders_init(si);
	si = inv_bullets_init(si);
	si = inv_score_init(si);
	si = inv_mot_init(si);
	si = inv_gun_init(si);
}

int main(void)
{
    // Initialize stdio and GPIO 25 for the onboard LED
    stdio_init_all();
    gpio_init(25);
    gpio_set_dir(25, GPIO_OUT);
    gpio_put(25, 1); // Turn LED on

	// Initialize the gpio input
	init_inv_input();

	// Initialize the HSTX DVI sprite system
    hstx_dvi_sprite_init_all();

    sleep_ms(2000); // Allow time for initialization

    printf("HSTX DVI Sprite Test\n");

    init_game();
    printf("Game initialized\n");

    while(1) {
        hstx_dvi_sprite_wait_for_frame();

		inv_invader_update();
		inv_bullets_update();
		inv_mot_update();
		inv_gun_update();
		inv_score_update();
    }
}

