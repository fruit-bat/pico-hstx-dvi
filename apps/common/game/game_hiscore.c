#include "game_hiscore.h"
#include <string.h>

// Held in RAM, so the table lasts until the power is turned off
static GameHiScore_t _table[GAME_HISCORE_COUNT];

static const char* _rank_names[GAME_HISCORE_COUNT] = {
	"1ST", "2ND", "3RD", "4TH", "5TH", "6TH", "7TH", "8TH", "9TH", "10TH"
};

void game_hiscore_init(const GameHiScore_t* defaults) {
	memcpy(_table, defaults, sizeof(_table));
}

int32_t game_hiscore_rank(uint32_t score) {
	for (uint32_t i = 0; i < GAME_HISCORE_COUNT; ++i) {
		if (score > _table[i].score) return i;
	}
	return -1;
}

int32_t game_hiscore_insert(uint32_t score, const char* name) {
	const int32_t rank = game_hiscore_rank(score);
	if (rank < 0) return rank;
	memmove(&_table[rank + 1], &_table[rank], sizeof(GameHiScore_t) * (GAME_HISCORE_COUNT - 1 - rank));
	GameHiScore_t* e = &_table[rank];
	e->score = score;
	strncpy(e->name, name, GAME_HISCORE_NAME_LEN);
	e->name[GAME_HISCORE_NAME_LEN] = 0;
	return rank;
}

const GameHiScore_t* game_hiscore_get(uint32_t rank) {
	return &_table[rank < GAME_HISCORE_COUNT ? rank : GAME_HISCORE_COUNT - 1];
}

uint32_t __not_in_flash_func(game_hiscore_top)(void) {
	return _table[0].score;
}

const char* game_hiscore_rank_name(uint32_t rank) {
	return _rank_names[rank < GAME_HISCORE_COUNT ? rank : GAME_HISCORE_COUNT - 1];
}
