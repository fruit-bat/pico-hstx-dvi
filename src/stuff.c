#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/clocks.h"
#include "hardware/irq.h"
#include "hardware/sync.h"
#include "hardware/gpio.h"
#include "hardware/vreg.h"
#include "hardware/structs/bus_ctrl.h"
#include "hardware/structs/ssi.h"
#include "hardware/dma.h"
#include "pico/sem.h"

#include "font_inv.h"

#define FONT_CHAR_WIDTH 8
#define FONT_CHAR_HEIGHT 8
#define FONT_N_CHARS 95
#define FONT_FIRST_ASCII 32

#define MODE_640x480_60Hz


typedef struct {
	uint8_t r[1];
	uint8_t g[1];
	uint8_t b[1];
} Pallet1_t;

typedef struct {
	uint8_t r[2];
	uint8_t g[2];
	uint8_t b[2];
} Pallet2_t;

typedef struct {
	uint16_t d[16];
} Tile16x16p2_t;

typedef struct {
	uint16_t d[8];
} Tile16x8p2_t;

typedef struct {
	uint32_t d[16];
} Tile32x16p2_t;

typedef struct {
	uint16_t w;
	uint8_t *s;
} TextGrid8_t;

typedef uint8_t SpriteId;
typedef uint8_t SpriteCollisionMask;

typedef void (*SpriteRenderer)(
	const void* d1,
	const void* d2,
	uint32_t * const dr,
	uint32_t * const dg,
	uint32_t * const db,
	const int32_t tdmsI,
	const int32_t row,
	const SpriteId spriteId
);

enum SpriteFlags {
  SF_ENABLE = 1
};

typedef struct Sprite {
	int32_t x,y;
	uint32_t w,h,f;
	void *d1, *d2;
	SpriteRenderer r;
} Sprite;

#define MAX_SPRITES (1<<6)

Sprite _sprites[MAX_SPRITES];

void init_sprites() {
	for(uint32_t i = 0; i < MAX_SPRITES; ++i) _sprites[i].f = 0;
}

void init_sprite(
	int i,
	int32_t x,
	int32_t y,
	uint32_t w,
	uint32_t h,
	uint32_t f,
	void *d1, 
	void *d2,
	SpriteRenderer r
) {
	Sprite *s = &_sprites[i];
	s->x = x;
	s->y = y;
	s->w = w;
	s->h = h;
	s->f = f;
	s->d1 = d1;
	s->d2 = d2;
	s->r = r;
}

// ----------------------------------------------------------------------------
// Sprite collisions
// 
// Collide with the drawn sprite only 
// ----------------------------------------------------------------------------
#define SPRITE_ID_ROW_WORDS ((FRAME_WIDTH + 3) >> 2)

typedef union {
	SpriteId id[FRAME_WIDTH];
	uint32_t word[SPRITE_ID_ROW_WORDS];
} SpriteIdRow;

typedef union {
	SpriteCollisionMask m[MAX_SPRITES];
	uint32_t word[MAX_SPRITES >> 2];
} SpriteCollisions;

static SpriteCollisionMask _spriteCollisionMasks[MAX_SPRITES];
static SpriteIdRow _spriteIdRow; 
static SpriteCollisions _spriteCollisions;

void __not_in_flash_func(clear_sprite_id_row)() {
	for(uint32_t i = 0; i < SPRITE_ID_ROW_WORDS; ++i) _spriteIdRow.word[i] = 0;
}

void __not_in_flash_func(clear_sprite_collisions)() {
	for(uint32_t i = 0; i < MAX_SPRITES >> 2; ++i) _spriteCollisions.word[i] = 0;
}

// ----------------------------------------------------------------------------

void __not_in_flash_func(render_row_mono)(
	uint32_t *dr,
	uint32_t *dg,
	uint32_t *db,
	uint32_t bgr,
	uint32_t bgg,
	uint32_t bgb
) {
	for(int32_t i = 0; i < FRAME_WIDTH; i++) {
		dr[i] = tmds_table[bgr];
		dg[i] = tmds_table[bgg];
		db[i] = tmds_table[bgb];
	}
}

