#include "game_initials.h"

// Frames a direction is held for before it starts to repeat, and then how often
#define GAME_INITIALS_REPEAT_DELAY 20
#define GAME_INITIALS_REPEAT_RATE 5

static const char _letters[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ.-! ";
#define GAME_INITIALS_LETTERS (sizeof(_letters) - 1)

void game_initials_reset(GameInitials_t* g) {
	for (uint32_t j = 0; j < GAME_HISCORE_NAME_LEN; ++j) g->letter[j] = 0;
	g->pos = 0;
	g->held = 0;
}

bool game_initials_update(GameInitials_t* g, int32_t dir, bool stepped, bool enter) {
	if (g->pos >= GAME_HISCORE_NAME_LEN) return true;
	g->held = dir ? g->held + 1 : 0;
	const bool step = stepped ||
		(g->held > GAME_INITIALS_REPEAT_DELAY && (g->held % GAME_INITIALS_REPEAT_RATE) == 0);
	if (dir && step) {
		g->letter[g->pos] = (g->letter[g->pos] + GAME_INITIALS_LETTERS + dir) % GAME_INITIALS_LETTERS;
	}
	if (enter) ++g->pos;
	return g->pos >= GAME_HISCORE_NAME_LEN;
}

char game_initials_char(const GameInitials_t* g, uint32_t j) {
	return j <= g->pos ? _letters[g->letter[j]] : '-';
}

void game_initials_name(const GameInitials_t* g, char name[GAME_HISCORE_NAME_LEN + 1]) {
	for (uint32_t j = 0; j < GAME_HISCORE_NAME_LEN; ++j) {
		name[j] = j <= g->pos ? _letters[g->letter[j]] : ' ';
	}
	name[GAME_HISCORE_NAME_LEN] = 0;
}
