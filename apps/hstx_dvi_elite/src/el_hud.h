// Copyright (c) 2024 fruit-bat.

// The dashboard under the view, after the original game: gauges either side
// of the scanner, which shows where the ships are round us, and a compass.
#pragma once

#include "el_draw.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	// From 0 to 1
	float fore_shield, aft_shield, fuel, cabin_temp, laser_temp, altitude;
	float speed;
	float overdrive; // how far into it, shown in red at the top of the speed
	float hyperdrive; // how near it is to engaging, in cyan at the very end
	float energy; // across the four banks, which empty from the top
	// From -1 to 1
	float roll, pitch;
	uint32_t missiles; // how many are left, of four
	// The way to what the compass points at, in the view
	Vec3_t compass;
	// How far the scanner sees
	float scanner_range;
	bool sights;
	// How far the sights are off the middle, following a ship, when locked,
	// or sliding over to it while locking on
	Vec2_t sights_off;
	bool locked, locking;
} Hud_t;

// Something on the scanner, where it is in the view
typedef struct {
	Vec3_t pos;
	const uint8_t* rgb;
	bool big;
} Blip_t;

void hud_init(void);
void hud_draw(const Hud_t* hud, const Blip_t* blips, uint32_t n, bool filled);

#ifdef __cplusplus
}
#endif
