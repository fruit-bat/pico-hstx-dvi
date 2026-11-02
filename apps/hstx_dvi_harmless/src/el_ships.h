// Copyright (c) 2024 fruit-bat.

// The other ships in a system: traders going about their business, pirates
// that come for us, and the police, who come for us if we are an offender.
// With the explosions of those destroyed, the cargo canisters some leave
// behind, and the missiles. Now and then a Thargoid, and in witchspace,
// between systems, nothing but Thargoids, each launching Thargons, little
// drones that go dead when it is destroyed.
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
	ROLE_POLICE,
	ROLE_THARGOID,
	ROLE_THARGON
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
	uint32_t missiles; // or for a Thargoid, Thargons
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
	int32_t mother;    // for a Thargon, the Thargoid that launched it, or -1
	uint32_t wanted;   // its bounty, in percent of its class's, as some are wanted more
	// A ship in a story: its id there, from 1, or 0, what it is called, how
	// it goes, whether it can be disabled and has been, whether it has come
	// near us, and the named item it leaves, or 0
	uint8_t story;
	uint8_t goal;
	bool can_disable, disabled, near;
	uint8_t drop;
	const char* label;
} Ship_t;

#define SHIPS 6
extern Ship_t ships[SHIPS];

// A cargo canister, drifting and tumbling where a ship was destroyed
typedef struct {
	Frame_t fr;
	Vec3_t drift;
	float roll, pitch;
	bool alien;       // alien items, from a Thargon
	bool on;
	uint8_t item;     // a story's named item, or 0
} Canister_t;

#define CANISTERS 4
extern Canister_t canisters[CANISTERS];

// What the ships need to know about the system and us
typedef struct {
	const Frame_t* me;
	Vec3_t station;
	uint32_t government; // 0 anarchy up to 7 corporate state
	uint32_t economy;    // 0 rich industrial up to 7 poor agricultural
	uint32_t population; // in hundreds of millions
	bool offender;       // whether the police are after us
	bool scoops;         // whether we have fuel scoops, to pick up canisters
	bool witch;          // whether we are in witchspace
	Vec3_t velocity;     // ours, in units a frame
	Vec3_t planet;       // where the planet is, for racing to
	float planet_radius;
	bool aliens_friendly; // Thargoids leave us be, unless shot
	bool pirates_friendly;
} ShipsWorld_t;

// What happened to us and the ships this frame
typedef struct {
	float fore, aft;  // damage to our shields, from in front and behind
	uint32_t kills;   // ships we destroyed
	uint32_t bounty;  // for the pirates among them, in tenths of a credit
	bool innocent;    // whether any was not a pirate or a Thargoid
	bool missile;     // whether a missile was fired at us
	uint32_t scooped; // canisters we picked up
	uint32_t aliens;  // of those, alien items
	uint32_t smashed; // canisters that broke against us
	// For the stories: the story ships disabled, destroyed, got where they
	// were going, or came near, a bit for each id from 1, and named items scooped
	uint8_t disabled, destroyed, escaped, near;
	uint8_t items[2];
	uint32_t items_n;
	bool rammed;      // a ship ran into us
} ShipsEvents_t;

// New ships for a new system, or Thargoids round us in witchspace
void ships_enter(const ShipsWorld_t* w);
void ships_move(const ShipsWorld_t* w, ShipsEvents_t* ev);
// The ship in our sights within range, or -1
int32_t ships_in_sights(const Frame_t* me, float range);
// The ship a targeting computer follows: the one nearest the middle of the
// view, within range and cone of it (as far off as across for each ahead),
// keeping to the one it followed, keep, while it can; or -1. The station is
// never one, as it isn't a ship here.
int32_t ships_tracked(const Frame_t* me, float range, float cone, int32_t keep);
// A story's ship, id from 1, with a label for the targeting computer, of the
// class named, going as StoryGoal_e goal, at pos facing f; it replaces one
// of the same id
void ships_story_spawn(uint8_t id, const char* label, const char* cls, uint8_t goal, bool can_disable, uint8_t drop,
	Vec3_t pos, Vec3_t f, Vec3_t u, const ShipsWorld_t* w);
// The story ships stop fighting
void ships_story_calm(void);
// The Thargoids stop fighting, unless shot
void ships_calm_aliens(void);
// A canister holding a story's named item, at pos
void ships_story_canister(uint8_t item, Vec3_t pos);
// A Viper launched from the station's slot, at frame from, after us; false if
// there is no room for one
bool ships_launch_police(const Frame_t* from, const ShipsWorld_t* w);
// The ship nearest the way we point, for a missile, or -1
int32_t ships_nearest_ahead(const Frame_t* me);
// How much of a laser's damage reaches d away, of the range it reaches: all of
// it close in, falling to a little at the end of the range
float ships_laser_falloff(float d, float range);
// The bounty on a ship, in tenths of a credit: on pirates and aliens, and
// none on the rest
uint32_t ships_bounty(uint32_t i);
// Damages a ship we have hit, destroying it if it has no energy left
void ships_hit(uint32_t i, float damage, ShipsEvents_t* ev);
// Fires a missile from us at a ship, returning false if one cannot be fired
bool ships_fire_missile(const Frame_t* me, uint32_t target);
// Our ECM: blows up every missile flying
void ships_ecm(void);
// The energy bomb: every ship within range of me destroyed, by us, and the missiles flying
void ships_bomb(Vec3_t me, float range, ShipsEvents_t* ev);
// Whether a hostile ship is on the scanner, or a missile is coming for us
bool ships_hostile_near(const Frame_t* me, float range);
// For trying things on the PC: puts a ship of the named type at pos, facing
// f, coming for us, or a canister at pos
void ships_debug_place(uint32_t i, const char* name, Vec3_t pos, Vec3_t f, Vec3_t u, const ShipsWorld_t* w);
void ships_debug_canister(Vec3_t pos);
// An explosion, as when a ship is destroyed
void ships_explode(Vec3_t pos, float size);
// The model of a ship, by its name, or a Cobra Mk III's
const WireModel_t* ships_model(const char* name);
// Explosions, lasers fired at us and missiles, before the ships so they are over them
void ships_draw_effects(const Frame_t* me, Style_e style, uint32_t frame);

#ifdef __cplusplus
}
#endif
