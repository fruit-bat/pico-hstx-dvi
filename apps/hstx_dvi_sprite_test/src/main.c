// Copyright (c) 2024 Raspberry Pi (Trading) Ltd.

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

void __not_in_flash_func(sprite_renderer_invader_16x8_p1)(
	const void* d1,
	const void* d2,
	hstx_dvi_row_t* r,
	const int32_t y,
	const int32_t row,
	const SpriteId spriteId
) {
	const Tile16x8p2_t *tile16x8p2_invader = (Tile16x8p2_t *)d1;
	sprite_renderer_sprite_16x8_p1(
		&tile16x8p2_invader[(y >> 2) & 1],
		d2,
		r,
		y,
		row,
		spriteId
	);
}

const hstx_dvi_pixel_t pallet2_BlackGreen[] = {
    HSTX_DVI_PIXEL_RGB(0, 0, 0), // Black
    HSTX_DVI_PIXEL_RGB(0, 200, 0), // Green
};

const hstx_dvi_pixel_t pallet1_Green[] = {
    HSTX_DVI_PIXEL_RGB(0, 200, 0), // Green
};

const hstx_dvi_pixel_t  pallet1_Red[] = {
    HSTX_DVI_PIXEL_RGB(200, 0, 0), // Red
};

const hstx_dvi_pixel_t  pallet1_Blue[] = {
    HSTX_DVI_PIXEL_RGB(40, 40, 200), // Blue
};

const hstx_dvi_pixel_t  pallet1_Purple[] = {
    HSTX_DVI_PIXEL_RGB(200, 0, 200), // Purple
};

const hstx_dvi_pixel_t  pallet1_White[] = {
    HSTX_DVI_PIXEL_RGB(200, 200, 200), // White
};

Tile16x8p2_t tile16x8p2_invader[] = {
	{{
		0b0000000110000000,
		0b0000001111000000,
		0b0000011111100000,
		0b0000110110110000,
		0b0000111111110000,
		0b0000010000100000,
		0b0000100000010000,
		0b0000010000100000,
	}},
	{{
		0b0000000110000000,
		0b0000001111000000,
		0b0000011111100000,
		0b0000110110110000,
		0b0000111111110000,
		0b0000001001000000,
		0b0000010110100000,
		0b0000101001010000,
	}},
	{{
		0b0000100000100000,
		0b0000010001000000,
		0b0000111111100000,
		0b0001101110110000,
		0b0011111111111000,
		0b0101111111110100,
		0b0101000000010100,
		0b0000111011100000,
	}},
	{{
		0b0000100000100000,
		0b0000010001000000,
		0b0100111111100100,
		0b0101101110110100,
		0b0111111111111100,
		0b0011111111111000,
		0b0001000000010000,
		0b0010000000001000,
	}},
	{{
		0b0000001111000000,
		0b0001111111111000,
		0b0011111111111100,
		0b0011001111001100,
		0b0011111111111100,
		0b0000111001110000,
		0b0001100110011000,
		0b0000110000110000,
	}},
	{{
		0b0000001111000000,
		0b0001111111111000,
		0b0011111111111100,
		0b0011001111001100,
		0b0011111111111100,
		0b0000111001110000,
		0b0001100110011000,
		0b0011000000001100,
	}},
	{{
		0b0000000000000000,
		0b0000001111000000,
		0b0001111111111000,
		0b0011111111111100,
		0b0110110110110110,
		0b1111111111111111,
		0b0011100110011100,
		0b0001000000001000,
	}},
	{{
		0b0000000010000000,
		0b0000000111000000,
		0b0000000111000000,
		0b0000111111111000,
		0b0011111111111110,
		0b0011111111111110,
		0b0011111111111110,
		0b0011111111111110,
	}}
};

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

// Score 0000
// 1234567890
static uint8_t _score_text[16];
static TextGrid8_t _textGrid1 = {
	10, 
	_score_text,
	(uint8_t*)&font_8x8
};
static uint32_t _score = 0;
static uint32_t inv_index;
static uint32_t inv_count = 0;
static uint32_t mot_index;
static uint32_t gun_index;

static int32_t inv_v = 1;

void write_score() {
	sprintf((char*)_score_text, "Score %4.4ld", _score);
}

