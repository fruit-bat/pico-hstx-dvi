#include "def_hiscore.h"

static const GameHiScore_t _defaults[GAME_HISCORE_COUNT] = {
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

const GameHiScore_t* def_hiscore_defaults(void) {
	return _defaults;
}
