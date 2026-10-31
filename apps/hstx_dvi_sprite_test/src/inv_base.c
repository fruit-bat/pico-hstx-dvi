#include "inv_base.h"
#include "game_pallet.h"
#include <memory.h>
#include "inv_collisions.h"
#include <stdio.h>
// Each base is made of two sprites side by side, so it can be wider than a tile
#define INV_BASES 4
#define INV_BASE_COUNT (INV_BASES * 2)
#define INV_BASE_W 40
#define INV_BASE_H 16
#define INV_BASE_Y (MODE_V_ACTIVE_LINES - 88)

static SpriteId _sprite_index = 0;

static Tile32x16p2_t tile32x16p2_bases[INV_BASE_COUNT];

// Whether column c of row r of a base is solid: rounded at the top, with an
// arch cut out at the bottom
static bool base_solid(const int32_t c, const int32_t r) {
	if (c < 0 || c >= INV_BASE_W) return false;
	if (r < 4 && (c < 4 - r || c >= INV_BASE_W - 4 + r)) return false;
	const int32_t mid2 = 2 * c - (INV_BASE_W - 1);
	if (r >= 10 && mid2 > -2 * (6 + r - 10) && mid2 < 2 * (6 + r - 10)) return false;
	return true;
}

SpriteId inv_base_init(SpriteId start) {
    _sprite_index = start;

	// The base sits in the middle of the 64 pixels the two sprites cover
	const int32_t inset = (64 - INV_BASE_W) / 2;
	for(uint i = 0; i < INV_BASE_COUNT; ++i){

		SpriteId si = _sprite_index + i;
		const uint32_t half = i & 1;

		for (int32_t r = 0; r < INV_BASE_H; ++r) {
			uint32_t bits = 0;
			for (int32_t c = 0; c < 32; ++c) {
				if (base_solid((int32_t)half * 32 + c - inset, r)) bits |= 0x80000000u >> c;
			}
			tile32x16p2_bases[i].d[r] = bits;
		}

		const uint32_t w1 = MODE_H_ACTIVE_PIXELS / INV_BASES;
		const uint32_t p = w1 * (i >> 1) + ((w1 - 64) / 2) + half * 32;
		init_sprite(
			si,
			p,
			INV_BASE_Y,
			32,
			INV_BASE_H,
			SF_ENABLE,
			&tile32x16p2_bases[i],
			game_pallet(GAME_GREEN),
			sprite_renderer_sprite_32x16_p1);

			hstx_dvi_sprite_set_sprite_collision_mask(si, INV_BASE_COLLISION_MASK << i);
	}

    return _sprite_index + INV_BASE_COUNT;
}

void __not_in_flash_func(inv_base_update)() {
}

static Tile8x8p2_t tile8x8p2_damage[] = {
	{{
		0b01011100,
		0b00011010,
		0b00001100,
		0b00011000,
		0b00001100,
		0b00000000,
		0b00000000,
		0b00000000,
	}}
};

// Position a w bit row (bit w-1 = left) at column dx of a 32 bit base row (bit 31 = left)
static __force_inline uint32_t row_at(const uint32_t d, const int32_t dx, const int32_t w) {
	const int32_t s = 32 - w - dx;
	if (s >= 32 || s <= -w) return 0;
	return s >= 0 ? d << s : d >> -s;
}

// Position an 8 bit damage row (bit 7 = left) at column dx of a 32 bit base row (bit 31 = left)
static __force_inline uint32_t damage_row_at(const uint32_t d, const int32_t dx) {
	return row_at(d, dx, 8);
}

void __not_in_flash_func(inv_base_bomb_hit)(SpriteId spriteId, SpriteCollisionMask m) {
	SpriteCollisionMask t = INV_BASE_COLLISION_MASK;
	for(uint i = 0; i < INV_BASE_COUNT; ++i) {
		if (m & t) {
			SpriteId si = _sprite_index + i;
			uint32_t* base_image = (uint32_t*)(&tile32x16p2_bases[i]);
			Tile8x8p2_t *d = &tile8x8p2_damage[0];
			Sprite *bomb_sprite = hstx_dvi_sprite_get(spriteId);
			int32_t bomb_x = bomb_sprite->x;
			int32_t bomb_y = bomb_sprite->y;
			int32_t base_x = hstx_dvi_sprite_get(si)->x;
			int32_t base_y = hstx_dvi_sprite_get(si)->y;
			int32_t dx = bomb_x - base_x;
			int32_t dy = bomb_y - base_y;
			for (uint32_t r = 0; r < 8; ++r)
			{
				uint32_t d1 = d->d[r];
				int32_t y = dy + r;
				if ((y < 0) || (y >= 16)) continue; // Skip
				base_image[y] &= ~damage_row_at(d1, dx);
			}
		}
		t <<= 1;
	}
}

void __not_in_flash_func(inv_base_bullet_hit)(SpriteId spriteId, SpriteCollisionMask m) {
	SpriteCollisionMask t = INV_BASE_COLLISION_MASK;
	for(uint i = 0; i < INV_BASE_COUNT; ++i) {
		if (m & t) {
			SpriteId si = _sprite_index + i;
			uint32_t* base_image = (uint32_t*)(&tile32x16p2_bases[i]);
			Tile8x8p2_t *d = &tile8x8p2_damage[0];
			Sprite *bullet_sprite = hstx_dvi_sprite_get(spriteId);
			int32_t bullet_x = bullet_sprite->x;
			int32_t bullet_y = bullet_sprite->y;
			int32_t base_x = hstx_dvi_sprite_get(si)->x;
			int32_t base_y = hstx_dvi_sprite_get(si)->y;
			int32_t dx = bullet_x - base_x - 4;
			int32_t dy = bullet_y - base_y;
			for (uint32_t r = 0; r < 8; ++r)
			{
				uint32_t d1 = d->d[7-r];
				int32_t y = dy + r;
				if ((y < 0) || (y >= 16)) continue; // Skip
				base_image[y] &= ~damage_row_at(d1, dx);
			}
		}
		t <<= 1;
	}
}

// An invader walking through a base wipes out the part of the base it covers
void __not_in_flash_func(inv_base_invader_hit)(int32_t x, int32_t y, int32_t w, int32_t h, SpriteCollisionMask m) {
	SpriteCollisionMask t = INV_BASE_COLLISION_MASK;
	const uint32_t wide = w < 32 ? (1u << w) - 1 : 0xffffffffu;
	for(uint i = 0; i < INV_BASE_COUNT; ++i) {
		if (m & t) {
			SpriteId si = _sprite_index + i;
			uint32_t* base_image = (uint32_t*)(&tile32x16p2_bases[i]);
			int32_t dx = x - hstx_dvi_sprite_get(si)->x;
			int32_t dy = y - hstx_dvi_sprite_get(si)->y;
			const uint32_t clear = ~row_at(wide, dx, w);
			for (int32_t r = 0; r < h; ++r)
			{
				int32_t y = dy + r;
				if ((y < 0) || (y >= 16)) continue; // Skip
				base_image[y] &= clear;
			}
		}
		t <<= 1;
	}
}
