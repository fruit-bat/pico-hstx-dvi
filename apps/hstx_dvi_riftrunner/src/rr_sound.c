// Copyright (c) 2024 fruit-bat.

// See rr_sound.h.

#include "rr_sound.h"

extern const SeqSound_t rr_sound_data[RR_SFX_SOUNDS];
extern const SeqTune_t rr_tune_data[RR_TUNES];

static const char* const _names[RR_SFX_SOUNDS + RR_TUNES] = {
	[RR_SFX_SHOT] = "SHOT",
	[RR_SFX_LASER] = "LASER",
	[RR_SFX_BEAM] = "BEAM",
	[RR_SFX_BOMB] = "BOMB",
	[RR_SFX_BLAST] = "BLAST",
	[RR_SFX_HIT] = "HIT",
	[RR_SFX_EXPLODE] = "EXPLODE",
	[RR_SFX_EXPLODE_BIG] = "BIG EXPLOSION",
	[RR_SFX_LAUNCH] = "LAUNCH",
	[RR_SFX_ENEMY_FIRE] = "ENEMY FIRE",
	[RR_SFX_CAPSULE] = "CAPSULE",
	[RR_SFX_POWER] = "POWER UP",
	[RR_SFX_FUEL] = "FUEL",
	[RR_SFX_FUEL_LOW] = "FUEL LOW",
	[RR_SFX_DEAD] = "DEAD",
	[RR_SFX_RESPAWN] = "RESPAWN",
	[RR_SFX_EXTRA_LIFE] = "EXTRA LIFE",
	[RR_SFX_ALARM] = "ALARM",
	[RR_SFX_BOSS_HIT] = "WARSHIP HIT",
	[RR_SFX_BOSS_DIE] = "WARSHIP DOWN",
	[RR_SFX_TALLY] = "TALLY",
	[RR_SFX_CHARGE] = "CHARGE",
	[RR_SFX_SOUNDS + RR_TUNE_TITLE] = "TITLE TUNE",
	[RR_SFX_SOUNDS + RR_TUNE_STAGE] = "FLYING TUNE",
	[RR_SFX_SOUNDS + RR_TUNE_BOSS] = "WARSHIP TUNE",
	[RR_SFX_SOUNDS + RR_TUNE_CLEAR] = "VICTORY TUNE",
	[RR_SFX_SOUNDS + RR_TUNE_GAME_OVER] = "GAME OVER TUNE",
	[RR_SFX_SOUNDS + RR_TUNE_HISCORE] = "HIGH SCORE TUNE",
};

const GameSounds_t game_sounds = { rr_sound_data, RR_SFX_SOUNDS, rr_tune_data, RR_TUNES, _names };

void rr_sound_init(void) {
	game_sound_init(&game_sounds);
}
