#include "hstx_dvi_sprite.h"
#include "hstx_dvi_core.h"
#include "hstx_dvi_row_fifo.h"
#include "hstx_dvi_row_buf.h"
#include "pico/sem.h" 
#include "pico/multicore.h"
#include <memory.h>

Sprite _sprites[MAX_SPRITES];
static Sprite _sprites_rdy[MAX_SPRITES];

static SpriteCollisionMask _spriteCollisionMasks[MAX_SPRITES];
static SpriteCollisionMask _spriteCollisionMasksRdy[MAX_SPRITES];
static SpriteIdRow _spriteIdRow;
SpriteCollisions _spriteCollisions;
static SpriteCollisions _spriteCollisionsFrame;
// Released by the application core when it has finished updating the sprites
static struct semaphore _update_sem;
// Released by the render core once the updates have been taken
static struct semaphore _frame_sem;

__force_inline void clear_sprite_collisions(SpriteCollisions* sc) {
	for(uint32_t i = 0; i < MAX_SPRITES; ++i) sc->m[i] = 0;
}

void __not_in_flash_func(hstx_dvi_sprite_render_loop)() {

    hstx_dvi_init(hstx_dvi_row_fifo_get_row_fetcher());

    while(true) {
        hstx_dvi_sprite_render_frame();
		// Only take the updates once the other core has finished making them.
		// It stays blocked in hstx_dvi_sprite_wait_for_frame until the copy is done.
		if(sem_try_acquire(&_update_sem)) {
			memcpy(_sprites_rdy, _sprites, sizeof(_sprites));
			memcpy(_spriteCollisionMasksRdy, _spriteCollisionMasks, sizeof(_spriteCollisionMasks));
			memcpy(&_spriteCollisions, &_spriteCollisionsFrame, sizeof(_spriteCollisionsFrame));
			clear_sprite_collisions(&_spriteCollisionsFrame);
			sem_release(&_frame_sem);
		}
    }
}

void __not_in_flash_func(hstx_dvi_sprite_wait_for_frame)(){
    sem_release(&_update_sem);
    sem_acquire_blocking(&_frame_sem);
}

void hstx_dvi_sprite_set_sprite_collision_mask(
	const SpriteId spriteId,
	const SpriteCollisionMask mask
) {
	if (spriteId < MAX_SPRITES) {
		_spriteCollisionMasks[spriteId] = mask;
	}
}

void __not_in_flash_func(hstx_dvi_sprite_init_all)() {
    // Initialize the row buffer
    hstx_dvi_row_buf_init();
	// Clear down the sprites	
	for(uint32_t i = 0; i < MAX_SPRITES; ++i) {
		_sprites[i].f = 0;
		_sprites_rdy[i].f = 0;
	}
	// Set up the frame handshake semaphores
    sem_init(&_update_sem, 0, 1);
    sem_init(&_frame_sem, 0, 1);

    // Initialize the HSTX DVI row FIFO.
    hstx_dvi_row_fifo_init1(pio0);

	// Clear down any collision flags
	clear_sprite_collisions(&_spriteCollisionsFrame);
	clear_sprite_collisions(&_spriteCollisions);

	// Start the renderer
	multicore_launch_core1(hstx_dvi_sprite_render_loop);
}

void __not_in_flash_func(clear_sprite_id_row)() {
	for(uint32_t i = 0; i < SPRITE_ID_ROW_WORDS; ++i) _spriteIdRow.word[i] = 0;
}

void __not_in_flash_func(render_row_mono)(
	hstx_dvi_row_t* r,
	hstx_dvi_pixel_t p
) {
    hstx_dvi_fill_row(r, p);
}

static __force_inline void render_sprite_pixel(
	hstx_dvi_row_t* r,
	const hstx_dvi_pixel_t p,
	const SpriteId spriteId,
	const SpriteCollisionMask spriteCollisionMask,
	SpriteCollisionMask* const spriteCollisionsPtr,
	const uint32_t j
) {
	const SpriteId ncid = _spriteIdRow.id[j];
	if (ncid)
	{
		const SpriteId cid = ncid - 1;
		_spriteCollisionsFrame.m[cid] |= spriteCollisionMask;
		*spriteCollisionsPtr |= _spriteCollisionMasksRdy[cid];
	}
	else
	{
		hstx_dvi_row_set_pixel(r, j, p);
		_spriteIdRow.id[j] = spriteId + 1;
	}
}

static __force_inline void render_pixel(
	hstx_dvi_row_t* r,
    hstx_dvi_pixel_t p,
    const uint32_t j
) {
    hstx_dvi_row_set_pixel(r, j, p);
}

