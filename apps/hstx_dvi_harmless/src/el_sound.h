// Copyright (c) 2024 fruit-bat.

// Sounds and music, Harmless's names for them. See game_sound.h for how they
// are played, and el_sound_data.c for what they are.
#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "game_sound.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	// Played once
	SFX_LASER = 0,
	SFX_HIT,       // a hit on our shields
	SFX_EXPLOSION,
	SFX_MISSILE,   // ours, launched
	SFX_ECM,
	SFX_BOMB,
	SFX_SCOOPED,   // cargo
	SFX_CHOOSE,    // bought, sold, or chosen on a page
	SFX_PAGE,      // a story's page comes up
	SFX_COUNT,     // a second of a countdown to hyperspace
	SFX_HYPERSPACE,
	SFX_TUNNEL,    // launching or docking
	SFX_DENIED,    // can't do that
	SFX_DEAD,
	// Held
	SFX_INCOMING,  // a missile coming for us
	SFX_LOW_ENERGY,
	SFX_HEAT,      // the cabin too hot
	SFX_SCOOPING,  // fuel, from the sun
	SFX_OVERDRIVE, // its level how far into it
	SFX_SOUNDS
} Sfx_e;

typedef enum {
	TUNE_TITLE = 0,
	TUNE_DANUBE, // for the docking computer
	TUNES
} Tune_e;

// Fading out, in the sequencer's ticks
#define FADE_QUICK GAME_FADE_QUICK
#define FADE_SLOW GAME_FADE_SLOW

void sound_init(void);
static inline void sound_play(Sfx_e s) { game_sound_play(s); }
static inline void sound_hold(Sfx_e s, bool on) { game_sound_hold(s, on); }
// Held, with a level from 0 to 1
static inline void sound_hold_level(Sfx_e s, bool on, float level) { game_sound_hold_level(s, on, level); }
// The game's tune: played only while music is on
static inline void music_play(Tune_e t) { game_music_play(t); }
static inline void music_stop(uint32_t fade) { game_music_stop(fade); }
// Music on or off, an option: the sounds play either way
static inline bool sound_music(void) { return game_music_on(); }
static inline void sound_set_music(bool on) { game_music_set_on(on); }
// The sound test, in the options: every sound, then every tune, by name
#define SOUND_TESTS (SFX_SOUNDS + TUNES)
extern const char* const sound_test_names[SOUND_TESTS];
static inline void sound_test(uint32_t i) { game_sound_test(i); }
static inline void sound_test_stop(void) { game_sound_test_stop(); }
// Once a frame, from screen_end: lets go of what wasn't held this frame
static inline void sound_frame(void) { game_sound_frame(); }

#ifdef __cplusplus
}
#endif