void init_game() {

	write_score();

    hstx_dvi_sprite_set_sprite_collision_mask(0, (SpriteCollisionMask)1);
    hstx_dvi_sprite_set_sprite_collision_mask(1, (SpriteCollisionMask)2);
    hstx_dvi_sprite_set_sprite_collision_mask(2, (SpriteCollisionMask)8);

	uint32_t si = 0;
	init_sprite(si++, 50, 15, 16, 8, SF_ENABLE, &tile16x8p2_invader, (hstx_dvi_pixel_t*)&pallet1_Green, sprite_renderer_invader_16x8_p1);
	init_sprite(si++, 66, 19, 16, 8, SF_ENABLE, &tile16x8p2_invader, (hstx_dvi_pixel_t*)&pallet1_Green, sprite_renderer_invader_16x8_p1);

	const uint nb = 6;
	for(uint i = 0; i < nb; ++i){
		uint w1 = MODE_H_ACTIVE_PIXELS / nb;
		uint s = w1 * i;
		uint p = s + ((w1 - 32) / 2);
		init_sprite(si++, p, MODE_V_ACTIVE_LINES - 88, 32, 16, SF_ENABLE, &tile32x16p2_base, (hstx_dvi_pixel_t*)&pallet1_Green, sprite_renderer_sprite_32x16_p1);

	}

	init_sprite(mot_index = si++, -1000, 9, 16, 8, SF_ENABLE, &tile16x8p2_invader[6], (void * const)&pallet1_Red, sprite_renderer_sprite_16x8_p1);
	init_sprite(gun_index = si++, 20, MODE_V_ACTIVE_LINES - 64, 16, 8, SF_ENABLE, &tile16x8p2_invader[7], (void * const)&pallet1_Green, sprite_renderer_sprite_16x8_p1);

	inv_index = si;
	uint32_t rt[5] = {0, 2, 2, 4, 4};
	hstx_dvi_pixel_t* rp[5] = {
        (hstx_dvi_pixel_t*)&pallet1_White, 
        (hstx_dvi_pixel_t*)&pallet1_Blue, 
        (hstx_dvi_pixel_t*)&pallet1_Blue, 
        (hstx_dvi_pixel_t*)&pallet1_Purple, 
        (hstx_dvi_pixel_t*)&pallet1_Purple};

	for(uint32_t x = 0; x < (11<< 1); ++x) {
		for(uint32_t y = 0; y < (5 << 1); ++y) {
			init_sprite(si, x << 4, 60 + (y << 4), 16, 8, SF_ENABLE, &tile16x8p2_invader[rt[y>>1]], rp[y >> 1], sprite_renderer_invader_16x8_p1);
			hstx_dvi_sprite_set_sprite_collision_mask(si, (SpriteCollisionMask)4);
			si++;
            inv_count++;
		}
	}
	init_sprite(si++, 8, 0, 16*8, 1*8, SF_ENABLE, &_textGrid1, (hstx_dvi_pixel_t*)&pallet1_Green, text_renderer_8x8_p1);

}

int main(void)
{
    // Initialize stdio and GPIO 25 for the onboard LED
    stdio_init_all();
    gpio_init(25);
    gpio_set_dir(25, GPIO_OUT);
    gpio_put(25, 1); // Turn LED on

    hstx_dvi_sprite_init_all();

    sleep_ms(2000); // Allow time for initialization

    printf("HSTX DVI Sprite Test\n");

    init_game();
    printf("Game initialized\n");

    while(1) {
        hstx_dvi_sprite_wait_for_frame();

		//continue;
    
		for (uint32_t i = 0; i < 2; ++i)
		{
			Sprite *sprite = &_sprites[i];
			if (_spriteCollisions.m[i]) sprite->d2 = (hstx_dvi_pixel_t*)&pallet1_Red;
		}
		_sprites[0].x++; if (_sprites[0].x > MODE_H_ACTIVE_PIXELS + 16) {
			_sprites[0].x = -16;
			_sprites[0].d2 = (hstx_dvi_pixel_t*)&pallet1_Blue;
		}
		_sprites[1].x--; if (_sprites[1].x < -16) {
			_sprites[1].x = MODE_H_ACTIVE_PIXELS + 16; 
			_sprites[1].d2 = (hstx_dvi_pixel_t*)&pallet1_Purple;
		}

		bool reverse = false;
		for (uint32_t i = inv_index; i < inv_index + inv_count; ++i)
		{
			Sprite *sprite = &_sprites[i];
			sprite->x += inv_v;
			if (inv_v > 0) {
				if(sprite->x + 16 >= MODE_H_ACTIVE_PIXELS) reverse = true;
			}
			else {
				if(sprite->x <= 0) reverse = true;
			}
		}
		if (reverse) inv_v = -inv_v;
		++_score;
		write_score();
    }
}

