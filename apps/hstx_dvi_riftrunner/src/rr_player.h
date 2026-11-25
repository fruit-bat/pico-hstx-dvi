// Copyright (c) 2024 fruit-bat.

// The player's ship: flying, shooting, the charged beam, bombs, fuel, and the
// power ups picked from the power bar
#pragma once

#include "pico/stdlib.h"

#ifdef __cplusplus
extern "C" {
#endif

// On the screen
#define RR_SHIP_W 64
#define RR_SHIP_H 28
// Where the shots come out, from the top left
#define RR_SHIP_NOSE_X 60
#define RR_SHIP_NOSE_Y 16

// Fuel, in frames of flying
#define RR_FUEL_MAX 3600
#define RR_FUEL_TANK 1000
#define RR_FUEL_LOW (RR_FUEL_MAX / 5)

// The beam is ready to let go after this many frames held, and full after
#define RR_BEAM_READY 24
#define RR_BEAM_FULL 96

typedef enum {
	RR_POW_SPEED = 0,
	RR_POW_BOMB,
	RR_POW_TWIN,
	RR_POW_LASER,
	RR_POW_DRONE,
	RR_POW_ORB,
	RR_POWERS
} RrPower_e;

void rr_player_init(void);
void rr_player_update(uint32_t frame, uint16_t input, uint16_t pressed);
// The ship and what goes with it, then its shots, which go behind the enemies
void rr_player_draw(uint32_t frame);
void rr_player_draw_shots(uint32_t frame);
// A beam len long and h high, its left end at x and middle at y on the screen
void rr_player_draw_beam(int32_t x, int32_t y, int32_t len, int32_t h, uint32_t frame);
// How far a beam goes in a frame, and how long it is
#define RR_BEAM_SPEED 26
#define RR_BEAM_LEN 200

bool rr_player_alive(void);
// Out of lives, and done blowing up
bool rr_player_over(void);
// The top left of the ship, the world x in pixels and the screen y
int32_t rr_player_px(void);
int32_t rr_player_py(void);

// Whether a box, a world x and screen y in pixels, hits the ship, blowing it up if so
bool rr_player_hit(int32_t x, int32_t y, int32_t w, int32_t h);
// Whether a box touches the ship, without hurting it, for picking things up
bool rr_player_touch(int32_t x, int32_t y, int32_t w, int32_t h);
// Whether a box hits the orb, if there is one
bool rr_player_orb(int32_t x, int32_t y, int32_t w, int32_t h);
void rr_player_kill(void);

void rr_player_add_capsule(void);
void rr_player_add_fuel(int32_t fuel);
// Fuel only burns while there is land below
void rr_player_set_burning(bool burning);
int32_t rr_player_fuel(void);
// The power lit on the bar, or -1
int32_t rr_player_power_cursor(void);
// Whether a power can be taken any more, and how far it goes
bool rr_player_power_full(uint32_t power);
uint32_t rr_player_power_max(uint32_t power);
uint32_t rr_player_power_level(uint32_t power);
// How far the beam is charged, 0 to RR_BEAM_FULL
uint32_t rr_player_beam(void);

// Clears the shots, between sections
void rr_player_clear_shots(void);

#ifdef __cplusplus
}
#endif
