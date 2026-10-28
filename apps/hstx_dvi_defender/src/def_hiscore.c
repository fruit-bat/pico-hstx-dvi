#include "def_hiscore.h"
#include <string.h>

// Held in RAM, so the table lasts until the power is turned off
static DefHiScore_t _table[DEF_HISCORE_COUNT];

static const DefHiScore_t _default_table[DEF_HISCORE_COUNT] = {
	{ 25000, "BAT" },
	{ 22500, "PIC" },
	{ 20000, "HST" },
	{ 17500, "DVI" },
	{ 15000, "RP2" },
	{ 12500, "ZAP" },
	{ 10000, "ACE" },
	{  7500, "CPU" },
	{  5000, "SPR" },
	{  2500, "BUG" },
};

static const char* _rank_names[DEF_HISCORE_COUNT] = {
	"1ST", "2ND", "3RD", "4TH", "5TH", "6TH", "7TH", "8TH", "9TH", "10TH"
};

void def_hiscore_init(void) {
	memcpy(_table, _default_table, sizeof(_table));
}

int32_t def_hiscore_rank(uint32_t score) {
	for (uint32_t i = 0; i < DEF_HISCORE_COUNT; ++i) {
		if (score > _table[i].score) return i;
	}
	return -1;
}

int32_t def_hiscore_insert(uint32_t score, const char* name) {
	const int32_t rank = def_hiscore_rank(score);
	if (rank < 0) return rank;
	memmove(&_table[rank + 1], &_table[rank], sizeof(DefHiScore_t) * (DEF_HISCORE_COUNT - 1 - rank));
	DefHiScore_t* e = &_table[rank];
	e->score = score;
	strncpy(e->name, name, DEF_HISCORE_NAME_LEN);
	e->name[DEF_HISCORE_NAME_LEN] = 0;
	return rank;
}

const DefHiScore_t* def_hiscore_get(uint32_t rank) {
	return &_table[rank < DEF_HISCORE_COUNT ? rank : DEF_HISCORE_COUNT - 1];
}

uint32_t __not_in_flash_func(def_hiscore_top)(void) {
	return _table[0].score;
}

const char* def_hiscore_rank_name(uint32_t rank) {
	return _rank_names[rank < DEF_HISCORE_COUNT ? rank : DEF_HISCORE_COUNT - 1];
}
