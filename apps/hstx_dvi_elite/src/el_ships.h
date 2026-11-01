// Copyright (c) 2024 fruit-bat.

// The other ships in a system: traders going about their business, pirates
// that come for us, and the police, who come for us if we are an offender.
// With the explosions of those destroyed, and the missiles.
//
// Each type of ship has a character of its own: light fighters are fast and
// hard to hit but weak, the big ships slow but deadly head on. Each system
// has a character too, from its data: how many pirates from its government,
// how busy it is from its population, and what the traders fly from its economy.
#pragma once

#include "el_space.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	ROLE_TRADER = 0,
	ROLE_PIRATE,
	ROLE_POLICE
} Role_e;

// How a type of ship flies and fights
typedef struct {
	const char* name;
	float speed;     // top speed, in units a frame
	float turn;      // how quickly it turns its nose
	float hull;      // how many pulses of our laser it takes
	float damage;    // a hit from its laser, off our shields, which are 1
	uint32_t rate;   // frames between shots
	uint32_t aim;    // the chance a shot hits, in percent
	float range;     // how far its laser reaches
	float close;     // how close it comes before breaking off
	bool jinks;      // weaves about as it comes in
	uint32_t missiles;
	uint32_t bounty; // in tenths of a credit
} ShipClass_t;

typedef struct {
	const ShipClass_t* cls;
	const WireModel_t* model;
	Frame_t fr;
	float speed;
	float roll, pitch; // how a trader turns as it goes
	Role_e role;
	bool alive;
	bool hostile;
	float energy;
	uint32_t missiles;
	uint32_t cooldown; // frames until it can fire again
	uint32_t beam;     // frames its laser is shown for
	uint32_t retreat;  // frames it turns away for, having come too close
	uint32_t respawn;  // frames until another takes its place, once destroyed
} Ship_t;

#define SHIPS 6
extern Ship_t ships[SHIPS];

// What the ships need to know about the system and us
typedef struct {
	const Frame_t* me;
	Vec3_t station;
	uint32_t government; // 0 anarchy up to 7 corporate state
	uint32_t economy;    // 0 rich industrial up to 7 poor agricultural
	uint32_t population; // in hundreds of millions
	bool offender;       // whether the police are after us
} ShipsWorld_t;

// What happened to us and the ships this frame
typedef struct {
	float fore, aft;  // damage to our shields, from in front and behind
	uint32_t kills;   // ships we destroyed
	uint32_t bounty;  // for the pirates among them, in tenths of a credit
	bool innocent;    // whether any was not a pirate
	bool missile;     // whether a missile was fired at us
} ShipsEvents_t;

// New ships for a new system
void ships_enter(const ShipsWorld_t* w);
void ships_move(const ShipsWorld_t* w, ShipsEvents_t* ev);
// The ship in our sights within range, or -1
int32_t ships_in_sights(const Frame_t* me, float range);
// The ship nearest the way we point, for a missile, or -1
int32_t ships_nearest_ahead(const Frame_t* me);
// Damages a ship we have hit, destroying it if it has no energy left
void ships_hit(uint32_t i, float damage, ShipsEvents_t* ev);
// Fires a missile from us at a ship, returning false if one cannot be fired
bool ships_fire_missile(const Frame_t* me, uint32_t target);
// Our ECM: blows up every missile flying
void ships_ecm(void);
// Whether a hostile ship is on the scanner, or a missile is coming for us
bool ships_hostile_near(const Frame_t* me, float range);
// For trying things on the PC: puts a ship of the named type at pos, facing
// f, coming for us
void ships_debug_place(uint32_t i, const char* name, Vec3_t pos, Vec3_t f, Vec3_t u, const ShipsWorld_t* w);
// Explosions, lasers fired at us and missiles, before the ships so they are over them
void ships_draw_effects(const Frame_t* me, Style_e style, uint32_t frame);

#ifdef __cplusplus
}
#endif
