// Copyright (c) 2024 fruit-bat.

// The companion who may fly with us, and the pet. A companion has a skill,
// likes and dislikes, and something to say about how we fly. What happens
// changes their mood, which settles back with time; their mood when we dock,
// and what we choose in the stories, changes their loyalty. With too little
// of it they leave us, at the next station.
//
// They only talk now and then, not over a fight unless it matters, and not
// the same thing twice in a while. Their state is in the story save.
#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	COMPANION_NONE = 0,
	COMPANION_PIP,   // a nervous navigator: warns of pirates sooner
	COMPANION_MARGE, // the ship's engineer: patches the shields once a fight
	COMPANION_SLY,   // a retired smuggler: sells for a little more
	COMPANION_QUILL, // a xenobiologist: talks to Thargoids, in the stories
	COMPANIONS
} Companion_e;

typedef enum {
	PET_NONE = 0,
	PET_KITTEN,
	PET_PARROT,
	PETS
} Pet_e;

// What a companion has something to say about
typedef enum {
	CE_LAUNCH = 0,
	CE_DOCK,          // by hand
	CE_DOCK_COMPUTER, // by the docking computer
	CE_HYPERSPACE,
	CE_WITCH,
	CE_RAMMED,
	CE_HULL_HIT,
	CE_SHIELDS_LOW,
	CE_LASER_HOT,
	CE_CABIN_HOT,
	CE_FUEL_SCOOP,
	CE_CARGO_SCOOP,
	CE_OVERDRIVE,
	CE_BOUNTY,
	CE_INNOCENT,
	CE_SHOOT_STATION,
	CE_LOCK_POLICE,
	CE_LOW_FUEL,
	CE_BOMB,
	CE_MISSILE_IN,
	CE_PIRATES,
	CE_ESCAPE,
	CE_QUIET,         // nothing for a while
	CE_JOIN,
	CE_BACK,          // back with us, having been taken
	CE_PATCHED,       // Marge has patched the shields
	CE_EVENTS
} CompanionEvent_e;

// Something has happened; busy if there is fighting about, when only what
// matters is said
void companion_event(CompanionEvent_e e, bool busy);
// Each frame, flying or not, to time what is said
void companion_tick(bool flying, bool busy);
// Docked: the companion's mood goes into their loyalty, and with too little
// they leave
void companion_dock(void);

// Who is aboard, and not away, and their name, or NULL
Companion_e companion(void);
const char* companion_name(void);
const char* companion_mood_name(void);
const char* pet_name(void);
const char* companion_name_of(uint32_t c);

// For the stories
void companion_join(uint32_t c);
void companion_leave(void);
void companion_away(bool away);
void companion_loyalty(int32_t by, uint32_t only);
void companion_pet(uint32_t p);

// For the harness: the lines one has for an event, and the mood they give
uint32_t companion_lines(uint32_t c, CompanionEvent_e e, const char** lines, uint32_t max);
uint32_t pet_lines(uint32_t p, CompanionEvent_e e, const char** lines, uint32_t max);
int32_t companion_likes(uint32_t c, CompanionEvent_e e);
const char* companion_event_name(CompanionEvent_e e);

#ifdef __cplusplus
}
#endif
