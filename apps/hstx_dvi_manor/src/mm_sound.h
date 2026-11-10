// Copyright (c) 2024 fruit-bat.

// Sounds and music, Moonlit Manor's names for them. See game_sound.h for how
// they are played, and mm_sound_data.c for what they are.
#pragma once

#include "game_sound.h"
#include "hstx_dvi_sprite.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	// Played once
	MM_SFX_JUMP = 0,
	MM_SFX_CAST,
	MM_SFX_CHEER,      // a spook cheered up
	MM_SFX_PULL,       // a crate pulled by a spell
	MM_SFX_HURT,       // a heart lost
	MM_SFX_DIZZY,      // the last one
	MM_SFX_KITTEN,
	MM_SFX_SWEET,
	MM_SFX_HEART,
	MM_SFX_KEY,
	MM_SFX_FEATHER,
	MM_SFX_LEVER,
	MM_SFX_CANDLE,
	MM_SFX_ALL_LIT,    // the last candle of a set
	MM_SFX_GATE,       // a way opening
	MM_SFX_UNLOCK,
	MM_SFX_LOCKED,
	MM_SFX_LANTERN,
	MM_SFX_EXTRA_LIFE,
	MM_SFX_BONUS,      // each part of the tally at the end of a night
	// Held
	MM_SFX_FLOAT,
	MM_SFX_SOUNDS
} MmSfx_e;

typedef enum {
	MM_TUNE_TITLE = 0, // the title, who lives here and the high scores
	MM_TUNE_PLAY,      // quietly, while exploring
	MM_TUNE_NIGHT,     // a night starting
	MM_TUNE_NIGHT_END, // all the kittens safe
	MM_TUNE_GAME_OVER,
	MM_TUNE_HISCORE,   // putting in initials
	MM_TUNES
} MmTune_e;

// After the display has started
void mm_sound_init(void);

// Waits for the next frame, and moves the sounds on a frame
static inline void mm_wait_for_frame(void) {
	hstx_dvi_sprite_wait_for_frame();
	game_sound_frame();
}

#ifdef __cplusplus
}
#endif
