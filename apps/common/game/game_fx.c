#include "game_fx.h"
#include "game_render.h"
#include "game_pallet.h"
#include "game_util.h"
#include <stdio.h>

#define GAME_FX_PARTICLES 128
#define GAME_FX_POPUPS 6
#define GAME_FX_POPUP_FRAMES 60
// Particles dim for their last few frames
#define GAME_FX_FADE 10

typedef struct {
	int32_t x, y, vx, vy;
	uint16_t life;
	uint8_t colour;
} GameParticle_t;

typedef struct {
	int32_t x;
	int32_t y;
	uint32_t life;
	GameText_t text;
} GamePopup_t;

static GameParticle_t _particles[GAME_FX_PARTICLES];
static uint32_t _next_particle = 0;
static GamePopup_t _popups[GAME_FX_POPUPS];
static uint32_t _flash = 0;

static int32_t screen_x(const int32_t x) {
	return x >> GAME_FP;
}

static int32_t no_wrap(const int32_t x) {
	return x;
}

static const GameFxView_t _screen = { screen_x, no_wrap, 0, GAME_SCREEN_H };
static const GameFxView_t* _view = &_screen;

static Tile8x8p2_t _tile_particle = {{
	0b11000000,
	0b11000000,
}};

void game_fx_set_view(const GameFxView_t* view) {
	_view = view ? view : &_screen;
}

void game_fx_init(void) {
	for (uint32_t i = 0; i < GAME_FX_PARTICLES; ++i) _particles[i].life = 0;
	for (uint32_t i = 0; i < GAME_FX_POPUPS; ++i) _popups[i].life = 0;
	_flash = 0;
}

// Takes the oldest particle if they are all in use
static GameParticle_t* new_particle(void) {
	GameParticle_t* p = &_particles[_next_particle];
	if (++_next_particle >= GAME_FX_PARTICLES) _next_particle = 0;
	return p;
}

static uint32_t particle_colour(const uint32_t colour) {
	return colour == GAME_COLOUR_RANDOM ? game_rand() % GAME_RAINBOW_COUNT : colour;
}

void game_fx_explode(int32_t x, int32_t y, uint32_t colour, uint32_t count, int32_t speed) {
	const uint32_t a0 = game_rand();
	for (uint32_t i = 0; i < count; ++i) {
		GameParticle_t* p = new_particle();
		const uint32_t a = a0 + (i << 8) / count;
		// Some go further than others
		const int32_t s = speed - (int32_t)(game_rand() % (uint32_t)(speed >> 1 | 1));
		p->x = x;
		p->y = y;
		p->vx = game_sin(a + 64) * s / 127;
		p->vy = game_sin(a) * s / 127;
		p->life = 24 + game_rand() % 20;
		p->colour = particle_colour(colour);
	}
}

void game_fx_implode(int32_t x, int32_t y, uint32_t colour, uint32_t count, uint32_t frames) {
	const uint32_t a0 = game_rand();
	for (uint32_t i = 0; i < count; ++i) {
		GameParticle_t* p = new_particle();
		const uint32_t a = a0 + (i << 8) / count;
		const int32_t s = GAME_PX(1) + (int32_t)(game_rand() % GAME_PX(2));
		p->vx = -game_sin(a + 64) * s / 127;
		p->vy = -game_sin(a) * s / 127;
		p->x = _view->wrap(x - p->vx * (int32_t)frames);
		p->y = y - p->vy * (int32_t)frames;
		p->life = frames;
		p->colour = particle_colour(colour);
	}
}

void game_fx_popup(int32_t x, int32_t y, uint32_t points) {
	for (uint32_t i = 0; i < GAME_FX_POPUPS; ++i) {
		GamePopup_t* p = &_popups[i];
		if (p->life) continue;
		char t[12];
		snprintf(t, sizeof(t), "%lu", (unsigned long)points);
		game_text_set(&p->text, t, 1);
		p->x = x;
		p->y = y;
		p->life = GAME_FX_POPUP_FRAMES;
		return;
	}
}

void game_fx_flash(uint32_t frames) {
	_flash = frames;
}

void __not_in_flash_func(game_fx_update)(void) {
	for (uint32_t i = 0; i < GAME_FX_PARTICLES; ++i) {
		GameParticle_t* p = &_particles[i];
		if (!p->life) continue;
		--p->life;
		p->x = _view->wrap(p->x + p->vx);
		p->y += p->vy;
	}
	for (uint32_t i = 0; i < GAME_FX_POPUPS; ++i) {
		if (_popups[i].life) --_popups[i].life;
	}
	if (_flash) --_flash;
}

void game_fx_draw_flash(uint32_t frame) {
	if (_flash) game_spr_fill(game_pallet((frame & 2) ? GAME_WHITE : GAME_GREY));
}

void game_fx_draw_popups(void) {
	for (uint32_t i = 0; i < GAME_FX_POPUPS; ++i) {
		GamePopup_t* p = &_popups[i];
		if (!p->life) continue;
		const int32_t sx = _view->screen_x(p->x);
		int32_t sy = p->y;
		if (_view->screen_y) {
			sy = _view->screen_y(GAME_PX(p->y));
			if (sy < _view->top || sy >= _view->bottom) continue;
		}
		if (sx < -32 || sx >= GAME_SCREEN_W) continue;
		game_spr_text(&p->text, sx, sy, game_pallet(GAME_WHITE));
	}
}

void __not_in_flash_func(game_fx_draw_particles)(void) {
	for (uint32_t i = 0; i < GAME_FX_PARTICLES; ++i) {
		GameParticle_t* p = &_particles[i];
		if (!p->life) continue;
		const int32_t sx = _view->screen_x(p->x);
		const int32_t sy = _view->screen_y ? _view->screen_y(p->y) : p->y >> GAME_FP;
		if (sx < -2 || sx >= GAME_SCREEN_W || sy < _view->top || sy >= _view->bottom) continue;
		hstx_dvi_pixel_t* c = p->life < GAME_FX_FADE ? game_pallet_dim(p->colour) : game_pallet(p->colour);
		game_spr_add(sx, sy, 8, 2, &_tile_particle, c, sprite_renderer_sprite_8x8_p1);
	}
}
