// Copyright (c) 2024 fruit-bat.

// Sounds and music, Rift Runner's names for them. See game_sound.h for how
// they are played, and rr_sound_data.c for what they are.
#pragma once

#include "game_sound.h"
#include "hstx_dvi_sprite.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	// Played once
	RR_SFX_SHOT = 0,
	RR_SFX_LASER,
	RR_SFX_BEAM,        // the charged beam let go
	RR_SFX_BOMB,        // dropped
	RR_SFX_BLAST,       // a bomb going off
	RR_SFX_HIT,         // something hit that takes more than one
	RR_SFX_EXPLODE,
	RR_SFX_EXPLODE_BIG,
	RR_SFX_LAUNCH,      // a rocket taking off
	RR_SFX_ENEMY_FIRE,
	RR_SFX_CAPSULE,     // picked up
	RR_SFX_POWER,       // a power up taken
	RR_SFX_FUEL,        // a fuel tank hit
	RR_SFX_FUEL_LOW,
	RR_SFX_DEAD,
	RR_SFX_RESPAWN,
	RR_SFX_EXTRA_LIFE,
	RR_SFX_ALARM,       // the warship coming
	RR_SFX_BOSS_HIT,
	RR_SFX_BOSS_DIE,
	RR_SFX_TALLY,       // fuel counted into the score after the warship
	RR_SFX_BUZZ,        // mosquitoes or wasps coming
	RR_SFX_PULSE,       // a jellyfish pushing itself up
	RR_SFX_STING,       // a wasp's sting
	// Held
	RR_SFX_CHARGE,      // the beam charging, rising with it
	RR_SFX_SOUNDS
} RrSfx_e;

typedef enum {
	RR_TUNE_TITLE = 0,  // the title and the hall of fame
	RR_TUNE_STAGE,      // flying, the land sections
	RR_TUNE_BOSS,       // the warship
	RR_TUNE_CLEAR,      // the warship beaten
	RR_TUNE_GAME_OVER,
	RR_TUNE_HISCORE,    // putting in initials
	// Others to fly to, from the songs all the games share, see rr_settings
	RR_TUNE_NIGHT_ORBIT,
	RR_TUNE_ATOMIC_SURF,
	RR_TUNE_CLOCKWORK,
	RR_TUNES
} RrTune_e;

// After the display has started
void rr_sound_init(void);

// Waits for the next frame, and moves the sounds on a frame
static inline void rr_wait_for_frame(void) {
	hstx_dvi_sprite_wait_for_frame();
	game_sound_frame();
}

#ifdef __cplusplus
}
#endif
