#pragma once

#include "pico/stdlib.h"
#include "hstx_dvi_sprite.h"

#ifdef __cplusplus
extern "C" {
#endif

#define INV_MOT_COLLISION_MASK     ((SpriteCollisionMask)0x01)
#define INV_INVADER_COLLISION_MASK ((SpriteCollisionMask)0x02)
#define INV_GUN_COLLISION_MASK     ((SpriteCollisionMask)0x04)
#define INV_BULLET_COLLISION_MASK  ((SpriteCollisionMask)0x08)
#define INV_BOMB_COLLISION_MASK    ((SpriteCollisionMask)0x10)
#define INV_BASE_COLLISION_MASK    ((SpriteCollisionMask)0x100)

#ifdef __cplusplus
} 
#endif

