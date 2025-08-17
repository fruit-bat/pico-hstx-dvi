#pragma once

#include "pico/stdlib.h"
#include "hstx_dvi_sprite.h"

#ifdef __cplusplus
extern "C" {
#endif

#define INV_MOT_COLLISION_MASK     ((SpriteCollisionMask)0x0001)
#define INV_INVADER_COLLISION_MASK ((SpriteCollisionMask)0x0002)
#define INV_GUN_COLLISION_MASK     ((SpriteCollisionMask)0x0004)
#define INV_BULLET_COLLISION_MASK  ((SpriteCollisionMask)0x0008)
#define INV_BOMB_COLLISION_MASK    ((SpriteCollisionMask)0x0010)
#define INV_BASE_COLLISION_MASK    ((SpriteCollisionMask)0x0100)
#define INV_BASE_COLLISION_MASKS   ((SpriteCollisionMask)0x0f00)

#ifdef __cplusplus
}
#endif

