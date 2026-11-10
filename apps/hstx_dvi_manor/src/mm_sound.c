// Copyright (c) 2024 fruit-bat.

// See mm_sound.h.

#include "mm_sound.h"

extern const SeqSound_t mm_sound_data[MM_SFX_SOUNDS];
extern const SeqTune_t mm_tune_data[MM_TUNES];

static const char* const _names[MM_SFX_SOUNDS + MM_TUNES] = {
	[MM_SFX_JUMP] = "JUMP",
	[MM_SFX_CAST] = "CAST",
	[MM_SFX_CHEER] = "CHEER",
	[MM_SFX_PULL] = "PULL",
	[MM_SFX_HURT] = "HURT",
	[MM_SFX_DIZZY] = "DIZZY",
	[MM_SFX_KITTEN] = "KITTEN",
	[MM_SFX_SWEET] = "SWEET",
	[MM_SFX_HEART] = "HEART",
	[MM_SFX_KEY] = "KEY",
	[MM_SFX_FEATHER] = "FEATHER",
	[MM_SFX_LEVER] = "LEVER",
	[MM_SFX_CANDLE] = "CANDLE",
	[MM_SFX_ALL_LIT] = "ALL LIT",
	[MM_SFX_GATE] = "GATE",
	[MM_SFX_UNLOCK] = "UNLOCK",
	[MM_SFX_LOCKED] = "LOCKED",
	[MM_SFX_LANTERN] = "LANTERN",
	[MM_SFX_EXTRA_LIFE] = "EXTRA LIFE",
	[MM_SFX_BONUS] = "BONUS",
	[MM_SFX_FLOAT] = "FLOAT",
	[MM_SFX_SOUNDS + MM_TUNE_TITLE] = "TITLE TUNE",
	[MM_SFX_SOUNDS + MM_TUNE_PLAY] = "PLAY TUNE",
	[MM_SFX_SOUNDS + MM_TUNE_NIGHT] = "NIGHT TUNE",
	[MM_SFX_SOUNDS + MM_TUNE_NIGHT_END] = "NIGHT END TUNE",
	[MM_SFX_SOUNDS + MM_TUNE_GAME_OVER] = "GAME OVER TUNE",
	[MM_SFX_SOUNDS + MM_TUNE_HISCORE] = "HIGH SCORE TUNE",
};

const GameSounds_t game_sounds = { mm_sound_data, MM_SFX_SOUNDS, mm_tune_data, MM_TUNES, _names };

void mm_sound_init(void) {
	game_sound_init(&game_sounds);
}
