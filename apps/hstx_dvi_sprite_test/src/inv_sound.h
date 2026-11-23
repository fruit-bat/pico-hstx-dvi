// Copyright (c) 2024 fruit-bat.

// Sounds and music, the invaders' names for them. See game_sound.h for how
// they are played, and inv_sound_data.c for what they are.
#pragma once

#include "game_sound.h"
#include "hstx_dvi_sprite.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	// Played once
	INV_SFX_SHOOT = 0,
	INV_SFX_HIT,         // an invader shot
	INV_SFX_ARMOUR,      // a commander hit, but not yet killed
	INV_SFX_MARCH_1,     // the formation's footsteps, four notes round
	INV_SFX_MARCH_2,
	INV_SFX_MARCH_3,
	INV_SFX_MARCH_4,
	INV_SFX_DIVE,        // an invader leaving the formation
	INV_SFX_BASE,        // a bit knocked off a base
	INV_SFX_MYSTERY_HIT,
	INV_SFX_DEAD,        // the gun blown up
	INV_SFX_LANDED,      // the invaders reaching the gun
	INV_SFX_EXTRA_LIFE,
	// Held
	INV_SFX_MYSTERY,     // the mystery ship, while it flies over
	INV_SFX_SOUNDS
} InvSfx_e;

typedef enum {
	INV_TUNE_TITLE = 0, // between games, taking turns with the next two
	INV_TUNE_MARCH,
	INV_TUNE_SURF,
	INV_TUNE_WAVE,      // a wave starting
	INV_TUNE_GAME_OVER,
	INV_TUNE_HISCORE,   // putting in initials
	INV_TUNES
} InvTune_e;

// After the display has started
void inv_sound_init(void);

// Waits for the next frame, and moves the sounds on a frame
static inline void inv_wait_for_frame(void) {
	hstx_dvi_sprite_wait_for_frame();
	game_sound_frame();
}

#ifdef __cplusplus
}
#endif
