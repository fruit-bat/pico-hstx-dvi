// Copyright (c) 2024 fruit-bat.

// Stories: small adventures that find us as we go, across a few systems,
// with choices that matter, and the companion who may fly with us.
//
// A story is a list of steps, as data. Each step waits for one of its exits:
// something happening, such as docking at one of the story's systems,
// scooping a canister or disabling a ship, maybe only while a condition
// holds. An exit shows a page, with up to two choices, or has someone say a
// line, then does what it or the choice says, and moves to another step.
// The systems a story goes to are picked when it starts, from those near.
//
// Only one story goes at a time. One starts now and then on docking or
// coming into a system, when none is going.
//
// It knows nothing of the screen or the ships: what it does to the game it
// does through the story_host_ functions, which the game provides, and the
// harness on the PC, which walks every story through every choice.
#pragma once

#include "galaxy.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define STORIES_MAX 12
#define STORY_ITEMS 8
#define STORY_PLACES 3
#define STORY_LOG 16

// What has happened, with the system, item or story ship it happened to
typedef enum {
	STORY_DOCK = 0,  // docked, at system arg
	STORY_ARRIVE,    // came into system arg by jumping, a moment after
	STORY_JUMP,      // jumped, anywhere, for counting
	STORY_WITCH,     // fell into witchspace
	STORY_SCOOP,     // scooped the canister holding item arg
	STORY_DISABLED,  // disabled story ship arg
	STORY_DESTROYED, // destroyed story ship arg
	STORY_ESCAPED,   // story ship arg got where it was going
	STORY_NEAR,      // story ship arg came near
	STORY_HULL_HIT,  // a hit got through the shields
	STORY_SUN,       // the cabin got hot by the sun
	STORY_NOW,       // straight after a choice, for showing what came of it
	STORY_LAUNCH,    // launching from system arg, still docked
	STORY_DEPART,    // out of the station at system arg, a moment after
	STORY_APPROACH,  // came near the station at system arg
	STORY_HERE,      // for exits only: ARRIVE or DEPART, flying in system arg
	STORY_EVENTS
} StoryEvent_e;

// Who says a line: a page, for reading and choosing, or the companion or pet
typedef enum {
	SPEAK_PAGE = 0,
	SPEAK_COMPANION,
	SPEAK_PET,
	SPEAK_HAIL
} Speaker_e;

// A story's progress, saved with the commander
typedef struct {
	uint8_t status;  // STORY_UNTOLD, STORY_GOING or STORY_TOLD
	uint8_t step;
	uint8_t place[STORY_PLACES];
	uint8_t galaxy;
	uint8_t counter; // for the story's own use, such as how a cake is
	uint8_t jumps;   // since coming to the step
	uint8_t ending;  // how it ended, for others to remember
	uint8_t pad[3];
} StoryProgress_t;

#define STORY_UNTOLD 0
#define STORY_GOING 1
#define STORY_TOLD 2

// All there is to save, which goes on the end of the commander
typedef struct {
	StoryProgress_t story[STORIES_MAX];
	uint8_t item[STORY_ITEMS]; // named items aboard, 0 for none
	uint8_t active;            // the story going, from 1, or 0
	uint8_t companion;         // who flies with us, from 1, or 0
	uint8_t pet;               // from 1, or 0
	uint8_t away;              // the companion is away, as kidnapped
	int8_t mood;               // the companion's, now
	int8_t loyalty;            // and over time
	uint8_t pad[2];
	uint32_t flags;            // what the stories remember, STORY_FLAG_
	uint32_t seed;             // for picking, so the harness can repeat it
	// What happened in the story last going, from 1, for the log: each the
	// step, and the exit taken from it and the choice made, from 1, or 0
	uint8_t log_story;
	uint8_t log_n;
	uint8_t log[STORY_LOG][2];
	uint8_t pad2[2];
} StorySave_t;

// What the stories remember that the game looks at
#define STORY_FLAG_THARGOID_FRIEND 0x01 // Thargoids leave us be in witchspace
#define STORY_FLAG_PIRATE_FRIEND   0x02 // pirates leave us be
#define STORY_FLAG_CAT             0x04 // the ginger kitten stayed
#define STORY_FLAG_MEDAL           0x08
#define STORY_FLAG_SOLD_THARGOID   0x10
#define STORY_FLAG_CAPTAIN_FRIEND  0x20 // the cursed trader, cured

// A page to show, with the lines filled in
#define PAGE_LINES 8
#define PAGE_LINE_MAX 48
typedef struct {
	const char* title;
	uint32_t lines;
	char line[PAGE_LINES][PAGE_LINE_MAX];
	uint32_t choices;               // 0 for just going on
	const char* choice[2];
	bool can[2];                    // whether a choice can be made
} StoryPage_t;

