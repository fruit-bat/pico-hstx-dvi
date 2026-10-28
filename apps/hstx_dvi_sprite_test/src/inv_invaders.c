#include "inv_invaders.h"
#include "inv_score.h"
#include "inv_pallet.h"
#include "inv_collisions.h"
#include "inv_bombs.h"
#include "inv_base.h"
#include "inv_gun.h"
#include "inv_level.h"
#include "inv_util.h"
#include "font_inv.h"
#include <string.h>

#define INV_INVADER_COLS 22
#define INV_INVADER_ROWS 10
#define INV_INVADER_COUNT (INV_INVADER_COLS * INV_INVADER_ROWS)
// How far the invaders step down each time they reach the side of the screen
#define INV_INVADER_DROP 8
// Frames before the first bomb of a wave
#define INV_INVADER_FIRE_DELAY 90
#define INV_TITLE_MAX 160
// Frames for the title invaders to fly in to place
#define INV_TITLE_FLY 90

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
// Lowest walking invader in each column, or -1
static int32_t _inv_lowest[INV_INVADER_COLS];
// Sub pixel march position, in 1/16 pixel
static uint32_t _inv_sub = 0;
// March speed last frame, in 1/16 pixel per frame
static uint32_t _inv_speed = 0;
static uint32_t _inv_alive = 0;
static uint32_t _inv_exploding = 0;
static bool _inv_landed = false;
static uint32_t _fire_timer = 0;

// Where each title invader rests, and the direction it flies in from
static uint32_t _title_count = 0;
static int16_t _title_x[INV_TITLE_MAX];
static int16_t _title_y[INV_TITLE_MAX];
static uint8_t _title_a[INV_TITLE_MAX];

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

static void inv_invader_explode(const uint32_t index, const uint32_t frame) {
	const SpriteId spriteId = _inv_index + index;
	Sprite *sprite = hstx_dvi_sprite_get(spriteId);
	if (sprite->f & SF_ENABLE) {
		// Set the sprite to explode
		sprite->r = sprite_renderer_sprite_16x8_p1;
		sprite->d1 = &tile16x8p2_invader_explosion[0];
		sprite->d2 = inv_pallet_white();
		_inv_state[index].state = INV_STATE_EXPLODE;
		_inv_state[index].end = frame + 10; 
	}
}

const Tile16x8p2_t* inv_invaders_tile(uint32_t type, uint32_t anim) {
	// The mystery ship only has the one frame
	if (type >= INV_INVADER_TYPE_MOT) return &tile16x8p2_invaders[6];
	return &tile16x8p2_invaders[(type << 1) + (anim & 1)];
}

hstx_dvi_pixel_t* inv_invaders_pallet(uint32_t type) {
	switch(type) {
		case 0: return inv_pallet_white();
		case 1: return inv_pallet_blue();
		case 2: return inv_pallet_purple();
		default: return inv_pallet_red();
	}
}

uint32_t inv_invaders_type_score(uint32_t type) {
	switch(type) {
		case 0: return 20;
		case 1: return 10;
		default: return 5;
	}
}

