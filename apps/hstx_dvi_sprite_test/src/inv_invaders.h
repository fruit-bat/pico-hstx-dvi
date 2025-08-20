#pragma once

#include "pico/stdlib.h"
#include "hstx_dvi_sprite.h"

#ifdef __cplusplus
extern "C" {
#endif

SpriteId inv_invaders_init(SpriteId start);

void inv_invader_update(uint32_t frame);

#ifdef __cplusplus
}
#endif

