// Copyright (c) 2024 fruit-bat.

// The eight galaxies of Elite, each of 256 systems, made from three numbers
// as the original game did.
#pragma once

#include <stddef.h>
#include <stdint.h>

#define GALAXIES 8
#define GALAXY_SYSTEMS 256
// The longest name, 8 letters, and the end
#define GALAXY_NAME_MAX 9

typedef struct {
	uint8_t x, y;        // where it is on the chart, from 0 to 255 each way
	uint8_t economy;     // 0 rich industrial up to 7 poor agricultural
	uint8_t government;  // 0 anarchy up to 7 corporate state
	uint8_t tech;        // the tech level, from 1
	uint8_t population;  // in hundreds of millions
	uint16_t radius;     // of the planet, in km
	char name[GALAXY_NAME_MAX];
	uint16_t seed[3];    // the numbers the system was made from, for the rest of its data
} GalaxySystem_t;

extern const char* const galaxy_economy_names[8];
extern const char* const galaxy_government_names[8];

// Makes the systems of galaxy g, from 0
void galaxy_make(uint32_t g, GalaxySystem_t systems[GALAXY_SYSTEMS]);

// The distance from a to b in tenths of a light year
uint32_t galaxy_distance(const GalaxySystem_t* a, const GalaxySystem_t* b);

// Who lives there, such as "Human Colonials" or "Large Green Slimy Frogs"
void galaxy_species(const GalaxySystem_t* sys, char* out, size_t n);

// What the system is known for, as the original game described it
void galaxy_description(const GalaxySystem_t* sys, char* out, size_t n);
