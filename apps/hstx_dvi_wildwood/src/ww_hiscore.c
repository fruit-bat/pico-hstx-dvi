#include "ww_hiscore.h"

static const GameHiScore_t _defaults[GAME_HISCORE_COUNT] = {
	{ 20000, "HAZ" },
	{ 18000, "OWL" },
	{ 16000, "FOX" },
	{ 14000, "BUN" },
	{ 12000, "DEE" },
	{ 10000, "BEA" },
	{  8000, "HOG" },
	{  6000, "FRG" },
	{  4000, "BAD" },
	{  2000, "WLF" },
};

const GameHiScore_t* ww_hiscore_defaults(void) {
	return _defaults;
}
