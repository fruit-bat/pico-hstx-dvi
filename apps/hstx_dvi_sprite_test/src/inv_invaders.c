#include "inv_invaders.h"

#define INV_INVADER_COLS 22
#define INV_INVADER_ROWS 10
#define INV_INVADER_COUNT (INV_INVADER_COLS * INV_INVADER_ROWS)
#define INV_INVADER_COLLISION_MASK ((SpriteCollisionMask)4)

static SpriteId _inv_index = 0;
static int32_t inv_v = 1;

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

static const hstx_dvi_pixel_t  pallet1_Blue[] = {
    HSTX_DVI_PIXEL_RGB(40, 40, 200), // Blue
};

static const hstx_dvi_pixel_t  pallet1_Purple[] = {
    HSTX_DVI_PIXEL_RGB(200, 0, 200), // Purple
};

static const hstx_dvi_pixel_t  pallet1_White[] = {
    HSTX_DVI_PIXEL_RGB(200, 200, 200), // White
};

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

SpriteId inv_invaders_init(SpriteId start) {
    _inv_index = start;

    SpriteId si = _inv_index;

	uint32_t rt[5] = {0, 2, 2, 4, 4};
	hstx_dvi_pixel_t* rp[5] = {
        (hstx_dvi_pixel_t*)&pallet1_White, 
        (hstx_dvi_pixel_t*)&pallet1_Blue, 
        (hstx_dvi_pixel_t*)&pallet1_Blue, 
        (hstx_dvi_pixel_t*)&pallet1_Purple, 
        (hstx_dvi_pixel_t*)&pallet1_Purple};

	for(uint32_t x = 0; x < INV_INVADER_COLS; ++x) {
		for(uint32_t y = 0; y < INV_INVADER_ROWS; ++y) {
            // Create the invader sprite
			init_sprite(
                si, 
                x << 4, 
                60 + (y << 4), 
                16, 
                8, 
                SF_ENABLE, 
                &tile16x8p2_invaders[rt[y>>1]], 
                rp[y >> 1], 
                sprite_renderer_invader_16x8_p1);

			hstx_dvi_sprite_set_sprite_collision_mask(si, INV_INVADER_COLLISION_MASK);
			si++;
		}
	}

    return si;
}

void inv_invader_update() {
    bool reverse = false;
    for (uint32_t i = 0; i < INV_INVADER_COUNT; ++i)
    {
        SpriteId si = _inv_index + i;
        Sprite *sprite = hstx_dvi_sprite_get(si);
        if (sprite->f & SF_ENABLE) {
            if (_spriteCollisions.m[si]) {
                hstx_dvi_sprite_disable_1(sprite); // Disable the invader if it was previously enabled
            }
            else {
                sprite->x += inv_v;
                if (inv_v > 0) {
                    if(sprite->x + 16 >= MODE_H_ACTIVE_PIXELS) reverse = true;
                }
                else {
                    if(sprite->x <= 0) reverse = true;
                }                
            }
        }
    }
    if (reverse) inv_v = -inv_v;
}
