#pragma once

#include "pico/stdlib.h"

#ifdef __cplusplus
extern "C" {
#endif

void def_fx_init(void);

// Bursts particles out from a world position. speed is in fixed point pixels per frame.
void def_fx_explode(int32_t x, int32_t y, uint32_t colour, uint32_t count, int32_t speed);

// Draws particles in to a world position over a number of frames, for things materialising
void def_fx_implode(int32_t x, int32_t y, uint32_t colour, uint32_t count, uint32_t frames);

// Shows some points scored at a world position for a while
void def_fx_popup(int32_t x, int32_t y, uint32_t points);

// Lights up the whole screen for a few frames
void def_fx_flash(uint32_t frames);

void def_fx_update(void);

// The flash goes first, then the popups, then the particles
void def_fx_draw_flash(uint32_t frame);
void def_fx_draw_popups(void);
void def_fx_draw_particles(void);

#ifdef __cplusplus
}
#endif
