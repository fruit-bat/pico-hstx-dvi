#pragma once

#include "pico/stdlib.h"

#ifdef __cplusplus
extern "C" {
#endif

#define WW_WITCH_W 24
#define WW_WITCH_H 32
// Her feet, which is what bumps into things
#define WW_WITCH_HW 7
#define WW_WITCH_HH 8
#define WW_WITCH_START_HEARTS 3
#define WW_WITCH_MAX_HEARTS 5

// What the magic flowers do
#define WW_EFFECT_HEART 0
#define WW_EFFECT_SPEEDY 1
#define WW_EFFECT_HUSH 2
#define WW_EFFECT_MUDDLE 3
#define WW_EFFECTS 4

// Which way she is shown
#define WW_VIEW_FRONT 0
#define WW_VIEW_BACK 1
#define WW_VIEW_RIGHT 2
#define WW_VIEW_LEFT 3
#define WW_VIEWS 4
#define WW_WITCH_STEPS 3

void ww_witch_init(void);

// Puts her down with her hearts back, for a new life
void ww_witch_reset(int32_t x, int32_t y);

// Returns the direction she has walked out of the room by, or -1
int32_t ww_witch_update(uint8_t input, uint8_t pressed, uint32_t frame);
// Moves her to the other side of the next room, having left by dir
void ww_witch_enter(int32_t dir);
// Where she came in to the room
void ww_witch_entry(int32_t* x, int32_t* y);

// Pixel position of her feet in the room
int32_t ww_witch_x(void);
int32_t ww_witch_y(void);
// 0 to 7, clockwise from facing right
uint32_t ww_witch_facing(void);
void ww_witch_facing_step(uint32_t facing, int32_t* dx, int32_t* dy);

// False while she is dizzy from losing all her hearts
bool ww_witch_alive(void);
// True once she has finished being dizzy
bool ww_witch_lost(void);
// Tiptoeing or hushed, so sleeping animals stay asleep
bool ww_witch_quiet(void);
// Hushed by a flower, so grumpy animals do not notice her
bool ww_witch_hushed(void);
bool ww_witch_overlaps(int32_t x0, int32_t y0, int32_t x1, int32_t y1);
// Bumped by something at x, y. Returns false if she could not be hurt just then.
bool ww_witch_hurt(int32_t x, int32_t y);

uint32_t ww_witch_hearts(void);
void ww_witch_effect(uint32_t effect);
// What a flower is doing to her, or NULL
const char* ww_witch_status(uint32_t* colour);

// The spell she has cast, if there is one in flight
bool ww_witch_bolt(int32_t* x, int32_t* y);
void ww_witch_bolt_end(void);

void ww_witch_draw(uint32_t frame);
void ww_witch_draw_bolt(uint32_t frame);
// For outside of the game, sx and sy being the top left on the screen
void ww_witch_draw_at(int32_t sx, int32_t sy, uint32_t view, uint32_t step);

#ifdef __cplusplus
}
#endif
