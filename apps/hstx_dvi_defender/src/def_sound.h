// Copyright (c) 2024 fruit-bat.

// Sounds and music, Defender's names for them. See game_sound.h for how they
// are played, and def_sound_data.c for what they are.
#pragma once

#include "game_sound.h"
#include "hstx_dvi_sprite.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	// Played once
	DEF_SFX_LASER = 0,
	DEF_SFX_EXPLODE,     // an enemy shot
	DEF_SFX_ENEMY_FIRE,
	DEF_SFX_SPAWN,       // enemies materialising
	DEF_SFX_GRAB,        // a lander takes a humanoid
	DEF_SFX_FALL,        // a humanoid let go
	DEF_SFX_CATCH,       // and caught
	DEF_SFX_RESCUE,      // and put down
	DEF_SFX_HUMAN_DIE,
	DEF_SFX_MUTANT,      // a lander turned
	DEF_SFX_BAITER,      // one coming
	DEF_SFX_SMART_BOMB,
	DEF_SFX_HYPERSPACE,
	DEF_SFX_DEAD,
	DEF_SFX_RESPAWN,
	DEF_SFX_EXTRA_LIFE,
	DEF_SFX_BONUS,       // a humanoid counted at the end of a wave
	DEF_SFX_PLANET,      // gone
	// Held
	DEF_SFX_THRUST,
	DEF_SFX_SOUNDS
} DefSfx_e;

typedef enum {
	DEF_TUNE_TITLE = 0, // the title and the hall of fame
	DEF_TUNE_WAVE,      // a wave starting
	DEF_TUNE_GAME_OVER,
	DEF_TUNE_HISCORE,   // putting in initials
	DEF_TUNES
} DefTune_e;

// After the display has started
void def_sound_init(void);

// Waits for the next frame, and moves the sounds on a frame
static inline void def_wait_for_frame(void) {
	hstx_dvi_sprite_wait_for_frame();
	game_sound_frame();
}

#ifdef __cplusplus
}
#endif
