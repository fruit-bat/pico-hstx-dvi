#include "def_fx.h"
#include "def_world.h"
#include "def_render.h"
#include "def_pallet.h"
#include "def_util.h"
#include <stdio.h>

#define DEF_FX_PARTICLES 128
#define DEF_FX_POPUPS 6
#define DEF_FX_POPUP_FRAMES 60
// Particles dim for their last few frames
#define DEF_FX_FADE 10

typedef struct {
	int32_t x, y, vx, vy;
	uint16_t life;
	uint8_t colour;
} DefParticle_t;

typedef struct {
	int32_t x;
	int32_t y;
	uint32_t life;
	DefText_t text;
} DefPopup_t;

static DefParticle_t _particles[DEF_FX_PARTICLES];
static uint32_t _next_particle = 0;
static DefPopup_t _popups[DEF_FX_POPUPS];
static uint32_t _flash = 0;

static Tile8x8p2_t _tile_particle = {{
	0b11000000,
	0b11000000,
}};

void def_fx_init(void) {
	for (uint32_t i = 0; i < DEF_FX_PARTICLES; ++i) _particles[i].life = 0;
	for (uint32_t i = 0; i < DEF_FX_POPUPS; ++i) _popups[i].life = 0;
	_flash = 0;
}

// Takes the oldest particle if they are all in use
static DefParticle_t* new_particle(void) {
	DefParticle_t* p = &_particles[_next_particle];
	if (++_next_particle >= DEF_FX_PARTICLES) _next_particle = 0;
	return p;
}

static uint32_t particle_colour(const uint32_t colour) {
	return colour == DEF_COLOUR_RANDOM ? def_rand() % DEF_RAINBOW_COUNT : colour;
}

void def_fx_explode(int32_t x, int32_t y, uint32_t colour, uint32_t count, int32_t speed) {
	const uint32_t a0 = def_rand();
	for (uint32_t i = 0; i < count; ++i) {
		DefParticle_t* p = new_particle();
		const uint32_t a = a0 + (i << 8) / count;
		// Some go further than others
		const int32_t s = speed - (int32_t)(def_rand() % (uint32_t)(speed >> 1 | 1));
		p->x = x;
		p->y = y;
		p->vx = def_sin(a + 64) * s / 127;
		p->vy = def_sin(a) * s / 127;
		p->life = 24 + def_rand() % 20;
		p->colour = particle_colour(colour);
	}
}

void def_fx_implode(int32_t x, int32_t y, uint32_t colour, uint32_t count, uint32_t frames) {
	const uint32_t a0 = def_rand();
	for (uint32_t i = 0; i < count; ++i) {
		DefParticle_t* p = new_particle();
		const uint32_t a = a0 + (i << 8) / count;
		const int32_t s = DEF_PX(1) + (int32_t)(def_rand() % DEF_PX(2));
		p->vx = -def_sin(a + 64) * s / 127;
		p->vy = -def_sin(a) * s / 127;
		p->x = def_wrap(x - p->vx * (int32_t)frames);
		p->y = y - p->vy * (int32_t)frames;
		p->life = frames;
		p->colour = particle_colour(colour);
	}
}

void def_fx_popup(int32_t x, int32_t y, uint32_t points) {
	for (uint32_t i = 0; i < DEF_FX_POPUPS; ++i) {
		DefPopup_t* p = &_popups[i];
		if (p->life) continue;
		char t[12];
		snprintf(t, sizeof(t), "%lu", (unsigned long)points);
		def_text_set(&p->text, t, 1);
		p->x = x;
		p->y = y;
		p->life = DEF_FX_POPUP_FRAMES;
		return;
	}
}

void def_fx_flash(uint32_t frames) {
	_flash = frames;
}

void __not_in_flash_func(def_fx_update)(void) {
	for (uint32_t i = 0; i < DEF_FX_PARTICLES; ++i) {
		DefParticle_t* p = &_particles[i];
		if (!p->life) continue;
		--p->life;
		p->x = def_wrap(p->x + p->vx);
		p->y += p->vy;
	}
	for (uint32_t i = 0; i < DEF_FX_POPUPS; ++i) {
		if (_popups[i].life) --_popups[i].life;
	}
	if (_flash) --_flash;
}

void def_fx_draw_flash(uint32_t frame) {
	if (_flash) def_spr_fill(def_pallet((frame & 2) ? DEF_WHITE : DEF_GREY));
}

void def_fx_draw_popups(void) {
	for (uint32_t i = 0; i < DEF_FX_POPUPS; ++i) {
		DefPopup_t* p = &_popups[i];
		if (!p->life) continue;
		const int32_t sx = def_world_screen_x(p->x);
		if (sx < -32 || sx >= DEF_SCREEN_W) continue;
		def_spr_text(&p->text, sx, p->y, def_pallet(DEF_WHITE));
	}
}

void __not_in_flash_func(def_fx_draw_particles)(void) {
	for (uint32_t i = 0; i < DEF_FX_PARTICLES; ++i) {
		DefParticle_t* p = &_particles[i];
		if (!p->life) continue;
		const int32_t sx = def_world_screen_x(p->x);
		const int32_t sy = p->y >> DEF_FP;
		if (sx < -2 || sx >= DEF_SCREEN_W || sy < DEF_HUD_H || sy >= DEF_SCREEN_H) continue;
		hstx_dvi_pixel_t* c = p->life < DEF_FX_FADE ? def_pallet_dim(p->colour) : def_pallet(p->colour);
		def_spr_add(sx, sy, 8, 2, &_tile_particle, c, sprite_renderer_sprite_8x8_p1);
	}
}