static inline void render_sprite_pixel(
	uint32_t *const dr,
	uint32_t *const dg,
	uint32_t *const db,
	const uint32_t r,
	const uint32_t g,
	const uint32_t b,
	const SpriteId spriteId,
	const uint32_t j)
{
	const SpriteId ncid = _spriteIdRow.id[j];
	if (ncid)
	{
		const SpriteId cid = ncid - 1;
		_spriteCollisions.m[cid] |= _spriteCollisionMasks[spriteId];
		_spriteCollisions.m[spriteId] |= _spriteCollisionMasks[cid];
	}
	else
	{
		dr[j] = r;
		dg[j] = g;
		db[j] = b;
		_spriteIdRow.id[j] = spriteId + 1;
	}
}

static inline void render_pixel(
	uint32_t * const dr,
	uint32_t * const dg,
	uint32_t * const db,	
	const uint32_t r,
	const uint32_t g,
	const uint32_t b,
	const uint32_t j
) {
	dr[j] = r;
	dg[j] = g;
	db[j] = b;
}

static inline void render_sprite_row_n_p1(
	uint32_t d,
	const Pallet1_t * const p,
	uint32_t * const dr,
	uint32_t * const dg,
	uint32_t * const db,
	const int32_t tdmsI,
	const SpriteId spriteId,
	const uint32_t w
) {
	if (d)
	{
		const uint32_t fgr = tmds_table[p->r[0]];
		const uint32_t fgg = tmds_table[p->g[0]];
		const uint32_t fgb = tmds_table[p->b[0]];
		const uint32_t bm = 1 << (w-1);
		if (((uint32_t)tdmsI) < (FRAME_WIDTH - w))
		{
			for (int32_t i = 0; i < w; i++)
			{
				const uint32_t j = (uint32_t)tdmsI + i;
				if (d & bm)
				{
					render_sprite_pixel(dr, dg, db, fgr, fgg, fgb, spriteId, j);
				}
				d <<= 1;
			}
		}
		else
		{
			for (int32_t i = 0; i < w; i++)
			{
				const uint32_t j = (uint32_t)tdmsI + i;
				if ((j < FRAME_WIDTH) && (d & bm))
				{
					render_sprite_pixel(dr, dg, db, fgr, fgg, fgb, spriteId, j);
				}
				d <<= 1;
			}
		}
	}
}

static inline void render_row_n_p1(
	uint32_t d,
	const Pallet1_t * const p,
	uint32_t * const dr,
	uint32_t * const dg,
	uint32_t * const db,
	const int32_t tdmsI,
	const uint32_t w
) {
	if (d)
	{
		const uint32_t fgr = tmds_table[p->r[0]];
		const uint32_t fgg = tmds_table[p->g[0]];
		const uint32_t fgb = tmds_table[p->b[0]];
		const uint32_t bm = 1 << (w-1);
		if (((uint32_t)tdmsI) < (FRAME_WIDTH - w))
		{
			for (int32_t i = 0; i < w; i++)
			{
				const uint32_t j = (uint32_t)tdmsI + i;
				if (d & bm)
				{
					render_pixel(dr, dg, db, fgr, fgg, fgb, j);
				}
				d <<= 1;
			}
		}
		else
		{
			for (int32_t i = 0; i < w; i++)
			{
				const uint32_t j = (uint32_t)tdmsI + i;
				if ((j < FRAME_WIDTH) && (d & bm))
				{
					render_pixel(dr, dg, db, fgr, fgg, fgb, j);
				}
				d <<= 1;
			}
		}
	}
}

