#pragma once

#include "pico/stdlib.h"
#include "hstx_dvi_sprite.h"

#ifdef __cplusplus
extern "C" {
#endif

SpriteId inv_base_init(SpriteId start);

void inv_base_update(void);

void inv_base_bomb_hit(SpriteId spriteId, SpriteCollisionMask m);
void inv_base_bullet_hit(SpriteId spriteId, SpriteCollisionMask m);
// An invader at x, y, w by h pixels, walking through the bases
void inv_base_invader_hit(int32_t x, int32_t y, int32_t w, int32_t h, SpriteCollisionMask m);

#ifdef __cplusplus
}
#endif

