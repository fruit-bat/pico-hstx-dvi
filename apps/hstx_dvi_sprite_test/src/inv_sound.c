// Copyright (c) 2024 fruit-bat.

// See inv_sound.h.

#include "inv_sound.h"

extern const SeqSound_t inv_sound_data[INV_SFX_SOUNDS];
extern const SeqTune_t inv_tune_data[INV_TUNES];

static const char* const _names[INV_SFX_SOUNDS + INV_TUNES] = {
	[INV_SFX_SHOOT] = "SHOOT",
	[INV_SFX_HIT] = "HIT",
	[INV_SFX_ARMOUR] = "ARMOUR",
	[INV_SFX_MARCH_1] = "MARCH 1",
	[INV_SFX_MARCH_2] = "MARCH 2",
	[INV_SFX_MARCH_3] = "MARCH 3",
	[INV_SFX_MARCH_4] = "MARCH 4",
	[INV_SFX_DIVE] = "DIVE",
	[INV_SFX_BASE] = "BASE",
	[INV_SFX_MYSTERY_HIT] = "MYSTERY HIT",
	[INV_SFX_DEAD] = "DEAD",
	[INV_SFX_LANDED] = "LANDED",
	[INV_SFX_EXTRA_LIFE] = "EXTRA LIFE",
	[INV_SFX_MYSTERY] = "MYSTERY",
	[INV_SFX_SOUNDS + INV_TUNE_TITLE] = "TITLE TUNE",
	[INV_SFX_SOUNDS + INV_TUNE_MARCH] = "SAUCER MARCH",
	[INV_SFX_SOUNDS + INV_TUNE_SURF] = "ATOMIC SURF",
	[INV_SFX_SOUNDS + INV_TUNE_WAVE] = "WAVE TUNE",
	[INV_SFX_SOUNDS + INV_TUNE_GAME_OVER] = "GAME OVER TUNE",
	[INV_SFX_SOUNDS + INV_TUNE_HISCORE] = "HIGH SCORE TUNE",
};

const GameSounds_t game_sounds = { inv_sound_data, INV_SFX_SOUNDS, inv_tune_data, INV_TUNES, _names };

void inv_sound_init(void) {
	game_sound_init(&game_sounds);
}