static inline void render_row_text_8_p1(
	const TextGrid8_t *tg,
	const Pallet1_t * const p,
	uint32_t * const dr,
	uint32_t * const dg,
	uint32_t * const db,
	const int32_t tdmsI,
	const int32_t row
) {
	uint8_t * const s = tg->s + __mul_instruction((row >> 3), tg->w);
	const uint32_t r = row & 7;
	const uint32_t w = tg->w;
	const uint32_t v = w >> 2;
	for(uint32_t i = 0; i < v; ++i) {
		const uint32_t q = i << 2;
		const uint8_t d1 = font_8x8[r + ((s[q] -  32) << 3)];
		const uint8_t d2 = font_8x8[r + ((s[q+1] -  32) << 3)];
		const uint8_t d3 = font_8x8[r + ((s[q+2] -  32) << 3)];
		const uint8_t d4 = font_8x8[r + ((s[q+3] -  32) << 3)];
		const uint32_t g = (((uint32_t)d1) << 24) | (((uint32_t)d2) << 16) | (((uint32_t)d3) << 8) | d4;
		render_row_n_p1(
			g,
			p,
			dr,
			dg,
			db,
			tdmsI + (i << 5),
			32
		);
	}
	for(uint32_t i = w & -4; i < w; ++i) {
		const uint8_t d = font_8x8[r + ((s[i] -  32) << 3)];
		render_row_n_p1(
			d,
			p,
			dr,
			dg,
			db,
			tdmsI + (i << 3),
			8
		);
	}
}

static inline void render_Tile16x16p1(
	const Tile16x16p2_t * const t,
	const Pallet1_t * const p,
	uint32_t * const dr,
	uint32_t * const dg,
	uint32_t * const db,
	const int32_t tdmsI,
	const int32_t row,
	const SpriteId spriteId
) {
	uint32_t d = t->d[row];
	render_sprite_row_n_p1(
		d,
		p,
		dr,
		dg,
		db,
		tdmsI,
		spriteId,
		16
	);
}

static inline void render_Tile16x8p1(
	const Tile16x8p2_t * const t,
	const Pallet1_t * const p,
	uint32_t * const dr,
	uint32_t * const dg,
	uint32_t * const db,
	const int32_t tdmsI,
	const int32_t row,
	const SpriteId spriteId
) {
	uint32_t d = t->d[row];
	render_sprite_row_n_p1(
		d,
		p,
		dr,
		dg,
		db,
		tdmsI,
		spriteId,
		16
	);
}

static inline void render_Tile32x16p1(
	const Tile32x16p2_t * const t,
	const Pallet1_t * const p,
	uint32_t * const dr,
	uint32_t * const dg,
	uint32_t * const db,
	const int32_t tdmsI,
	const int32_t row,
	const SpriteId spriteId
) {
	uint32_t d = t->d[row];
	render_sprite_row_n_p1(
		d,
		p,
		dr,
		dg,
		db,
		tdmsI,
		spriteId,
		32
	);
}

void __not_in_flash_func(sprite_renderer_sprite_16x8_p1)(
	const void* d1,
	const void* d2,
	uint32_t * const dr,
	uint32_t * const dg,
	uint32_t * const db,
	const int32_t tdmsI,
	const int32_t row,
	const SpriteId spriteId
) {
	render_Tile16x8p1(
		d1,
		d2,
		dr,
		dg,
		db,
		tdmsI,
		row,
		spriteId
	);
}

void __not_in_flash_func(sprite_renderer_sprite_16x16_p1)(
	const void* d1,
	const void* d2,
	uint32_t * const dr,
	uint32_t * const dg,
	uint32_t * const db,
	const int32_t tdmsI,
	const int32_t row,
	const SpriteId spriteId
) {
	render_Tile16x16p1(
		d1,
		d2,
		dr,
		dg,
		db,
		tdmsI,
		row,
		spriteId
	);
}

void __not_in_flash_func(sprite_renderer_sprite_32x16_p1)(
	const void* d1,
	const void* d2,
	uint32_t * const dr,
	uint32_t * const dg,
	uint32_t * const db,
	const int32_t tdmsI,
	const int32_t row,
	const SpriteId spriteId
) {
	render_Tile32x16p1(
		d1,
		d2,
		dr,
		dg,
		db,
		tdmsI,
		row,
		spriteId
	);
}

void __not_in_flash_func(text_renderer_8x8_p1)(
	const void* d1,
	const void* d2,
	uint32_t * const dr,
	uint32_t * const dg,
	uint32_t * const db,
	const int32_t tdmsI,
	const int32_t row,
	const SpriteId spriteId
) {
	render_row_text_8_p1(
		(TextGrid8_t *)d1,
		(Pallet1_t *)d2,
		dr,
		dg,
		db,
		tdmsI,
		row
	);
}

