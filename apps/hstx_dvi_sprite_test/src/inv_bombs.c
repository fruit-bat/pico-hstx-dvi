#include "inv_bombs.h"
#include "game_pallet.h"
#include "inv_collisions.h"
#include "inv_base.h"
#include "inv_level.h"

#define INV_BOMB_COUNT 10

static SpriteId _sprite_index = 0;

#define INV_BOMB_KINDS 3
#define INV_BOMB_FRAMES 2

// The commanders drop sparks that fall faster, the crabs plungers and the jellies zig zags
static Tile8x8p2_t tile8x8p2_bombs[INV_BOMB_KINDS][INV_BOMB_FRAMES] = {
	{
		{{
			0b00010000,
			0b00111000,
			0b01111100,
			0b00111000,
			0b00010000,
			0b00010000,
			0b00111000,
			0b00010000,
		}},
		{{
			0b00010000,
			0b00010000,
			0b00111000,
			0b00010000,
			0b00010000,
			0b00111000,
			0b01111100,
			0b00111000,
		}},
	},
	{
		{{
			0b00010000,
			0b00010000,
			0b00010000,
			0b01111100,
			0b00010000,
			0b00010000,
			0b00010000,
			0b00111000,
		}},
		{{
			0b00111000,
			0b00010000,
			0b00010000,
			0b00010000,
			0b01111100,
			0b00010000,
			0b00010000,
			0b00010000,
		}},
	},
	{
		{{
			0b00010000,
			0b00100000,
			0b00010000,
			0b00001000,
			0b00010000,
			0b00100000,
			0b00010000,
			0b00010000,
		}},
		{{
			0b00010000,
			0b00001000,
			0b00010000,
			0b00100000,
			0b00010000,
			0b00001000,
			0b00010000,
			0b00010000,
		}},
	},
};

static const uint8_t _bomb_colour[INV_BOMB_KINDS] = { GAME_PINK, GAME_ORANGE, GAME_YELLOW };
static uint8_t _bomb_kind[INV_BOMB_COUNT];

SpriteId inv_bombs_init(SpriteId start) {
    _sprite_index = start;

	for(uint32_t x = 0; x < INV_BOMB_COUNT; ++x) {

	    SpriteId si = _sprite_index + x;

		init_sprite(
			si,
			0,  // x
			0,  // y
			8,  // width
			8,  // height
			0,  // flags
			&tile8x8p2_bombs[0][0],
			game_pallet(GAME_YELLOW),
			sprite_renderer_sprite_8x8_p1);

		hstx_dvi_sprite_set_sprite_collision_mask(si, INV_BOMB_COLLISION_MASK);
	}

    return _sprite_index + INV_BOMB_COUNT;
}

void __not_in_flash_func(inv_bombs_update)() {
	const int32_t speed = inv_level_bomb_speed();
    for (uint32_t i = 0; i < INV_BOMB_COUNT; ++i)
    {
        SpriteId si = _sprite_index + i;
        Sprite *sprite = hstx_dvi_sprite_get(si);
        if (sprite->f & SF_ENABLE) {
			SpriteCollisionMask m = _spriteCollisions.m[si];
			// Bombs fall through the invaders
			if (m & ~(INV_BOMB_COLLISION_MASK | INV_INVADER_COLLISION_MASK)) {
				if (m & INV_BASE_COLLISION_MASKS) {
					inv_base_bomb_hit(si, m); // Notify the base of the bomb hit
				}
				hstx_dvi_sprite_disable_1(sprite); // Disable the bullet if it was previously enabled
			}
			else {
				const uint32_t kind = _bomb_kind[i];
				sprite->y += speed + (kind == 0 ? 1 : 0); // Move the bomb down
				sprite->d1 = &tile8x8p2_bombs[kind][(sprite->y >> 3) & 1]; // Wiggle
				if (sprite->y > MODE_V_ACTIVE_LINES + 8) { // If the bullet goes off screen
					sprite->f &= ~SF_ENABLE; // Disable the bullet
				}
			}
		}
    }
}

void inv_bombs_fire(int32_t x, int32_t y, uint32_t kind) {
	for (uint32_t i = 0; i < INV_BOMB_COUNT; ++i)
	{
		SpriteId si = _sprite_index + i;
		Sprite *sprite = hstx_dvi_sprite_get(si);
		if (!(sprite->f & SF_ENABLE)) {
			_bomb_kind[i] = kind % INV_BOMB_KINDS;
			sprite->x = x;
			sprite->y = y;
			sprite->d1 = &tile8x8p2_bombs[_bomb_kind[i]][0];
			sprite->d2 = game_pallet(_bomb_colour[_bomb_kind[i]]);
			sprite->f |= SF_ENABLE;
			break;
		}
	}
}

void inv_bombs_clear(void) {
	for (uint32_t i = 0; i < INV_BOMB_COUNT; ++i) {
		hstx_dvi_sprite_disable(_sprite_index + i);
	}
}

uint32_t __not_in_flash_func(inv_bombs_active)(void) {
	uint32_t n = 0;
	for (uint32_t i = 0; i < INV_BOMB_COUNT; ++i) {
		if (hstx_dvi_sprite_get(_sprite_index + i)->f & SF_ENABLE) ++n;
	}
	return n;
}

bool __not_in_flash_func(inv_bombs_threat)(int32_t x0, int32_t x1, int32_t y0, int32_t* bx) {
	int32_t lowest = INT32_MIN;
	for (uint32_t i = 0; i < INV_BOMB_COUNT; ++i) {
		const Sprite *sprite = hstx_dvi_sprite_get(_sprite_index + i);
		if (!(sprite->f & SF_ENABLE)) continue;
		const int32_t x = sprite->x + 3; // The middle of the bomb
		if (x < x0 || x > x1 || sprite->y < y0 || sprite->y <= lowest) continue;
		lowest = sprite->y;
		*bx = x;
	}
	return lowest != INT32_MIN;
}
