// Copyright (c) 2024 fruit-bat.

// The stories as data, see el_story.h: what they are made of, for writing
// them in el_story_data.c and for the engine in el_story.c.
#pragma once

#include "el_story.h"

#ifdef __cplusplus
extern "C" {
#endif

// What an exit or a choice does
typedef enum {
	OP_END = 0,
	OP_GIVE,      // the named item a
	OP_TAKE,      // the named item a
	OP_WORSE,     // the item from a to b aboard goes to the next, worse one
	OP_CASH,      // a credits, or takes them
	OP_OFFENDER,  // makes us one, or not if a is 0
	OP_FLAG,      // remembers STORY_FLAG_ a
	OP_COUNT,     // sets the counter to a
	OP_ADD,       // adds a to the counter
	OP_SPAWN,     // story ship a, from _ships, as id b
	OP_CANISTER,  // a canister holding the item a, ahead
	OP_CALM,      // the story's ships stop fighting
	OP_WITCH,     // the next jump goes into witchspace
	OP_LASER,     // a laser at the front, Laser_e a, if better
	OP_JOIN,      // companion a joins us
	OP_LEAVE,     // the companion leaves
	OP_AWAY,      // the companion is taken away
	OP_BACK,      // and comes back
	OP_LOYAL,     // the companion's loyalty, by a; if b, only companion b's
	OP_PET,       // pet a joins us, or goes if 0
	OP_ENDING,    // the story ends as ending a
	OP_DRAIN      // our shields go down
} StoryOpCode_e;

typedef struct {
	uint8_t op;
	int16_t a, b;
} StoryOp_t;

// When an exit or choice can be taken
typedef enum {
	IF_ALWAYS = 0,
	IF_ITEM,        // item a aboard
	IF_NO_ITEM,
	IF_COMPANION,   // companion a aboard, or any if a is 0
	IF_NO_COMPANION,
	IF_CABIN,       // a passenger cabin fitted
	IF_CASH,        // a credits
	IF_COUNT_AT_LEAST,
	IF_COUNT_UNDER,
	IF_FLAG,
	IF_NO_FLAG,
	IF_ENDED,       // story a ended as ending b
	IF_TOLD,        // story a has been told
	IF_ROOM,        // a tonnes free in the hold
	IF_FITTED       // StoryKit_e a fitted
} StoryIf_e;

typedef struct {
	uint8_t what;
	int16_t a, b;
} StoryIf_t;

// Back to untold, to be offered again another time
#define NEXT_AGAIN 0xfd
#define NEXT_SAME 0xfe
#define NEXT_DONE 0xff

typedef struct {
	const char* label;  // NULL for an exit without a choice
	StoryIf_t when;
	const StoryOp_t* ops;
	uint8_t next;
} StoryChoice_t;

// Which of the story's systems, or anywhere
#define PLACE_ANY 0xff

typedef struct {
	uint8_t ev;         // StoryEvent_e
	uint8_t place;      // for docking and arriving, or PLACE_ANY
	uint8_t arg;        // the item or ship, or for STORY_JUMP how many
	StoryIf_t when;
	uint8_t who;        // Speaker_e
	const char* text;   // lines split by '\n', with %0 to %2 the systems and %C the companion
	StoryChoice_t choice[2];
} StoryExit_t;

typedef struct {
	const StoryExit_t* exits;
	uint32_t n;
} StoryStep_t;

// The systems a story goes to, picked when it starts
typedef enum {
	PICK_HERE = 0,      // where it starts
	PICK_HERE_ANARCHY,  // where it starts, which must be lawless
	PICK_HERE_INDUSTRY, // where it starts, which must be industrial
	PICK_HERE_SAFE,     // where it starts, which must be orderly
	PICK_HERE_RICH,     // where it starts, which must be rich
	PICK_NEAR,          // in one jump from the system before
	PICK_NEAR_ANARCHY,
	PICK_NEAR_SAFE,
	PICK_NEAR_TECH,     // high tech
	PICK_FAR,           // two or three jumps on
	PICK_NONE
} StoryPick_e;

typedef struct {
	const char* name;
	uint8_t pick[STORY_PLACES];
	uint8_t chance;      // percent, each time it could start
	StoryIf_t when;      // and only when this holds
	const StoryStep_t* steps;
	uint32_t n;
} Story_t;

// The named items
typedef struct {
	const char* name;
	uint8_t tonnes;  // the room it takes in the hold, if any
} StoryItem_t;

extern const Story_t story_data[];
extern const uint32_t story_data_count;
extern const StoryItem_t story_items[];
extern const uint32_t story_items_count;
extern const StoryShip_t story_ships[];
extern const uint32_t story_ships_count;

// The save the engine is on, for the companions
StorySave_t* story_save(void);
// Whether a condition holds now for story i, for the harness
bool story_holds(uint32_t i, const StoryIf_t* w);

#ifdef __cplusplus
}
#endif