void __not_in_flash_func(render_Tile16x16p2)(
	Tile16x16p2_t *t,
	Pallet2_t *p,
	uint32_t *dr,
	uint32_t *dg,
	uint32_t *db,
	int32_t tdmsI,
	int32_t row
) {
	uint16_t d = t->d[row];
	uint32_t bgr = p->r[0];
	uint32_t fgr = p->r[1];
	uint32_t bgg = p->g[0];
	uint32_t fgg = p->g[1];
	uint32_t bgb = p->b[0];
	uint32_t fgb = p->b[1];
	for(int32_t i = 0; i < 16; i++) {
		int32_t j = tdmsI + i;
		if (d & (1<<15)) {
			dr[j] = tmds_table[fgr];
			dg[j] = tmds_table[fgg];
			db[j] = tmds_table[fgb];
		}
		else {
			dr[j] = tmds_table[bgr];
			dg[j] = tmds_table[bgg];
			db[j] = tmds_table[bgb];
		}
		d <<= 1;
	}
}

Pallet2_t pallet2_BlackGreen = {
	{ (uint8_t)0, (uint8_t)0 },
	{ (uint8_t)0, (uint8_t)63},
	{ (uint8_t)0, (uint8_t)0 }
};

Pallet1_t pallet1_Green = {
	{ (uint8_t)0 },
	{ (uint8_t)63},
	{ (uint8_t)0 }
};

Pallet1_t pallet1_Red = {
	{ (uint8_t)63 },
	{ (uint8_t)0 },
	{ (uint8_t)0 }
};

Pallet1_t pallet1_Blue = {
	{ (uint8_t)10 },
	{ (uint8_t)10 },
	{ (uint8_t)63 }
};

Pallet1_t pallet1_Purple = {
	{ (uint8_t)42 },
	{ (uint8_t)0 },
	{ (uint8_t)42 }
};

