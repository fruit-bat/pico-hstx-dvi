// Copyright (c) 2024 fruit-bat.

// The galaxies are made as in Ian Bell's Text Elite, which follows the
// original game: three 16 bit numbers, the seed, are turned like a Fibonacci
// sequence to give each system in turn, and twisted to give the next galaxy.

#include "galaxy.h"
#include <math.h>
#include <stdbool.h>

typedef struct {
	uint16_t w0, w1, w2;
} Seed_t;

// The seed of the first galaxy
static const Seed_t _first = { 0x5A4A, 0x0248, 0xB753 };

// The pairs of letters the names are made of. A dot is left out.
static const char _pairs[] = "..LEXEGEZACEBISOUSESARMAINDIREA.ERATENBERALAVETIEDORQUANTEISRION";

static void tweak(Seed_t* s) {
	const uint16_t t = (uint16_t)(s->w0 + s->w1 + s->w2);
	s->w0 = s->w1;
	s->w1 = s->w2;
	s->w2 = t;
}

// Turns each byte left one bit, for the next galaxy
static uint16_t twist(uint16_t x) {
	const uint32_t h = x >> 8, l = x & 0xff;
	return (uint16_t)((((h << 1) | (h >> 7)) & 0xff) << 8 | (((l << 1) | (l >> 7)) & 0xff));
}

static void make_system(Seed_t* s, GalaxySystem_t* sys) {
	sys->x = (uint8_t)(s->w1 >> 8);
	sys->y = (uint8_t)(s->w0 >> 8);
	sys->government = (s->w1 >> 3) & 7;
	sys->economy = (s->w0 >> 8) & 7;
	if (sys->government <= 1) sys->economy |= 2;
	uint32_t tech = ((s->w1 >> 8) & 3) + (sys->economy ^ 7) + (sys->government >> 1);
	if (sys->government & 1) ++tech;
	sys->tech = (uint8_t)(tech + 1);
	sys->population = (uint8_t)(4 * tech + sys->economy + sys->government + 1);
	sys->radius = (uint16_t)(256 * (((s->w2 >> 8) & 15) + 11) + sys->x);

	// Four pairs of letters, or three for a short name
	const bool long_name = s->w0 & 64;
	uint32_t n = 0;
	for (uint32_t i = 0; i < 4; ++i) {
		const uint32_t p = 2 * ((s->w2 >> 8) & 31);
		tweak(s);
		if (i == 3 && !long_name) continue;
		for (uint32_t j = 0; j < 2; ++j) {
			if (_pairs[p + j] != '.') sys->name[n++] = _pairs[p + j];
		}
	}
	sys->name[n] = 0;
}

void galaxy_make(uint32_t g, GalaxySystem_t systems[GALAXY_SYSTEMS]) {
	Seed_t s = _first;
	for (uint32_t i = 0; i < g % GALAXIES; ++i) {
		s.w0 = twist(s.w0);
		s.w1 = twist(s.w1);
		s.w2 = twist(s.w2);
	}
	for (uint32_t i = 0; i < GALAXY_SYSTEMS; ++i) make_system(&s, &systems[i]);
}

uint32_t galaxy_distance(const GalaxySystem_t* a, const GalaxySystem_t* b) {
	// The chart is twice as tall as it is shown
	const float dx = (float)a->x - (float)b->x;
	const float dy = ((float)a->y - (float)b->y) * 0.5f;
	return (uint32_t)(4.0f * sqrtf(dx * dx + dy * dy));
}
