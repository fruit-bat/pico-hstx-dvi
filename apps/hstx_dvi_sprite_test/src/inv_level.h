#pragma once

#include "pico/stdlib.h"

#ifdef __cplusplus
extern "C" {
#endif

// The current wave and the difficulty settings that go with it

void inv_level_reset(void);
void inv_level_next(void);
uint32_t inv_level_get(void);

// Top of the invader block at the start of the wave
int32_t inv_level_invader_top(void);
// Base invader march speed in 1/16 pixel per frame
uint32_t inv_level_invader_speed(void);
// Most bombs allowed in flight
uint32_t inv_level_bomb_max(void);
// Average frames between bombs
uint32_t inv_level_bomb_interval(void);
// Pixels per frame
int32_t inv_level_bomb_speed(void);
// Chance out of 256 that a bomb is dropped from the column nearest the gun
uint32_t inv_level_aim_chance(void);
// Frames until the next mystery ship
uint32_t inv_level_mot_interval(void);

#ifdef __cplusplus
}
#endif
