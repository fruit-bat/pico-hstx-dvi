// Copyright (c) 2024 fruit-bat.

// Trading: our ship and what it carries, the market at the station, made
// as in Ian Bell's Text Elite, and the equipment for sale.
#pragma once

#include "galaxy.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define GOODS 17

typedef enum {
	LASER_PULSE = 0,
	LASER_BEAM,
	LASER_MILITARY
} Laser_e;

// Us: what we have, and what our ship carries
typedef struct {
	uint32_t cash;       // in tenths of a credit
	uint32_t fuel;       // in tenths of a light year
	uint32_t missiles;
	uint32_t hold;       // how many tonnes the hold takes
	uint32_t cargo[GOODS];
	bool ecm;
	Laser_e laser;
} Commander_t;

extern Commander_t cmdr;

#define MISSILES_MAX 4

// As Commander Jameson starts
void trade_new_commander(void);
// The market at a system we have just come to, which changes a little each time
void trade_arrive(const GalaxySystem_t* sys);
// The market and the equipment for sale, when docked. Up and down pick,
// right buys and left sells in the market, and fire buys equipment.
// Return a message to show, or NULL.
const char* trade_market_input(uint8_t pressed, uint8_t held);
const char* trade_equipment_input(uint8_t pressed, const GalaxySystem_t* sys);
void trade_draw_market(void);
void trade_draw_equipment(const GalaxySystem_t* sys);
// What our ship has fitted, for the status
uint32_t trade_equipment_names(const char** names, uint32_t max);

#ifdef __cplusplus
}
#endif
