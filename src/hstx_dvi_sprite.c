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

// Clips a row of w pixels at x to the screen. m holds the pixels with bit 31
// as the leftmost; on return it only holds visible pixels starting at ux.
// Returns false if nothing is visible.
static __force_inline bool clip_row(
	uint32_t* const m,
	uint32_t* const ux,
	const int32_t x,
	const uint32_t w
) {
	*ux = (uint32_t)x;
	if (*ux <= (MODE_H_ACTIVE_PIXELS - w)) return true;
	if (x < 0) {
		const uint32_t s = 0u - *ux;
		if (s >= w) return false;
		*m <<= s;
		*ux = 0;
	}
	else {
		if (*ux >= MODE_H_ACTIVE_PIXELS) return false;
		*m &= ~(0xffffffffu >> (MODE_H_ACTIVE_PIXELS - *ux));
	}
	return *m != 0;
}

#if MODE_BYTES_PER_PIXEL == 1
// Byte masks for 4 pixels in a row word; nibble bit 3 is the leftmost (lowest address) pixel.
// Placed in RAM to avoid flash cache misses while rendering.
static const uint32_t __attribute__((section(".data._nibble_byte_mask"))) _nibble_byte_mask[16] = {
	0x00000000, 0xff000000, 0x00ff0000, 0xffff0000,
	0x0000ff00, 0xff00ff00, 0x00ffff00, 0xffffff00,
	0x000000ff, 0xff0000ff, 0x00ff00ff, 0xffff00ff,
	0x0000ffff, 0xff00ffff, 0x00ffffff, 0xffffffff
};

// Runs BODY for each row word wi holding visible pixels, with nib holding those
// pixels (bit 3 leftmost). Words with no pixels are skipped. m and ux are as left by clip_row.
#define FOR_EACH_PIXEL_NIBBLE(m, ux, wi, nib, BODY) \
	{ \
		uint32_t wi = (ux) >> 2; \
		uint64_t mm = ((uint64_t)(m) << 32) >> ((ux) & 3); \
		while (mm) { \
			const uint32_t z = __builtin_clzll(mm) & ~3u; \
			mm <<= z; \
			wi += z >> 2; \
			const uint32_t nib = (uint32_t)(mm >> 60); \
			BODY \
			mm <<= 4; \
			++wi; \
		} \
	}
#endif

static __force_inline void render_sprite_row_n_p1(
	const uint32_t d,
	const hstx_dvi_pixel_t* p1,
	hstx_dvi_row_t* r,
	const int32_t x,
	const SpriteId spriteId,
	const uint32_t w
) {
	uint32_t m = d << (32 - w);
	uint32_t ux;
	if (!m || !clip_row(&m, &ux, x, w)) return;

	const SpriteCollisionMask spriteCollisionMask = _spriteCollisionMasksRdy[spriteId];
	SpriteCollisionMask* const spriteCollisionsPtr = &_spriteCollisionsFrame.m[spriteId];
	const hstx_dvi_pixel_t p = p1[0];
#if MODE_BYTES_PER_PIXEL == 1
	// Write 4 pixels at a time where none of them are already taken by another sprite
	const uint32_t pq = p * 0x01010101u;
	const uint32_t idq = (spriteId + 1) * 0x01010101u;
	FOR_EACH_PIXEL_NIBBLE(m, ux, wi, nib, {
		const uint32_t bm = _nibble_byte_mask[nib];
		const uint32_t ids = _spriteIdRow.word[wi];
		if (!(ids & bm)) {
			r->w[wi] = (r->w[wi] & ~bm) | (pq & bm);
			_spriteIdRow.word[wi] = ids | (idq & bm);
		}
		else {
			for (uint32_t b = nib; b; ) {
				const uint32_t k = __builtin_clz(b) - 28;
				b ^= 8u >> k;
				render_sprite_pixel(r, p, spriteId, spriteCollisionMask, spriteCollisionsPtr, (wi << 2) + k);
			}
		}
	})
#else
	// Visit the set pixels only
	while (m) {
		const uint32_t i = __builtin_clz(m);
		m ^= 0x80000000u >> i;
		render_sprite_pixel(r, p, spriteId, spriteCollisionMask, spriteCollisionsPtr, ux + i);
	}
#endif
}

static __force_inline void render_row_n_p1(
	const uint32_t d,
	const hstx_dvi_pixel_t* p1,
	hstx_dvi_row_t* r,
	const int32_t x,
	const uint32_t w
) {
	uint32_t m = d << (32 - w);
	uint32_t ux;
	if (!m || !clip_row(&m, &ux, x, w)) return;

	const hstx_dvi_pixel_t p = p1[0];
#if MODE_BYTES_PER_PIXEL == 1
	const uint32_t pq = p * 0x01010101u;
	FOR_EACH_PIXEL_NIBBLE(m, ux, wi, nib, {
		const uint32_t bm = _nibble_byte_mask[nib];
		r->w[wi] = (r->w[wi] & ~bm) | (pq & bm);
	})
#else
	while (m) {
		const uint32_t i = __builtin_clz(m);
		m ^= 0x80000000u >> i;
		hstx_dvi_row_set_pixel(r, ux + i, p);
	}
#endif
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

#if MODE_BYTES_PER_PIXEL == 1
	// Whole grid on screen and word aligned: 2 masked word writes per character
	if (!(x & 3) && ((w << 3) <= MODE_H_ACTIVE_PIXELS) && ((uint32_t)x <= MODE_H_ACTIVE_PIXELS - (w << 3))) {
		const uint32_t pq = p1[0] * 0x01010101u;
		uint32_t* rw = &r->w[(uint32_t)x >> 2];
		for(uint32_t i = 0; i < w; ++i, rw += 2) {
			const uint32_t d = font_8x8[text_grid8_glyph_offset(s[i], k)];
			if (d) {
				const uint32_t bm1 = _nibble_byte_mask[d >> 4];
				const uint32_t bm2 = _nibble_byte_mask[d & 15];
				rw[0] = (rw[0] & ~bm1) | (pq & bm1);
				rw[1] = (rw[1] & ~bm2) | (pq & bm2);
			}
		}
		return;
	}
#endif

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
		hstx_dvi_row_set_pixel(r, x + i, d & (1<<15) ? fg : bg);
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

