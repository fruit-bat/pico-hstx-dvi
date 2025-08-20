#include "inv_invaders.h"
#include "inv_score.h"
#include "inv_pallet.h"
#include "inv_collisions.h"
#include "inv_bombs.h"

#define INV_INVADER_COLS 22
#define INV_INVADER_ROWS 10
#define INV_INVADER_COUNT (INV_INVADER_COLS * INV_INVADER_ROWS)

typedef enum  {
	INV_STATE_WALK = 0,
	INV_STATE_EXPLODE,
	INV_STATE_DEAD
 } InvInvaderState_e;

typedef struct {
	InvInvaderState_e state;
	uint32_t end;
} InvInvaderState_t;

static SpriteId _inv_index = 0;
static int32_t inv_v = 1;
static InvInvaderState_t _inv_state[INV_INVADER_COUNT];

const static uint8_t _inv_row_score[INV_INVADER_ROWS] = {
    20,20,10,10,10,10,5,5,5,5
};

__force_inline static const uint32_t get_score_for_sprite(const uint32_t index) {
    const uint32_t row = index / INV_INVADER_COLS;
    return _inv_row_score[row];
}

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

static Tile16x8p2_t tile16x8p2_invader_explosion[] = {
	{{
		0b0000010001000000,
		0b0010001010001000,
		0b0001000000010000,
		0b0000100000100000,
		0b0110000000001100,
		0b0000100000100000,
		0b0001001010010000,
		0b0010010001001000,
	}},
};

void inv_invader_explode(const uint32_t index, const uint32_t frame) {
	const SpriteId spriteId = _inv_index + index;
	Sprite *sprite = hstx_dvi_sprite_get(spriteId);
	if (sprite->f & SF_ENABLE) {
		// Set the sprite to explode
		sprite->r = sprite_renderer_sprite_16x8_p1;
		sprite->d1 = &tile16x8p2_invader_explosion[0];
		sprite->d2 = inv_pallet_white();
		_inv_state[index].state = INV_STATE_EXPLODE;
		_inv_state[index].end = frame + 20; 
	}
}

SpriteId inv_invaders_init(SpriteId start) {
    _inv_index = start;

	uint32_t rt[5] = {0, 2, 2, 4, 4};
	hstx_dvi_pixel_t* rp[5] = {
        inv_pallet_white(),
        inv_pallet_blue(),
        inv_pallet_blue(),
        inv_pallet_purple(),
        inv_pallet_purple()
    };

	uint32_t i = 0;
	for(uint32_t y = 0; y < INV_INVADER_ROWS; ++y) {
        for(uint32_t x = 0; x < INV_INVADER_COLS; ++x, ++i) {
		    const SpriteId si = _inv_index + i;
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

			// Initialize the invader states
			_inv_state[i].state = INV_STATE_WALK;
			_inv_state[i].end = 0;
		}
	}

	// Set the invaders to move right
	inv_v = 1;

    return start + INV_INVADER_COUNT;
}

void __not_in_flash_func(inv_invader_update)(uint32_t frame) {

    int32_t inv_lowest[INV_INVADER_COLS];
    for (uint32_t i = 0; i < INV_INVADER_COLS; ++i) {
        inv_lowest[i] = -1;
    }

    bool reverse = false;
	uint32_t i = 0;
	for(uint32_t y = 0; y < INV_INVADER_ROWS; ++y) {
        for(uint32_t x = 0; x < INV_INVADER_COLS; ++x, ++i) {
			InvInvaderState_t* state = &_inv_state[i];
		    const SpriteId si = _inv_index + i;
			Sprite *sprite = hstx_dvi_sprite_get(si);
			switch (state->state) {
				case INV_STATE_WALK: {
					sprite->x += inv_v;
					if (sprite->f & SF_ENABLE) {
						if (_spriteCollisions.m[si]) {
							inv_invader_explode(i, frame);
						}
						else {
							if (inv_v > 0) {
								if(sprite->x + 16 >= MODE_H_ACTIVE_PIXELS) reverse = true;
							}
							else {
								if(sprite->x <= 0) reverse = true;
							}

							if ((inv_lowest[x] == -1) || (hstx_dvi_sprite_get(inv_lowest[x])->y < sprite->y)) {
								inv_lowest[x] = si;
							}
						}
					}
					break;
				}
				case INV_STATE_EXPLODE: {
					sprite->x += inv_v;
					if ((int32_t)(state->end - frame) < 0) {
						state->state = INV_STATE_DEAD;
						hstx_dvi_sprite_disable_1(hstx_dvi_sprite_get(si));
						inv_score_add(get_score_for_sprite(si));
					}
					break;
				}
				case INV_STATE_DEAD: {
					// Do nothing, the sprite is already disabled
					break;
				}
				default: {
					break;
				}
			}
        }
    }
    if (reverse) inv_v = -inv_v;

	bool any_alive = false;
    for (uint32_t i = 0; i < INV_INVADER_COLS; ++i) {
        int32_t id = inv_lowest[i];
        if (id != -1)  {
			any_alive = true;
            inv_bombs_fire((SpriteId)id);
        }
    }

	if (!any_alive) {
		// Next wave
		inv_invaders_init(_inv_index);
	}
}
