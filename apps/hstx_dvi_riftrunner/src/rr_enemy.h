// Copyright (c) 2024 fruit-bat.

// The enemies: fliers that come in waves, things on the ground, meteors, the
// capsules some leave behind, and the bullets they all fire. Positions are in
// the world, fixed point, as rr_world, unless said to be in pixels.
#pragma once

#include "pico/stdlib.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	RR_EN_DART = 0, // in waves; a whole wave shot leaves a capsule
	RR_EN_CARRIER,  // leaves a capsule
	RR_EN_SAUCER,   // comes after the ship and fires
	RR_EN_METEOR,   // takes a lot, and breaks up
	RR_EN_ROCK,     // a piece of meteor
	RR_EN_ROCKET,   // on the ground, taking off as the ship comes
	RR_EN_FUEL,     // on the ground, fuel when hit
	RR_EN_TURRET,   // on the ground, firing at the ship
	RR_EN_TURRET_C, // on the ceiling
	RR_EN_CAPSULE,  // picked up, lights the next power up on the bar
	RR_EN_TYPES
} RrEnemy_e;

// How fliers move
typedef enum {
	RR_PAT_SINE = 0, // in a wave, across the screen
	RR_PAT_HAIRPIN,  // in, then back out towards the ship
} RrPattern_e;

void rr_enemy_init(void);
// Harder each time round
void rr_enemy_set_loop(uint32_t loop);

// Offered a level place on the floor or ceiling, a world x and screen y in
// pixels, as the land is made. Returns true if something was put there.
bool rr_enemy_place(int32_t px, int32_t y, bool ceiling, uint32_t section);
// Fliers, coming in from the right at screen y in pixels
void rr_enemy_wave(uint32_t type, uint32_t count, int32_t y, uint32_t pattern);
void rr_enemy_spawn(uint32_t type, int32_t px, int32_t y);

void rr_enemy_update(uint32_t frame);
void rr_enemy_draw(uint32_t frame);
void rr_enemy_draw_bullets(uint32_t frame);

void rr_enemy_fire(int32_t x, int32_t y, int32_t vx, int32_t vy);
// At the ship, speed in fixed point pixels a frame
void rr_enemy_fire_at(int32_t x, int32_t y, int32_t speed);
void rr_enemy_clear_bullets(void);

// The player's weapons: a box, world x and screen y in pixels. A shot that
// stops at the first thing it hits has a pierce of 0; one that goes through
// has its own pierce, so each thing is hurt by it once. Returns true if it hit.
bool rr_enemy_shot(int32_t x, int32_t y, int32_t w, int32_t h, int32_t damage, uint32_t pierce);
// Hurts everything within r pixels of a world x and screen y in pixels
void rr_enemy_blast(int32_t x, int32_t y, int32_t r, int32_t damage);

// For the demo: the nearest flier in front of the ship, things on the ground
// ahead, bullets or fliers about to hit it, all in pixels
bool rr_enemy_target(int32_t px, int32_t py, int32_t* tx, int32_t* ty);
bool rr_enemy_ground_ahead(int32_t px, int32_t range, int32_t* tx, int32_t* ty);
bool rr_enemy_danger_near(int32_t px, int32_t py, int32_t r, int32_t* bx, int32_t* by);
bool rr_enemy_capsule(int32_t* tx, int32_t* ty);
uint32_t rr_enemy_fliers(void);
// All of them, wherever they are
uint32_t rr_enemy_count(void);

// For the title screen's roll call
const char* rr_enemy_name(uint32_t type);
uint32_t rr_enemy_score(uint32_t type);
int32_t rr_enemy_width(uint32_t type);
int32_t rr_enemy_height(uint32_t type);
void rr_enemy_draw_icon(uint32_t type, int32_t sx, int32_t sy, uint32_t frame);

#ifdef __cplusplus
}
#endif
