#include "mm_hiscore.h"

static const GameHiScore_t _defaults[GAME_HISCORE_COUNT] = {
	{ 30000, "HAZ" },
	{ 26000, "BOO" },
	{ 22000, "MEW" },
	{ 18000, "BAT" },
	{ 15000, "PUM" },
	{ 12000, "SPI" },
	{  9000, "OWL" },
	{  6000, "CAT" },
	{  4000, "KIT" },
	{  2000, "EEK" },
};

const GameHiScore_t* mm_hiscore_defaults(void) {
	return _defaults;
}
