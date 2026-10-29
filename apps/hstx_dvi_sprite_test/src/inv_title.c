#include "inv_title.h"
#include "game_pallet.h"
#include "game_util.h"
#include "font_inv.h"
#include <string.h>

#define INV_TITLE_MAX 160
// Frames for the title invaders to fly in to place
#define INV_TITLE_FLY 90

static SpriteId _inv_index = 0;

// Where each title invader rests, and the direction it flies in from
static uint32_t _title_count = 0;
static int16_t _title_x[INV_TITLE_MAX];
static int16_t _title_y[INV_TITLE_MAX];
static uint8_t _title_a[INV_TITLE_MAX];

// The little invaders of old, two frames each, stepping as they move
static void __not_in_flash_func(sprite_renderer_invader_16x8_p1)(
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

static Tile16x8p2_t tile16x8p2_invaders[] = {
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
	}}
};

SpriteId inv_title_init(SpriteId start) {
    _inv_index = start;

	char* inv_title = "INVADERS";
	uint32_t len = strlen(inv_title);
	uint32_t i = 0;
	uint8_t* font = (uint8_t*)font_8x8;
	const uint32_t sx = (MODE_H_ACTIVE_PIXELS - (len << (3+3))) >> 1;

	for(uint32_t y = 0; y < 8; ++y) {
        for(uint32_t x = 0; x < len; ++x) {
			const uint8_t c = inv_title[x];
			if (c < TEXT_GRID8_FIRST_CHAR || c > TEXT_GRID8_LAST_CHAR) continue; // Skip invalid characters
			const uint32_t k = (c - 32) << 3;
			const uint8_t d = font[k + y];
			for(uint32_t j = 0; j < 8; ++j) {
				if ((d & (1 << (7 - j))) && i < INV_TITLE_MAX) {
					const SpriteId si = start + i;
					_title_x[i] = sx + (((x << 3) + j) << 3);
					_title_y[i] = 60 + (y << 4);
					_title_a[i] = game_rand();
					// Create the invader sprite off screen, ready to fly in
					init_sprite(
						si,
						-32,
						-32,
						16,
						8,
						SF_ENABLE,
						&tile16x8p2_invaders[(y >> 1) & 2],
						game_pallet_rainbow(i),
						sprite_renderer_invader_16x8_p1);
					++i;
				}
			}
		}
	}
	_title_count = i;

    return start + i;
}

// Flies the title invaders in from all around, waves them about in a rainbow
// and then flies them back out again by the end frame
void __not_in_flash_func(inv_title_update)(uint32_t frame, uint32_t end) {
	const int32_t left = (int32_t)end - (int32_t)frame;
	int32_t phase = (int32_t)frame < left ? (int32_t)frame : left;
	if (phase < 0) phase = 0;
	if (phase > INV_TITLE_FLY) phase = INV_TITLE_FLY;
	// Ease out on the way in, ease in on the way out
	const int32_t ff = INV_TITLE_FLY * INV_TITLE_FLY;
	const int32_t e = ff - (INV_TITLE_FLY - phase) * (INV_TITLE_FLY - phase);
	const int32_t cx = (MODE_H_ACTIVE_PIXELS >> 1) - 8;
	const int32_t cy = MODE_V_ACTIVE_LINES >> 1;
	const int32_t radius = MODE_H_ACTIVE_PIXELS * 3 / 4;

	for (uint32_t i = 0; i < _title_count; ++i) {
		Sprite *sprite = hstx_dvi_sprite_get(_inv_index + i);
		const int32_t hx = _title_x[i];
		const int32_t hy = _title_y[i];
		const int32_t fx = cx + game_sin(_title_a[i] + 64) * radius / 127;
		const int32_t fy = cy + game_sin(_title_a[i]) * radius / 127;
		const int32_t wave = game_sin((hx >> 1) - (frame << 2)) * 8 / 127;
		sprite->x = fx + (hx - fx) * e / ff;
		sprite->y = fy + (hy + wave - fy) * e / ff;
		sprite->d2 = game_pallet_rainbow((hx >> 3) + (hy >> 4) - (frame >> 2));
	}
}
