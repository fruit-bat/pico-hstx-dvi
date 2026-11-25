// Copyright (c) 2024 fruit-bat.

// The warship at the end of the sections: its core sits in its open jaws
// behind three walls, which have to be shot away first, while turrets above
// and below fire at the ship.
#pragma once

#include "pico/stdlib.h"

#ifdef __cplusplus
extern "C" {
#endif

void rr_boss_init(void);
void rr_boss_start(uint32_t loop);
// Coming in, fighting or blowing up
bool rr_boss_active(void);
// Blown up and gone
bool rr_boss_done(void);
void rr_boss_update(uint32_t frame);
void rr_boss_draw(uint32_t frame);

// As rr_enemy_shot and rr_enemy_blast
bool rr_boss_shot(int32_t x, int32_t y, int32_t w, int32_t h, int32_t damage, uint32_t pierce);
void rr_boss_blast(int32_t x, int32_t y, int32_t r, int32_t damage);

// For the demo: what to shoot at, in pixels
bool rr_boss_target(int32_t* tx, int32_t* ty);

#ifdef __cplusplus
}
#endif
