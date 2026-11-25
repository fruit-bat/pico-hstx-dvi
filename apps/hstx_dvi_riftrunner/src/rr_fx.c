// Copyright (c) 2024 fruit-bat.

// See rr_fx.h.

#include "rr_fx.h"
#include "rr_world.h"
#include "rr_art.h"
#include "game_render.h"
#include "game_util.h"

#define RR_FX_BOOMS 20
#define RR_FX_CHUNKS 48
#define RR_FX_BOOM_LIFE 26
// Gravity on the chunks, a frame
#define RR_FX_GRAVITY 20

typedef struct {
	int32_t x, y;
	uint8_t age;
	uint8_t r;     // the biggest it gets
	uint8_t delay; // frames before it starts
} RrBoomFx_t;

typedef struct {
	int32_t x, y, vx, vy;
	uint8_t life;
	uint8_t size;
	hstx_dvi_pixel_t p;
} RrChunk_t;

static RrBoomFx_t _booms[RR_FX_BOOMS];
static RrChunk_t _chunks[RR_FX_CHUNKS];
static uint32_t _next_boom = 0, _next_chunk = 0;
// Half the width of a disc of radius r, dy from its middle row, as the
// square root of r * r - dy * dy
static uint8_t _sqrt[RR_FX_MAX_R * RR_FX_MAX_R + 1];
static bool _made = false;

// Fire cooling down, from white hot
static const hstx_dvi_pixel_t _fire[] = {
	RR_RGB(255, 255, 255), RR_RGB(255, 255, 160), RR_RGB(255, 224, 0), RR_RGB(255, 160, 0),
	RR_RGB(255, 96, 0), RR_RGB(224, 32, 0), RR_RGB(160, 0, 0), RR_RGB(96, 0, 0),
};
#define RR_FIRE_COUNT (sizeof(_fire) / sizeof(_fire[0]))

static const hstx_dvi_pixel_t _chunk_colours[] = {
	RR_RGB(255, 255, 255), RR_RGB(255, 224, 0), RR_RGB(255, 128, 0), RR_RGB(160, 160, 160), RR_RGB(255, 64, 0),
};

void rr_fx_init(void) {
	if (!_made) {
		for (uint32_t v = 0; v <= RR_FX_MAX_R * RR_FX_MAX_R; ++v) {
			uint32_t s = 0;
			while ((s + 1) * (s + 1) <= v) ++s;
			_sqrt[v] = s;
		}
		_made = true;
	}
	for (uint32_t i = 0; i < RR_FX_BOOMS; ++i) _booms[i].r = 0;
	for (uint32_t i = 0; i < RR_FX_CHUNKS; ++i) _chunks[i].life = 0;
}

static void boom(int32_t x, int32_t y, uint32_t r, uint32_t delay) {
	RrBoomFx_t* b = &_booms[_next_boom];
	if (++_next_boom >= RR_FX_BOOMS) _next_boom = 0;
	b->x = x;
	b->y = y;
	b->r = r > RR_FX_MAX_R ? RR_FX_MAX_R : r;
	b->age = 0;
	b->delay = delay;
}

static void chunks(int32_t x, int32_t y, uint32_t n, int32_t speed) {
	const uint32_t a0 = game_rand();
	for (uint32_t i = 0; i < n; ++i) {
		RrChunk_t* c = &_chunks[_next_chunk];
		if (++_next_chunk >= RR_FX_CHUNKS) _next_chunk = 0;
		const uint32_t a = a0 + (i << 8) / n + (game_rand() & 15);
		const int32_t s = speed / 2 + (int32_t)(game_rand() % (uint32_t)(speed / 2 + 1));
		c->x = x;
		c->y = y;
		// Carried along with the land a little
		c->vx = game_sin(a + 64) * s / 127 + RR_SCROLL / 2;
		c->vy = game_sin(a) * s / 127 - RR_PX(1);
		c->life = 30 + game_rand() % 30;
		c->size = 3 + (game_rand() & 3);
		c->p = _chunk_colours[game_rand() % (sizeof(_chunk_colours) / sizeof(_chunk_colours[0]))];
	}
}

void rr_fx_explode(int32_t x, int32_t y, uint32_t size) {
	switch (size) {
		case RR_BOOM_SMALL:
			boom(x, y, 12, 0);
			chunks(x, y, 4, RR_PX(2));
			break;
		case RR_BOOM_MEDIUM:
			boom(x, y, 22, 0);
			boom(x + RR_PX(8), y - RR_PX(6), 12, 6);
			chunks(x, y, 8, RR_PX(3));
			break;
		case RR_BOOM_BIG:
			boom(x, y, 34, 0);
			boom(x - RR_PX(14), y + RR_PX(8), 18, 5);
			boom(x + RR_PX(12), y - RR_PX(10), 20, 9);
			chunks(x, y, 14, RR_PX(4));
			break;
		default:
			boom(x, y, 60, 0);
			boom(x - RR_PX(30), y + RR_PX(16), 32, 6);
			boom(x + RR_PX(28), y - RR_PX(20), 36, 10);
			boom(x + RR_PX(6), y + RR_PX(30), 28, 16);
			chunks(x, y, 24, RR_PX(6));
			break;
	}
}

