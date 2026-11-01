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
	LASER_MILITARY,
	LASER_NONE
} Laser_e;

// Where a laser is fitted, which is also the way we look to fire it
typedef enum {
	MOUNT_FRONT = 0,
	MOUNT_REAR,
	MOUNT_LEFT,
	MOUNT_RIGHT,
	MOUNTS
} Mount_e;

// Us: what we have, and what our ship carries. It is saved as it is, so
// what is added goes on the end, and saves made before it take what a new
// commander has.
typedef struct {
	uint32_t cash;       // in tenths of a credit
	uint32_t fuel;       // in tenths of a light year
	uint32_t missiles;
	uint32_t hold;       // how many tonnes the hold takes
	uint32_t cargo[GOODS];
	bool ecm;
	Laser_e laser[MOUNTS]; // the front first, where it was when there was only one
	bool scoops;
	bool docking;        // whether we have a docking computer
	bool pod;            // whether we have an escape pod
	bool bomb;           // whether we have an energy bomb
} Commander_t;

extern Commander_t cmdr;

#define MISSILES_MAX 4

// As Commander Jameson starts
void trade_new_commander(void);
// What a new commander has
void trade_commander_start(Commander_t* c);
// The market at a system we have just come to, which changes a little each time
void trade_arrive(const GalaxySystem_t* sys);
// The market and the equipment for sale, when docked. Up and down pick,
// right buys and left sells in the market, and fire buys equipment.
// Return a message to show, or NULL.
const char* trade_market_input(uint8_t pressed, uint8_t held);
const char* trade_equipment_input(uint8_t pressed, const GalaxySystem_t* sys);
void trade_draw_market(void);
void trade_draw_equipment(const GalaxySystem_t* sys);
// A canister picked up: a good at random into the hold, returning its name,
// or that the hold is full
const char* trade_scoop(void);
// What is in the hold, with the fuel and cash
void trade_draw_inventory(void);
// Escaping in the pod: the cargo goes with the ship, and so does the pod
void trade_escape(void);
// Everything fitted, the best lasers all round, and full of fuel and
// missiles, for trying things out
void trade_max_equipment(void);
// What our ship has fitted, for the status
uint32_t trade_equipment_names(const char** names, uint32_t max);

#ifdef __cplusplus
}
#endif
