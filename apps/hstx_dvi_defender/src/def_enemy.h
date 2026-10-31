#pragma once

#include "pico/stdlib.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	DEF_ENEMY_NONE = 0,
	DEF_ENEMY_LANDER,
	DEF_ENEMY_MUTANT,
	DEF_ENEMY_BOMBER,
	DEF_ENEMY_POD,
	DEF_ENEMY_SWARMER,
	DEF_ENEMY_BAITER,
	DEF_ENEMY_TYPE_COUNT
} DefEnemyType_e;

// Clears out all the enemies and their bullets
void def_enemy_init(void);
void def_enemy_set_wave(uint32_t wave);

// x is in world fixed point, y in screen fixed point. Returns false if there was no room.
bool def_enemy_spawn(uint32_t type, int32_t x, int32_t y);

// When frozen nothing moves, while the player is being replaced
void def_enemy_update(uint32_t frame, bool frozen);

void def_enemy_draw(uint32_t frame);
void def_enemy_draw_bullets(uint32_t frame);
void def_enemy_scan(void);

// Everything that has to be shot to finish the wave
uint32_t def_enemy_count_hostile(void);
uint32_t def_enemy_count(uint32_t type);
uint32_t def_enemy_on_screen(void);

void def_enemy_smart_bomb(void);
void def_enemy_clear_bullets(void);
void def_enemy_remove(uint32_t type);
// Turns all the landers into mutants, for when the planet has gone
void def_enemy_mutate_all(void);

// For the demo player, the middle of the nearest enemy and any bullet close by
bool def_enemy_nearest(int32_t x, int32_t* ex, int32_t* ey);
bool def_enemy_bullet_near(int32_t x, int32_t y, int32_t range, int32_t* by);

// For the attract screens
void def_enemy_draw_icon(uint32_t type, int32_t sx, int32_t sy, uint32_t frame);
uint32_t def_enemy_width(uint32_t type);
uint32_t def_enemy_height(uint32_t type);
const char* def_enemy_name(uint32_t type);
uint32_t def_enemy_score(uint32_t type);
uint32_t def_enemy_colour(uint32_t type);

#ifdef __cplusplus
}
#endif