SpriteId inv_invaders_init_title(SpriteId start) {
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
					_title_a[i] = inv_rand();
					// Create the invader sprite off screen, ready to fly in
					init_sprite(
						si,
						-32,
						-32,
						16,
						8,
						SF_ENABLE,
						&tile16x8p2_invaders[(y >> 1) & 2],
						inv_pallet_rainbow(i),
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
void __not_in_flash_func(inv_invaders_update_title)(uint32_t frame, uint32_t end) {
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
		const int32_t fx = cx + inv_sin(_title_a[i] + 64) * radius / 127;
		const int32_t fy = cy + inv_sin(_title_a[i]) * radius / 127;
		const int32_t wave = inv_sin((hx >> 1) - (frame << 2)) * 8 / 127;
		sprite->x = fx + (hx - fx) * e / ff;
		sprite->y = fy + (hy + wave - fy) * e / ff;
		sprite->d2 = inv_pallet_rainbow((hx >> 3) + (hy >> 4) - (frame >> 2));
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

	const int32_t top = inv_level_invader_top();
	uint32_t i = 0;
	for(uint32_t y = 0; y < INV_INVADER_ROWS; ++y) {
        for(uint32_t x = 0; x < INV_INVADER_COLS; ++x, ++i) {
		    const SpriteId si = _inv_index + i;
            // Create the invader sprite
			init_sprite(
                si,
                x << 4,
                top + (y << 4),
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
	for (uint32_t x = 0; x < INV_INVADER_COLS; ++x) {
		_inv_lowest[x] = -1;
	}

	// Set the invaders to move right
	inv_v = 1;
	_inv_sub = 0;
	_inv_speed = 0;
	_inv_alive = INV_INVADER_COUNT;
	_inv_exploding = 0;
	_inv_landed = false;
	_fire_timer = INV_INVADER_FIRE_DELAY;

    return start + INV_INVADER_COUNT;
}

// The march speeds up as the invaders are thinned out, and the last one really goes for it
static uint32_t __not_in_flash_func(march_speed)(void) {
	uint32_t s = inv_level_invader_speed();
	s += ((INV_INVADER_COUNT - _inv_alive) * 24) / INV_INVADER_COUNT;
	if (_inv_alive <= 10) s += 8;
	if (_inv_alive <= 1) s += 24;
	return s;
}

// Picks a column to drop a bomb from, either the one nearest the gun or one at random
static int32_t __not_in_flash_func(pick_bomb_column)(void) {
	if ((inv_rand() & 255) < inv_level_aim_chance()) {
		const int32_t gx = inv_gun_get_x();
		int32_t best = -1;
		int32_t best_d = INT32_MAX;
		for (uint32_t x = 0; x < INV_INVADER_COLS; ++x) {
			if (_inv_lowest[x] < 0) continue;
			int32_t d = hstx_dvi_sprite_get(_inv_index + _inv_lowest[x])->x - gx;
			if (d < 0) d = -d;
			if (d < best_d) {
				best_d = d;
				best = x;
			}
		}
		return best;
	}
	const uint32_t first = inv_rand() % INV_INVADER_COLS;
	for (uint32_t i = 0; i < INV_INVADER_COLS; ++i) {
		uint32_t x = first + i;
		if (x >= INV_INVADER_COLS) x -= INV_INVADER_COLS;
		if (_inv_lowest[x] >= 0) return x;
	}
	return -1;
}

void __not_in_flash_func(inv_invader_update)(uint32_t frame) {

	// Everything holds still while the gun is being replaced
	const bool frozen = _inv_landed || !inv_gun_is_alive();
	int32_t dx = 0;
	if (!frozen) {
		_inv_speed = march_speed();
		_inv_sub += _inv_speed;
		dx = (int32_t)(_inv_sub >> 4) * inv_v;
		_inv_sub &= 15;
	}

    for (uint32_t i = 0; i < INV_INVADER_COLS; ++i) {
        _inv_lowest[i] = -1;
    }

    bool reverse = false;
	uint32_t alive = 0;
	uint32_t exploding = 0;
	int32_t bottom = 0;
	uint32_t i = 0;
	for(uint32_t y = 0; y < INV_INVADER_ROWS; ++y) {
        for(uint32_t x = 0; x < INV_INVADER_COLS; ++x, ++i) {
			InvInvaderState_t* state = &_inv_state[i];
		    const SpriteId si = _inv_index + i;
			Sprite *sprite = hstx_dvi_sprite_get(si);
			switch (state->state) {
				case INV_STATE_WALK: {
					sprite->x += dx;
					const SpriteCollisionMask m = _spriteCollisions.m[si];
					if (m & INV_BULLET_COLLISION_MASK) {
						inv_invader_explode(i, frame);
						++exploding;
					}
					else {
						if (m & INV_BASE_COLLISION_MASKS) {
							inv_base_invader_hit(si, m);
						}
						++alive;
						if (dx > 0) {
							if(sprite->x + 16 >= MODE_H_ACTIVE_PIXELS) reverse = true;
						}
						else if (dx < 0) {
							if(sprite->x <= 0) reverse = true;
						}
						// Rows are visited top to bottom, so the last one seen is the lowest
						_inv_lowest[x] = i;
						if (sprite->y + 8 > bottom) bottom = sprite->y + 8;
					}
					break;
				}
				case INV_STATE_EXPLODE: {
					sprite->x += dx;
					if ((int32_t)(state->end - frame) < 0) {
						state->state = INV_STATE_DEAD;
						hstx_dvi_sprite_disable_1(hstx_dvi_sprite_get(si));
						inv_score_add(get_score_for_sprite(i));
					}
					else {
						++exploding;
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
    if (reverse) {
		inv_v = -inv_v;
		for (uint32_t j = 0; j < INV_INVADER_COUNT; ++j) {
			hstx_dvi_sprite_get(_inv_index + j)->y += INV_INVADER_DROP;
		}
		bottom += INV_INVADER_DROP;
	}
	_inv_alive = alive;
	_inv_exploding = exploding;

	if (alive && bottom >= inv_gun_get_y()) {
		_inv_landed = true;
	}

	if (frozen || !alive) return;

	if (_fire_timer) {
		--_fire_timer;
	}
	else if (inv_bombs_active() < inv_level_bomb_max()) {
		const int32_t x = pick_bomb_column();
		if (x >= 0) inv_bombs_fire(_inv_index + _inv_lowest[x]);
		const uint32_t interval = inv_level_bomb_interval();
		_fire_timer = (interval >> 1) + (inv_rand() % interval);
	}
}

uint32_t __not_in_flash_func(inv_invaders_alive)(void) {
	return _inv_alive;
}

bool inv_invaders_cleared(void) {
	return !_inv_alive && !_inv_exploding;
}

bool inv_invaders_landed(void) {
	return _inv_landed;
}

bool __not_in_flash_func(inv_invaders_aim)(const int32_t gx, int32_t* tx) {
	const Sprite *best = NULL;
	int32_t best_d = INT32_MAX;
	for (uint32_t x = 0; x < INV_INVADER_COLS; ++x) {
		if (_inv_lowest[x] < 0) continue;
		const Sprite *sprite = hstx_dvi_sprite_get(_inv_index + _inv_lowest[x]);
		int32_t d = sprite->x + 8 - gx;
		if (d < 0) d = -d;
		if (d < best_d) {
			best_d = d;
			best = sprite;
		}
	}
	if (!best) return false;
	// Lead the target by how far it will march while a bullet climbs up to it
	const int32_t frames = (inv_gun_get_y() - best->y) >> 2;
	*tx = best->x + 8 + ((frames * (int32_t)_inv_speed * inv_v) >> 4);
	return true;
}
