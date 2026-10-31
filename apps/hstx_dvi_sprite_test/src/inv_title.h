#pragma once

#include "pico/stdlib.h"
#include "hstx_dvi_sprite.h"

#ifdef __cplusplus
extern "C" {
#endif

// The title, spelt out in little invaders
SpriteId inv_title_init(SpriteId start);

// Flies the invaders in, waves them about and flies them out again by the end frame
void inv_title_update(uint32_t frame, uint32_t end);

#ifdef __cplusplus
}
#endif
