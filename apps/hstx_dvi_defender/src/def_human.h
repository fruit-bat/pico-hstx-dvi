#pragma once

#include "pico/stdlib.h"
#include "hstx_dvi_sprite.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DEF_HUMAN_MAX 10
#define DEF_HUMAN_W 6
#define DEF_HUMAN_H 12
#define DEF_HUMAN_CATCH_SCORE 500
#define DEF_HUMAN_RETURN_SCORE 500
#define DEF_HUMAN_LAND_SCORE 250

// Puts count humanoids down on the planet
void def_human_init(uint32_t count);

void def_human_update(uint32_t frame);
void def_human_draw(uint32_t frame);
void def_human_scan(void);

uint32_t def_human_alive(void);

// A random humanoid on the ground, or -1 if there are none
int32_t def_human_pick(void);
bool def_human_on_ground(int32_t i);
int32_t def_human_x(int32_t i);
int32_t def_human_y(int32_t i);

// For the lander that has got hold of one
void def_human_grab(int32_t i);
void def_human_carry(int32_t i, int32_t x, int32_t y);
void def_human_release(int32_t i);
void def_human_kill(int32_t i);

void def_human_draw_icon(int32_t sx, int32_t sy);

#ifdef __cplusplus
}
#endif
