// Copyright (c) 2024 fruit-bat.

// See ww_sound.h.

#include "ww_sound.h"

extern const SeqSound_t ww_sound_data[WW_SFX_SOUNDS];
extern const SeqTune_t ww_tune_data[WW_TUNES];

static const char* const _names[WW_SFX_SOUNDS + WW_TUNES] = {
	[WW_SFX_CAST] = "CAST",
	[WW_SFX_FRIEND] = "FRIEND",
	[WW_SFX_BETTER] = "BETTER",
	[WW_SFX_HAPPY] = "HAPPY",
	[WW_SFX_FOUND] = "FOUND",
	[WW_SFX_REUNITE] = "REUNITE",
	[WW_SFX_WAKE] = "WAKE",
	[WW_SFX_YELP] = "YELP",
	[WW_SFX_HURT] = "HURT",
	[WW_SFX_DIZZY] = "DIZZY",
	[WW_SFX_PIECE] = "PIECE",
	[WW_SFX_CHARM] = "CHARM",
	[WW_SFX_FLOWER] = "FLOWER",
	[WW_SFX_MUDDLE] = "MUDDLE",
	[WW_SFX_HOWL] = "HOWL",
	[WW_SFX_WOLF] = "WOLF",
	[WW_SFX_SNEAK] = "SNEAK",
	[WW_SFX_EXTRA_LIFE] = "EXTRA LIFE",
	[WW_SFX_BONUS] = "BONUS",
	[WW_SFX_SOUNDS + WW_TUNE_TITLE] = "TITLE TUNE",
	[WW_SFX_SOUNDS + WW_TUNE_PLAY] = "PLAY TUNE",
	[WW_SFX_SOUNDS + WW_TUNE_NIGHT] = "NIGHT TUNE",
	[WW_SFX_SOUNDS + WW_TUNE_NIGHT_END] = "NIGHT END TUNE",
	[WW_SFX_SOUNDS + WW_TUNE_GAME_OVER] = "GAME OVER TUNE",
	[WW_SFX_SOUNDS + WW_TUNE_HISCORE] = "HIGH SCORE TUNE",
};

const GameSounds_t game_sounds = { ww_sound_data, WW_SFX_SOUNDS, ww_tune_data, WW_TUNES, _names };

void ww_sound_init(void) {
	game_sound_init(&game_sounds);
}
