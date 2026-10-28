#pragma once

#include "pico/stdlib.h"

#ifdef __cplusplus
extern "C" {
#endif

#define INV_HISCORE_COUNT 10
#define INV_HISCORE_NAME_LEN 3

typedef struct {
	uint32_t score;
	char name[INV_HISCORE_NAME_LEN + 1];
} InvHiScore_t;

void inv_hiscore_init(void);

// The rank a score would take in the table, or -1 if it does not make it
int32_t inv_hiscore_rank(uint32_t score);

// Returns the rank the score was entered at, or -1 if it did not make it
int32_t inv_hiscore_insert(uint32_t score, const char* name);

const InvHiScore_t* inv_hiscore_get(uint32_t rank);

uint32_t inv_hiscore_top(void);

// "1ST", "2ND" ...
const char* inv_hiscore_rank_name(uint32_t rank);

#ifdef __cplusplus
}
#endif
