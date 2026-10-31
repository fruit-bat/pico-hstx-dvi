#pragma once

#include "pico/stdlib.h"

#ifdef __cplusplus
extern "C" {
#endif

// The folk of the wood
#define WW_BUNNY 0
#define WW_FROG 1
#define WW_HEDGEHOG 2
#define WW_FOX 3
#define WW_BADGER 4
#define WW_DEER 5
#define WW_BOAR 6
#define WW_BEAR 7
#define WW_OWL 8
#define WW_OWLET 9
#define WW_WOLF 10
#define WW_ANIMAL_TYPES 11

// How they are feeling
#define WW_MOOD_HAPPY 0
#define WW_MOOD_HURT 1
#define WW_MOOD_GRUMPY 2
#define WW_MOOD_ASLEEP 3
// A baby that has wandered off, and then follows the witch once found
#define WW_MOOD_LOST 4
#define WW_MOOD_FOLLOW 5
// Its mother, waiting for it to come home
#define WW_MOOD_WAITING 6
#define WW_MOODS 7

// Has been helped, so there are no more points for it
#define WW_AF_DONE 1
// Goes back to sleep when left alone
#define WW_AF_SLEEPER 2
// Gives up a piece of the moon charm when helped
#define WW_AF_PIECE 4
// Has been cast at while happy, for a few points the first time
#define WW_AF_FRIEND 8
// The witch has been close while it slept
#define WW_AF_NEAR 16
// The witch has been rewarded for creeping past it
#define WW_AF_CREPT 32
// Has said something since the witch came in
#define WW_AF_SPOKE 64

typedef struct {
	uint8_t type;
	uint8_t mood;
	uint8_t room;
	uint8_t need;  // spells still needed to heal or calm it
	uint8_t flags;
	uint8_t phase; // so they do not all move together
	bool left;
	bool moving;
	int32_t x, y;  // fixed point position of its feet in the room
	int32_t hx, hy;
	int32_t vx, vy;
	uint16_t timer;
	uint16_t stun;
	uint16_t joy;   // frames to show a heart for after being helped
	uint16_t flash;
	uint16_t stuck;
	uint16_t detour;
	uint16_t anim;
} WwAnimal_t;

void ww_animal_init(void);

// Fills the wood, using game_rand
void ww_animal_populate(uint32_t night);
// Where the bear sleeps, so its charm piece can be left by it
bool ww_animal_bear(uint32_t* room, int32_t* x, int32_t* y);

// The witch comes in to a room at x, y, bringing anyone following her
void ww_animal_enter(uint32_t room, int32_t x, int32_t y);
// Leaving the room puts everyone back. Returns how many sleepers she crept past.
uint32_t ww_animal_leave(void);
// After she loses a life, the room calms down
void ww_animal_reset_room(void);

void ww_animal_update(uint32_t frame);
// A spell at x, y in the room. Returns true if it hit someone.
bool ww_animal_spell(int32_t x, int32_t y);

// The old wolf turns up if she stays in one room for too long
void ww_animal_wolf_spawn(int32_t x, int32_t y, uint32_t night);
bool ww_animal_wolf_here(void);
void ww_animal_wolf_remove(void);
bool ww_animal_wolf_calmed(void);

// Animals helped this night
uint32_t ww_animal_helped(void);
// Where the owl is waiting for her owlet
uint32_t ww_animal_owl_room(void);
bool ww_animal_following(void);

// Those in the current room, including the wolf, for the demo player
uint32_t ww_animal_here(void);
const WwAnimal_t* ww_animal_get(uint32_t i);
uint32_t ww_animal_width(uint32_t type);
uint32_t ww_animal_height(uint32_t type);
const char* ww_animal_name(uint32_t type);

// Those in front of split_y, or those behind, nearest first
void ww_animal_draw(uint32_t frame, int32_t split_y, bool front);
// What they are feeling, over their heads, to go over everything else
void ww_animal_draw_moods(uint32_t frame);

// For outside of the game, sx and sy being the top left on the screen
void ww_animal_draw_at(uint32_t type, uint32_t mood, int32_t sx, int32_t sy, uint32_t frame, bool left);
// cx is the middle and sy the bottom of the picture
void ww_animal_draw_mood(uint32_t mood, int32_t cx, int32_t sy, uint32_t frame);

#ifdef __cplusplus
}
#endif
