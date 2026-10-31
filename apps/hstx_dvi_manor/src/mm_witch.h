#pragma once

#include "pico/stdlib.h"
#include "mm_body.h"

#ifdef __cplusplus
extern "C" {
#endif

// Hazel, who runs, jumps and casts spells
#define MM_WITCH_HW 18
#define MM_WITCH_H 56
#define MM_WITCH_HEARTS 3
#define MM_WITCH_MAX_HEARTS 5

void mm_witch_init(void);
// Starts a night, standing on the tile below cx, cy
void mm_witch_reset(int32_t cx, int32_t cy);
// Back at the last lantern after losing a life
void mm_witch_respawn(void);

void mm_witch_update(uint8_t input, uint32_t frame);
void mm_witch_draw(uint32_t frame);
// Stands facing the front, cheering
void mm_witch_set_cheer(bool cheer);

// Middle and bottom, in pixels
int32_t mm_witch_x(void);
int32_t mm_witch_y(void);
int32_t mm_witch_facing(void);
bool mm_witch_grounded(void);
// Up and down speed, fixed point, negative going up
int32_t mm_witch_vy(void);
bool mm_witch_casting(void);
void mm_witch_box(int32_t* x0, int32_t* y0, int32_t* x1, int32_t* y1);
// Whether Hazel is standing on a top at pixel row y, between x0 and x1
bool mm_witch_standing_on(int32_t x0, int32_t x1, int32_t y);

// Bumped by a grumpy creature at x
void mm_witch_hurt(int32_t from_x);
// Returns false if her hearts were already full
bool mm_witch_heal(void);
uint32_t mm_witch_hearts(void);
// Out of hearts, and finished being dizzy
bool mm_witch_fainted(void);

void mm_witch_add_key(uint32_t group);
uint32_t mm_witch_keys(void);
void mm_witch_learn_float(void);
bool mm_witch_can_float(void);

#ifdef __cplusplus
}
#endif
