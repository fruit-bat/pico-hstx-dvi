// Copyright (c) 2024 fruit-bat.

// The charts of the galaxy and the data on its systems, as in the original
// game, with a cursor for picking a system: the galactic chart, the short
// range chart round the system we are at, and the data on a system with a
// picture of it.
#pragma once

#include "el_draw.h"
#include "galaxy.h"

#ifdef __cplusplus
extern "C" {
#endif

// The galaxy, from 0, and the system we are at in it
void chart_set_galaxy(uint32_t g);
uint32_t chart_galaxy(void);
void chart_set_home(uint32_t i);
uint32_t chart_home(void);
// Finds the systems on the short range chart again, after draw_aspect changes
void chart_refresh(void);
const GalaxySystem_t* chart_system(uint32_t i);
// The k-th nearest system to us, us being the 0th
uint32_t chart_nearest(uint32_t k);
// The colour a system is shown in, from what it makes
const uint8_t* chart_economy_rgb(uint32_t economy);

// The cursor, in steps across the galaxy. It moves freely, or glides from
// one system to another by itself, and picks the system nearest it.
void chart_cursor_set(float x, float y);
// Kept on the short range chart if local is set
void chart_cursor_move(float dx, float dy, bool local);
void chart_cursor_glide(bool local, uint32_t frame);
uint32_t chart_selected(void);

void chart_draw_galaxy(void);
// The charts and data are always wireframes, to look like data: solid unless style is STYLE_WIRE
void chart_draw_local(Style_e style);
// Returns the name of the system
const char* chart_draw_data(uint32_t i, Style_e style, uint32_t frame);

#ifdef __cplusplus
}
#endif
