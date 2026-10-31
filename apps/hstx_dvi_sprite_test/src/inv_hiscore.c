#include "inv_hiscore.h"

static const GameHiScore_t _defaults[GAME_HISCORE_COUNT] = {
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

const GameHiScore_t* inv_hiscore_defaults(void) {
	return _defaults;
}
