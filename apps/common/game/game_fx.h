#pragma once

#include "pico/stdlib.h"

#ifdef __cplusplus
extern "C" {
#endif

// How positions get on to the screen. Positions are fixed point, see GAME_FP.
// The popups are the exception, with y in pixels.
typedef struct {
	// Turns an x position into a screen x in pixels
	int32_t (*screen_x)(int32_t x);
	// Keeps an x position in range, for a world that wraps round
	int32_t (*wrap)(int32_t x);
	// Particles are only drawn between these screen rows
	int32_t top;
	int32_t bottom;
} GameFxView_t;

// NULL for positions on the screen, which is how it starts off
void game_fx_set_view(const GameFxView_t* view);

void game_fx_init(void);

// Bursts particles out from a position. speed is in fixed point pixels per frame.
void game_fx_explode(int32_t x, int32_t y, uint32_t colour, uint32_t count, int32_t speed);

// Draws particles in to a position over a number of frames, for things materialising
void game_fx_implode(int32_t x, int32_t y, uint32_t colour, uint32_t count, uint32_t frames);

// Shows some points scored at a position for a while
void game_fx_popup(int32_t x, int32_t y, uint32_t points);

// Lights up the whole screen for a few frames
void game_fx_flash(uint32_t frames);

void game_fx_update(void);

// The flash goes first, then the popups, then the particles
void game_fx_draw_flash(uint32_t frame);
void game_fx_draw_popups(void);
void game_fx_draw_particles(void);

#ifdef __cplusplus
}
#endif
