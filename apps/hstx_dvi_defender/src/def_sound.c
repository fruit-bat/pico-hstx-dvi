// Copyright (c) 2024 fruit-bat.

// See def_sound.h.

#include "def_sound.h"

extern const SeqSound_t def_sound_data[DEF_SFX_SOUNDS];
extern const SeqTune_t def_tune_data[DEF_TUNES];

static const char* const _names[DEF_SFX_SOUNDS + DEF_TUNES] = {
	[DEF_SFX_LASER] = "LASER",
	[DEF_SFX_EXPLODE] = "EXPLODE",
	[DEF_SFX_ENEMY_FIRE] = "ENEMY FIRE",
	[DEF_SFX_SPAWN] = "SPAWN",
	[DEF_SFX_GRAB] = "GRAB",
	[DEF_SFX_FALL] = "FALL",
	[DEF_SFX_CATCH] = "CATCH",
	[DEF_SFX_RESCUE] = "RESCUE",
	[DEF_SFX_HUMAN_DIE] = "HUMAN DIE",
	[DEF_SFX_MUTANT] = "MUTANT",
	[DEF_SFX_BAITER] = "BAITER",
	[DEF_SFX_SMART_BOMB] = "SMART BOMB",
	[DEF_SFX_HYPERSPACE] = "HYPERSPACE",
	[DEF_SFX_DEAD] = "DEAD",
	[DEF_SFX_RESPAWN] = "RESPAWN",
	[DEF_SFX_EXTRA_LIFE] = "EXTRA LIFE",
	[DEF_SFX_BONUS] = "BONUS",
	[DEF_SFX_PLANET] = "PLANET",
	[DEF_SFX_THRUST] = "THRUST",
	[DEF_SFX_SOUNDS + DEF_TUNE_TITLE] = "TITLE TUNE",
	[DEF_SFX_SOUNDS + DEF_TUNE_WAVE] = "WAVE TUNE",
	[DEF_SFX_SOUNDS + DEF_TUNE_GAME_OVER] = "GAME OVER TUNE",
	[DEF_SFX_SOUNDS + DEF_TUNE_HISCORE] = "HIGH SCORE TUNE",
};

const GameSounds_t game_sounds = { def_sound_data, DEF_SFX_SOUNDS, def_tune_data, DEF_TUNES, _names };

void def_sound_init(void) {
	game_sound_init(&game_sounds);
}