// The engine on a save, which it keeps up to date
void story_bind(StorySave_t* s);
// A new commander's: no stories told, nobody aboard
void story_reset(StorySave_t* s);

// Something has happened
void story_event(StoryEvent_e ev, uint32_t arg);

// What the story going waits for, in words, for the log: those that can
// happen now, without the jumps it gives up after. Returns how many lines.
#define STORY_HINT_MAX 40
uint32_t story_hints(char (*line)[STORY_HINT_MAX], uint32_t max);
// How many stories have been told
uint32_t story_told(void);
// What has happened in the story last going, oldest first, in short: what
// each page or line began with, and after it any choice made, as choice[].
// Returns how many lines, the last ones if there are more than max, and the
// story, from 0, or -1.
#define STORY_LOG_LINE 46
uint32_t story_log(char (*line)[STORY_LOG_LINE], bool* choice, uint32_t max, int32_t* story);

// The page waiting to be read, if any, and the choice made on it, which
// does what it says
bool story_page(StoryPage_t* page);
void story_choose(uint32_t choice);

// The named items aboard, for the inventory, and whether one is
bool story_has_item(uint32_t item);
const char* story_item_name(uint32_t item);
uint32_t story_item_names(const char** names, uint32_t max);
// The tonnes the named items aboard take in the hold, such as a whale tank
uint32_t story_item_tonnes(void);
bool story_flag(uint32_t flag);

// The stories, for the harness: how many, their names, and starting one
// at system here, whatever the chance, returning false if it cannot
uint32_t story_count(void);
const char* story_name(uint32_t i);
bool story_start(uint32_t i, uint32_t here);
// The story going, from 0, or -1
int32_t story_active(void);
const StoryProgress_t* story_progress(uint32_t i);
// For trying a story on the PC: moves story i on to a step
void story_debug_step(uint32_t i, uint32_t step);
// Whether an event happens at a system, given as its arg
bool story_at_place(StoryEvent_e ev);
// What a story's exits wait for at its step: their events, for the harness
// to make them happen, and its systems, items and ships
uint32_t story_waits(uint32_t i, StoryEvent_e* ev, uint32_t* arg, uint32_t max);
// The page waiting, kept and put back, for the harness to try each choice
typedef struct {
	int32_t pending;
	const void* exit;
	const char* title;
	const char* text;
} StoryPending_t;
void story_pending_keep(StoryPending_t* k);
void story_pending_put(const StoryPending_t* k);
// Shows a page that is not part of a story, such as a companion leaving
void story_note(const char* title, const char* text);

// What the game does for the stories
const GalaxySystem_t* story_host_system(uint32_t i);
uint32_t story_host_galaxy(void);
uint32_t story_host_here(void);
bool story_host_has_cabin(void);
uint32_t story_host_cash(void);       // in tenths of a credit
void story_host_add_cash(int32_t tenths);
void story_host_offender(bool on);
// The tonnes free in the hold
uint32_t story_host_room(void);
// Equipment a story may look for
typedef enum {
	KIT_TARGETING = 0,
	KIT_DOCKING,
	KIT_SCOOPS,
	KIT_ECM
} StoryKit_e;
bool story_host_fitted(uint32_t kit);
// A story ship: its id, what it is called and what it is, how it goes, and what it leaves
typedef enum {
	GOAL_FIGHT = 0, // comes for us
	GOAL_IDLE,      // goes about its business, fighting back if shot
	GOAL_RACE,      // flies from the station to the planet
	GOAL_DRIFT,     // disabled already, drifting
	GOAL_FOLLOW,    // keeps near us
	GOAL_WEAVE      // goes about weaving oddly, as a Thargon does
} StoryGoal_e;
typedef struct {
	const char* label;  // the name the targeting computer shows
	const char* cls;    // the type of ship, as its class is named
	uint8_t goal;
	bool disable;       // hit hard, it is disabled rather than destroyed
	uint8_t drop;       // the item it leaves when disabled or destroyed, or 0
	bool at_station;    // starts at the station, rather than ahead of us
} StoryShip_t;
void story_host_spawn(uint32_t id, const StoryShip_t* ship);
void story_host_canister(uint32_t item);
void story_host_calm(void);
void story_host_witch(void);
void story_host_laser(uint32_t laser);
void story_host_drain(void);          // the shields go down
void story_host_say(Speaker_e who, const char* name, const char* text);

#ifdef __cplusplus
}
#endif
