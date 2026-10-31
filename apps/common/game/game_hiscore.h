#pragma once

#include "pico/stdlib.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GAME_HISCORE_COUNT 10
#define GAME_HISCORE_NAME_LEN 3

typedef struct {
	uint32_t score;
	char name[GAME_HISCORE_NAME_LEN + 1];
} GameHiScore_t;

// Starts the table off with the game's own defaults, GAME_HISCORE_COUNT of them
void game_hiscore_init(const GameHiScore_t* defaults);

// The rank a score would take in the table, or -1 if it does not make it
int32_t game_hiscore_rank(uint32_t score);

// Returns the rank the score was entered at, or -1 if it did not make it
int32_t game_hiscore_insert(uint32_t score, const char* name);

const GameHiScore_t* game_hiscore_get(uint32_t rank);

uint32_t game_hiscore_top(void);

// "1ST", "2ND" ...
const char* game_hiscore_rank_name(uint32_t rank);

#ifdef __cplusplus
}
#endif
