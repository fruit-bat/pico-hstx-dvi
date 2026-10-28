#pragma once

#include "pico/stdlib.h"
#include "hstx_dvi_sprite.h"

#ifdef __cplusplus
extern "C" {
#endif

SpriteId inv_bullets_init(SpriteId start);

void inv_bullets_update(void);

void inv_bullets_fire(SpriteId gunSpriteId);

void inv_bullets_clear(void);

// Shots fired since inv_bullets_init
uint32_t inv_bullets_get_shots(void);

#ifdef __cplusplus
}
#endif