void __not_in_flash_func(rr_fx_update)(void) {
	for (uint32_t i = 0; i < RR_FX_BOOMS; ++i) {
		RrBoomFx_t* b = &_booms[i];
		if (!b->r) continue;
		if (b->delay) {
			--b->delay;
			continue;
		}
		if (++b->age >= RR_FX_BOOM_LIFE) b->r = 0;
	}
	for (uint32_t i = 0; i < RR_FX_CHUNKS; ++i) {
		RrChunk_t* c = &_chunks[i];
		if (!c->life) continue;
		--c->life;
		c->x += c->vx;
		c->y += c->vy;
		c->vy += RR_FX_GRAVITY;
	}
}

// d1 holds the radius, the hole's radius above it and the colour above that
static void __not_in_flash_func(disc_renderer)(
	const void* d1,
	const void* d2,
	hstx_dvi_row_t* r,
	const int32_t x,
	const int32_t row,
	const SpriteId spriteId
) {
	const uint32_t v = (uint32_t)(uintptr_t)d1;
	const int32_t rad = v & 0xff;
	const int32_t inner = (v >> 8) & 0xff;
	const hstx_dvi_pixel_t p = (hstx_dvi_pixel_t)(v >> 16);
	const int32_t dy = row - rad;
	const int32_t hw = _sqrt[rad * rad - dy * dy];
	const int32_t cx = x + rad;
	if (inner && dy < inner && dy > -inner) {
		const int32_t ihw = _sqrt[inner * inner - dy * dy];
		sprite_render_span(r, cx - hw, cx - ihw, p, spriteId);
		sprite_render_span(r, cx + ihw + 1, cx + hw + 1, p, spriteId);
	}
	else {
		sprite_render_span(r, cx - hw, cx + hw + 1, p, spriteId);
	}
}

void __not_in_flash_func(rr_fx_disc)(int32_t x, int32_t y, uint32_t r, uint32_t inner, hstx_dvi_pixel_t p) {
	if (r > RR_FX_MAX_R) r = RR_FX_MAX_R;
	if (inner >= r) return;
	game_spr_add(x - (int32_t)r, y - (int32_t)r, 2 * r + 1, 2 * r + 1, (void*)(uintptr_t)(r | (inner << 8) | ((uint32_t)p << 16)), NULL, disc_renderer);
}

void __not_in_flash_func(rr_fx_draw)(void) {
	for (uint32_t i = 0; i < RR_FX_BOOMS; ++i) {
		const RrBoomFx_t* b = &_booms[i];
		if (!b->r || b->delay) continue;
		const uint32_t age = b->age;
		// Swells quickly, then burns out from the middle
		uint32_t r = age < 8 ? 3 + (b->r - 3) * age / 8 : b->r;
		uint32_t inner = age < 12 ? 0 : r * (age - 12) / (RR_FX_BOOM_LIFE - 12);
		const uint32_t c = age * RR_FIRE_COUNT / RR_FX_BOOM_LIFE;
		// Kept off the score and power bar
		const int32_t sy = b->y >> RR_FP;
		if (sy <= RR_PLAY_TOP || sy >= RR_PLAY_BOTTOM) continue;
		if ((int32_t)r > sy - RR_PLAY_TOP) r = sy - RR_PLAY_TOP;
		if ((int32_t)r > RR_PLAY_BOTTOM - 1 - sy) r = RR_PLAY_BOTTOM - 1 - sy;
		if (inner >= r) continue;
		// A hotter heart while it swells, the rest a ring round it, so no
		// pixel is drawn twice
		if (age < 10 && r > 6) {
			rr_fx_disc(rr_world_sx(b->x), b->y >> RR_FP, r / 2, 0, _fire[c > 1 ? c - 2 : 0]);
			if (inner < r / 2 + 1) inner = r / 2 + 1;
		}
		rr_fx_disc(rr_world_sx(b->x), b->y >> RR_FP, r, inner, _fire[c]);
	}
	for (uint32_t i = 0; i < RR_FX_CHUNKS; ++i) {
		const RrChunk_t* c = &_chunks[i];
		if (!c->life) continue;
		const int32_t sx = rr_world_sx(c->x), sy = c->y >> RR_FP;
		if (sx < -8 || sx >= GAME_SCREEN_W || sy < RR_PLAY_TOP || sy >= RR_PLAY_BOTTOM) continue;
		rr_spr_rect(sx, sy, c->size, c->size, c->life < 10 ? RR_RGB(128, 32, 0) : c->p);
	}
}
