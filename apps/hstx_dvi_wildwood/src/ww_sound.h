// Copyright (c) 2024 fruit-bat.

// Sounds and music, Wildwood's names for them. See game_sound.h for how they
// are played, and ww_sound_data.c for what they are.
#pragma once

#include "game_sound.h"
#include "hstx_dvi_sprite.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	WW_SFX_CAST = 0,
	WW_SFX_FRIEND,     // a spell for a happy animal
	WW_SFX_BETTER,     // a spell that helps, but not enough yet
	WW_SFX_HAPPY,      // healed or calmed
	WW_SFX_FOUND,      // the owlet, now following
	WW_SFX_REUNITE,    // and back with its mother
	WW_SFX_WAKE,       // an animal woken up, grumpy
	WW_SFX_YELP,       // the wolf backing off from a spell
	WW_SFX_HURT,       // a heart lost
	WW_SFX_DIZZY,      // the last one
	WW_SFX_PIECE,      // a piece of the moon charm
	WW_SFX_CHARM,      // and the whole charm
	WW_SFX_FLOWER,
	WW_SFX_MUDDLE,     // the purple flower
	WW_SFX_HOWL,       // the wolf coming
	WW_SFX_WOLF,       // and here
	WW_SFX_SNEAK,      // crept past, or got away
	WW_SFX_EXTRA_LIFE,
	WW_SFX_BONUS,      // each part of the tally at the end of a night
	WW_SFX_SOUNDS
} WwSfx_e;

typedef enum {
	WW_TUNE_TITLE = 0, // the title and the high scores
	WW_TUNE_PLAY,      // quietly, while exploring
	WW_TUNE_NIGHT,     // a night starting
	WW_TUNE_NIGHT_END, // the wolf calmed
	WW_TUNE_GAME_OVER,
	WW_TUNE_HISCORE,   // putting in initials
	WW_TUNES
} WwTune_e;

// After the display has started
void ww_sound_init(void);

// Waits for the next frame, and moves the sounds on a frame
static inline void ww_wait_for_frame(void) {
	hstx_dvi_sprite_wait_for_frame();
	game_sound_frame();
}

#ifdef __cplusplus
}
#endif
