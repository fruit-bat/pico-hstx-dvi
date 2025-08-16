#pragma once

#include "pico/stdlib.h"
#include "hstx_dvi_sprite.h"

#ifdef __cplusplus
extern "C" {
#endif

SpriteId inv_base_init(SpriteId start);

void inv_base_update(void);

void inv_base_bomb_hit(SpriteId spriteId, SpriteCollisionMask m);

#ifdef __cplusplus
} 
#endif