static __force_inline void render_sprite_row_n_p1(
	const uint32_t d,
	const hstx_dvi_pixel_t* p1,
	hstx_dvi_row_t* r,
	const int32_t x,
	const SpriteId spriteId,
	const uint32_t w
) {
	if (d)
	{
		const SpriteCollisionMask spriteCollisionMask = _spriteCollisionMasksRdy[spriteId];
		SpriteCollisionMask* const spriteCollisionsPtr = &_spriteCollisionsFrame.m[spriteId];
		const uint32_t bm = 1u << (w-1);
        const hstx_dvi_pixel_t p = p1[0];
		const uint32_t ux = (uint32_t)x;
		if (ux <= (MODE_H_ACTIVE_PIXELS - w))
		{
			for (int32_t i = 0; i < w; i++)
			{
				const uint32_t j = ux + i;
				if (d & (bm >> i))
				{
					render_sprite_pixel(r, p, spriteId, spriteCollisionMask, spriteCollisionsPtr, j);
				}
			}
		}
		else
		{
			for (int32_t i = 0; i < w; i++)
			{
				const uint32_t j = ux + i;
				if ((j < MODE_H_ACTIVE_PIXELS) && (d & (bm >> i)))
				{
					render_sprite_pixel(r, p, spriteId, spriteCollisionMask, spriteCollisionsPtr, j);
				}
			}
		}
	}
}

static __force_inline void render_row_n_p1(
	const uint32_t d,
	const hstx_dvi_pixel_t* p1,
	hstx_dvi_row_t* r,
	const int32_t x,
	const uint32_t w
) {
	if (d)
	{
        const hstx_dvi_pixel_t p = p1[0];
		const uint32_t bm = 1u << (w-1);
		if (((uint32_t)x) <= (MODE_H_ACTIVE_PIXELS - w))
		{
			for (int32_t i = 0; i < w; i++)
			{
				const uint32_t j = (uint32_t)x + i;
				if (d & (bm >> i))
				{
					render_pixel(r, p, j);
				}
			}
		}
		else
		{
			for (int32_t i = 0; i < w; i++)
			{
				const uint32_t j = (uint32_t)x + i;
				if ((j < MODE_H_ACTIVE_PIXELS) && (d & (bm >> i)))
				{
					render_pixel(r, p, j);
				}
			}
		}
	}
}

// Characters outside the font are rendered as a space
static __force_inline uint32_t text_grid8_glyph_offset(const uint8_t c, const uint32_t k) {
	uint32_t g = (uint32_t)c - TEXT_GRID8_FIRST_CHAR;
	if (g > (TEXT_GRID8_LAST_CHAR - TEXT_GRID8_FIRST_CHAR)) g = ' ' - TEXT_GRID8_FIRST_CHAR;
	return k + (g << 3);
}

static __force_inline void render_row_text_8_p1(
	const TextGrid8_t *tg,
	const hstx_dvi_pixel_t* p1,
	hstx_dvi_row_t* r,
	const int32_t x,
	const int32_t row
) {
	uint8_t * const s = tg->s + __mul_instruction((row >> 3), tg->w);
	const uint32_t k = row & 7;
	const uint32_t w = tg->w;
	const uint32_t v = w >> 2;
    uint8_t* font_8x8 = tg->font;

	for(uint32_t i = 0; i < v; ++i) {
		const uint32_t q = i << 2;
		const uint8_t d1 = font_8x8[text_grid8_glyph_offset(s[q], k)];
		const uint8_t d2 = font_8x8[text_grid8_glyph_offset(s[q+1], k)];
		const uint8_t d3 = font_8x8[text_grid8_glyph_offset(s[q+2], k)];
		const uint8_t d4 = font_8x8[text_grid8_glyph_offset(s[q+3], k)];
		const uint32_t g = (((uint32_t)d1) << 24) | (((uint32_t)d2) << 16) | (((uint32_t)d3) << 8) | d4;
		render_row_n_p1(
			g,
			p1,
			r,
			x + (i << 5),
			32
		);
	}
	for(uint32_t i = w & -4; i < w; ++i) {
		const uint8_t d = font_8x8[text_grid8_glyph_offset(s[i], k)];
		render_row_n_p1(
			d,
			p1,
			r,
			x + (i << 3),
			8
		);
	}
}


static __force_inline void render_Tile16x16p1(
	const Tile16x16p2_t * const t,
	const hstx_dvi_pixel_t* p1,
	hstx_dvi_row_t* r,
	const int32_t x,
	const int32_t row,
	const SpriteId spriteId
) {
	uint32_t d = t->d[row];
	render_sprite_row_n_p1(
		d,
		p1,
		r,
		x,
		spriteId,
		16
	);
}