Pallet1_t pallet1_White = {
	{ (uint8_t)42 },
	{ (uint8_t)42 },
	{ (uint8_t)42 }
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

void __not_in_flash_func(sprite_renderer_invader_16x8_p1)(
	const void* d1,
	const void* d2,
	uint32_t * const dr,
	uint32_t * const dg,
	uint32_t * const db,
	const int32_t tdmsI,
	const int32_t row,
	const SpriteId spriteId
) {
	const Tile16x8p2_t *tile16x8p2_invader = (Tile16x8p2_t *)d1;
	render_Tile16x8p1(
		&tile16x8p2_invader[(tdmsI >> 2) & 1],
		d2,
		dr,
		dg,
		db,
		tdmsI,
		row,
		spriteId
	);
}

static uint8_t _text1 [40*24] = "In Xanadu did Kubla Khan \
A stately pleasure-dome decree: \
Where Alph, the sacred river, ran \
Through caverns measureless to man \
   Down to a sunless sea. \
So twice five miles of fertile ground \
With walls and towers were girdled round; \
And there were gardens bright with sinuous rills, \
Where blossomed many an incense-bearing tree; \
And here were forests ancient as the hills, \
Enfolding sunny spots of greenery."; 
static TextGrid8_t _textGrid1 = {
	40, _text1
};

static uint32_t inv_index;
static uint32_t mot_index;
static uint32_t gun_index;

static int32_t inv_v = 1;
void init_game() {
	_spriteCollisionMasks[0] = (SpriteCollisionMask)1;
	_spriteCollisionMasks[1] = (SpriteCollisionMask)2;
	_spriteCollisionMasks[2] = (SpriteCollisionMask)8;

	uint32_t si = 0;	
	init_sprite(si++, 50, 15, 16, 8, SF_ENABLE, &tile16x8p2_invader, &pallet1_Green, sprite_renderer_invader_16x8_p1);
	init_sprite(si++, 66, 19, 16, 8, SF_ENABLE, &tile16x8p2_invader, &pallet1_Green, sprite_renderer_invader_16x8_p1);
	init_sprite(si++, 66, 200, 32, 16, SF_ENABLE, &tile32x16p2_base, &pallet1_Green, sprite_renderer_sprite_32x16_p1);

	init_sprite(mot_index = si++, -1000, 9, 16, 8, SF_ENABLE, &tile16x8p2_invader[6], &pallet1_Red, sprite_renderer_sprite_16x8_p1);
	init_sprite(gun_index = si++, 20, FRAME_HEIGHT - 24, 16, 8, SF_ENABLE, &tile16x8p2_invader[7], &pallet1_Green, sprite_renderer_sprite_16x8_p1);

	inv_index = si;
	uint32_t rt[5] = {0, 2, 2, 4, 4};
	Pallet1_t* rp[5] = {&pallet1_White, &pallet1_Blue, &pallet1_Blue, &pallet1_Purple, &pallet1_Purple};

	for(uint32_t x = 0; x < 11; ++x) {
		for(uint32_t y = 0; y < 5; ++y) {
			init_sprite(si, x << 4, 30 + (y << 4), 16, 8, SF_ENABLE, &tile16x8p2_invader[rt[y]], rp[y], sprite_renderer_invader_16x8_p1);
			_spriteCollisionMasks[si] = (SpriteCollisionMask)4;
			si++;
		}
	}
	init_sprite(si++, 0, 0, 32*8, 24*8, SF_ENABLE, &_textGrid1, &pallet1_Green, text_renderer_8x8_p1);

}
void __not_in_flash_func(update_mother_ship)() {	
	Sprite *sprite = &_sprites[mot_index];
	sprite->x += 2;
	if(sprite->x > FRAME_WIDTH) sprite->x = -1000;
}

void __not_in_flash_func(core1_main)() {

	uint32_t frames = 0;
	while (true) {
		clear_sprite_collisions();
		for (uint32_t y = 0; y < FRAME_HEIGHT; ++y) {
			uint32_t *tmdsbuf;
			clear_sprite_id_row();
			queue_remove_blocking(&dvi0.q_tmds_free, &tmdsbuf);
			uint32_t *db = tmdsbuf;
			uint32_t *dg = db + FRAME_WIDTH;
			uint32_t *dr = dg + FRAME_WIDTH;

			// Render a blank row
			// TODO optionally render a tiled background
			render_row_mono(
				dr, dg, db,
				0, 0, 0);

			for (uint32_t i = 0; i < MAX_SPRITES; ++i)
			{
				const Sprite *sprite = &_sprites[i];
				const uint32_t r = y - sprite->y;
				if ((sprite-> f & SF_ENABLE) && r < sprite->h)
				{
					(sprite->r)(
						sprite->d1,
						sprite->d2,
						dr, dg, db,
						sprite->x,
						r,
						i);
				}
			}
			queue_add_blocking(&dvi0.q_tmds_valid, &tmdsbuf);
		}
		++frames;

		// Just messing about - start
		update_mother_ship();

		for (uint32_t i = 0; i < 2; ++i)
		{
			Sprite *sprite = &_sprites[i];
			if (_spriteCollisions.m[i]) sprite->d2 = &pallet1_Red;
		}
		_sprites[0].x++; if (_sprites[0].x > FRAME_WIDTH) {
			_sprites[0].x = -16;
			_sprites[0].d2 = &pallet1_Blue;
		}
		_sprites[1].x--; if (_sprites[1].x < -16) {
			_sprites[1].x = FRAME_WIDTH + 16; 
			_sprites[1].d2 = &pallet1_Purple;
		}

		bool reverse = false;
		for (uint32_t i = inv_index; i < inv_index + (5*11); ++i)
		{
			Sprite *sprite = &_sprites[i];
			sprite->x += inv_v;
			if (inv_v > 0) {
				if(sprite->x + 16 >= FRAME_WIDTH) reverse = true;
			}
			else {
				if(sprite->x <= 0) reverse = true;
			}
		}
		if (reverse) inv_v = -inv_v;
		// Just messing about - end
	}
}
/*
int __not_in_flash_func(main)() {
	vreg_set_voltage(VREG_VSEL);
	sleep_ms(10);
	// Run system at TMDS bit clock
	set_sys_clock_khz(DVI_TIMING.bit_clk_khz, true);
	
	printf("starting...\n");
	init_game();

	multicore_launch_core1(core1_main);

	while (1)
		__wfi();
}
	*/