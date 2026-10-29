#pragma once

#include "pico/stdlib.h"
#include "game_hiscore.h"

#ifdef __cplusplus
extern "C" {
#endif

// Choosing initials for the high score table, a letter at a time
typedef struct {
	uint8_t letter[GAME_HISCORE_NAME_LEN];
	uint32_t pos;
	uint32_t held;
} GameInitials_t;

void game_initials_reset(GameInitials_t* g);

// dir is -1, 0 or 1 for the direction held down, and stepped is true on the
// frame it goes down. Holding it down repeats. enter moves on to the next
// letter. Returns true once all the letters have been entered.
bool game_initials_update(GameInitials_t* g, int32_t dir, bool stepped, bool enter);

// The letter shown at j, or '-' for those not got to yet
char game_initials_char(const GameInitials_t* g, uint32_t j);

// Letters not got to are left blank
void game_initials_name(const GameInitials_t* g, char name[GAME_HISCORE_NAME_LEN + 1]);

#ifdef __cplusplus
}
#endif