static __force_inline void render_Tile8x8p1(
	const Tile8x8p2_t * const t,
	const hstx_dvi_pixel_t* p1,
	hstx_dvi_row_t* r,
	const int32_t x,
	const int32_t row,
	const SpriteId spriteId
) {
	uint32_t d = t->d[row];
	render_sprite_row_n_p1(
		d,
		p1,
		r,
		x,
		spriteId,
		8
	);
}

static __force_inline void render_Tile16x8p1(
	const Tile16x8p2_t * const t,
	const hstx_dvi_pixel_t* p1,
	hstx_dvi_row_t* r,
	const int32_t x,
	const int32_t row,
	const SpriteId spriteId
) {
	uint32_t d = t->d[row];
	render_sprite_row_n_p1(
		d,
		p1,
		r,
		x,
		spriteId,
		16
	);
}

static __force_inline void render_Tile32x16p1(
	const Tile32x16p2_t * const t,
	const hstx_dvi_pixel_t* p1,
	hstx_dvi_row_t* r,
	const int32_t x,
	const int32_t row,
	const SpriteId spriteId
) {
	uint32_t d = t->d[row];
	render_sprite_row_n_p1(
		d,
		p1,
		r,
		x,
		spriteId,
		32
	);
}

void __not_in_flash_func(sprite_renderer_sprite_8x8_p1)(
	const void* d1,
	const void* d2,
	hstx_dvi_row_t* r,
	const int32_t x,
	const int32_t row,
	const SpriteId spriteId
) {
	render_Tile8x8p1(
		d1,
		d2,
		r,
		x,
		row,
		spriteId
	);
}

void __not_in_flash_func(sprite_renderer_sprite_16x8_p1)(
	const void* d1,
	const void* d2,
	hstx_dvi_row_t* r,
	const int32_t x,
	const int32_t row,
	const SpriteId spriteId
) {
	render_Tile16x8p1(
		d1,
		d2,
		r,
		x,
		row,
		spriteId
	);
}

void __not_in_flash_func(sprite_renderer_sprite_16x16_p1)(
	const void* d1,
	const void* d2,
	hstx_dvi_row_t* r,
	const int32_t x,
	const int32_t row,
	const SpriteId spriteId
) {
	render_Tile16x16p1(
		d1,
		d2,
		r,
		x,
		row,
		spriteId
	);
}

void __not_in_flash_func(sprite_renderer_sprite_32x16_p1)(
	const void* d1,
	const void* d2,
	hstx_dvi_row_t* r,
	const int32_t x,
	const int32_t row,
	const SpriteId spriteId
) {
	render_Tile32x16p1(
		d1,
		d2,
		r,
		x,
		row,
		spriteId
	);
}

void __not_in_flash_func(text_renderer_8x8_p1)(
	const void* d1,
	const void* d2,
	hstx_dvi_row_t* r,
	const int32_t x,
	const int32_t row,
	const SpriteId spriteId
) {
	render_row_text_8_p1(
		(TextGrid8_t *)d1,
		d2,
		r,
		x,
		row
	);
}

void __not_in_flash_func(render_Tile16x16p2)(
	Tile16x16p2_t *t,
	const hstx_dvi_pixel_t* p2,
	hstx_dvi_row_t* r,
	int32_t x,
	int32_t row
) {
    const hstx_dvi_pixel_t bg = p2[0];
    const hstx_dvi_pixel_t fg = p2[1];
	uint16_t d = t->d[row];
	for(int32_t i = 0; i < 16; i++) {
		render_pixel(r, d & (1<<15) ? fg : bg, x + i);
		d <<= 1;
	}
}

void __not_in_flash_func(hstx_dvi_sprite_render_frame)() {
		
	//clear_sprite_collisions();
	for (uint32_t y = 0; y < MODE_V_ACTIVE_LINES; ++y) {
		hstx_dvi_row_t *r = hstx_dvi_row_buf_get();
		clear_sprite_id_row();

		// Render a blank row
		// TODO optionally render a tiled background
		render_row_mono(
			r,
			hstx_dvi_pixel_rgb(0, 0, 0));

		for (uint32_t i = 0; i < MAX_SPRITES; ++i)
		{
			const Sprite *sprite = &_sprites_rdy[i];
			const uint32_t k = y - sprite->y;
			if ((sprite-> f & SF_ENABLE) && k < sprite->h)
			{
				(sprite->r)(
					sprite->d1,
					sprite->d2,
					r,
					sprite->x,
					k,
					i);
			}
		}
		hstx_dvi_row_fifo_put_blocking(r);
	}
}

