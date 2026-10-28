#include "inv_hiscore.h"
#include <string.h>

// Held in RAM, so the table lasts until the power is turned off
static InvHiScore_t _table[INV_HISCORE_COUNT];

static const InvHiScore_t _default_table[INV_HISCORE_COUNT] = {
	{ 5000, "BAT" },
	{ 4500, "PIC" },
	{ 4000, "HST" },
	{ 3500, "DVI" },
	{ 3000, "RP2" },
	{ 2500, "ZAP" },
	{ 2000, "ACE" },
	{ 1500, "CPU" },
	{ 1000, "SPR" },
	{  500, "BUG" },
};

static const char* _rank_names[INV_HISCORE_COUNT] = {
	"1ST", "2ND", "3RD", "4TH", "5TH", "6TH", "7TH", "8TH", "9TH", "10TH"
};

void inv_hiscore_init(void) {
	memcpy(_table, _default_table, sizeof(_table));
}

int32_t inv_hiscore_rank(uint32_t score) {
	for (uint32_t i = 0; i < INV_HISCORE_COUNT; ++i) {
		if (score > _table[i].score) return i;
	}
	return -1;
}

int32_t inv_hiscore_insert(uint32_t score, const char* name) {
	const int32_t rank = inv_hiscore_rank(score);
	if (rank < 0) return rank;
	memmove(&_table[rank + 1], &_table[rank], sizeof(InvHiScore_t) * (INV_HISCORE_COUNT - 1 - rank));
	InvHiScore_t* e = &_table[rank];
	e->score = score;
	strncpy(e->name, name, INV_HISCORE_NAME_LEN);
	e->name[INV_HISCORE_NAME_LEN] = 0;
	return rank;
}

const InvHiScore_t* inv_hiscore_get(uint32_t rank) {
	return &_table[rank < INV_HISCORE_COUNT ? rank : INV_HISCORE_COUNT - 1];
}

uint32_t __not_in_flash_func(inv_hiscore_top)(void) {
	return _table[0].score;
}

const char* inv_hiscore_rank_name(uint32_t rank) {
	return _rank_names[rank < INV_HISCORE_COUNT ? rank : INV_HISCORE_COUNT - 1];
}
