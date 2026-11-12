// Copyright (c) 2024 fruit-bat.

// See el_sound.h.

#include "el_sound.h"

extern const SeqSound_t sound_data[SFX_SOUNDS];
extern const SeqTune_t tune_data[TUNES];

const char* const sound_test_names[SOUND_TESTS] = {
	[SFX_LASER] = "LASER",
	[SFX_HIT] = "HIT",
	[SFX_EXPLOSION] = "EXPLOSION",
	[SFX_MISSILE] = "MISSILE",
	[SFX_ECM] = "ECM",
	[SFX_BOMB] = "ENERGY BOMB",
	[SFX_SCOOPED] = "SCOOPED",
	[SFX_CHOOSE] = "CHOOSE",
	[SFX_PAGE] = "PAGE",
	[SFX_COUNT] = "COUNTDOWN",
	[SFX_HYPERSPACE] = "HYPERSPACE",
	[SFX_TUNNEL] = "TUNNEL",
	[SFX_DENIED] = "DENIED",
	[SFX_DEAD] = "DEAD",
	[SFX_INCOMING] = "INCOMING",
	[SFX_LOW_ENERGY] = "LOW ENERGY",
	[SFX_HEAT] = "HEAT",
	[SFX_SCOOPING] = "FUEL SCOOP",
	[SFX_OVERDRIVE] = "OVERDRIVE",
	[SFX_SOUNDS + TUNE_TITLE] = "TITLE TUNE",
	[SFX_SOUNDS + TUNE_DOCKING] = "DOCKING TUNE",
};

const GameSounds_t game_sounds = { sound_data, SFX_SOUNDS, tune_data, TUNES, sound_test_names };

void sound_init(void) {
	game_sound_init(&game_sounds);
}
