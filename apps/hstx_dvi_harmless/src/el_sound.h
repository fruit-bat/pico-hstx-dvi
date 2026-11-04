// Copyright (c) 2024 fruit-bat.

// Sounds and music. A sound is played once, or held: held, it goes on while
// sound_hold says so each frame, and stops the first frame nobody does, so a
// stop is never missed. A tune plays until stopped, fading out, or until
// another is played, which waits for it to fade. See game_seq.h for how
// they are played, and el_sound_data.c for what they are.
#pragma once

#include <stdbool.h>
#include <stdint.h>

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
#define FADE_QUICK 60
#define FADE_SLOW 240

void sound_init(void);
void sound_play(Sfx_e s);
void sound_hold(Sfx_e s, bool on);
// Held, with a level from 0 to 1
void sound_hold_level(Sfx_e s, bool on, float level);
void music_play(Tune_e t);
void music_stop(uint32_t fade);
// The sound test, in the options: every sound, then every tune, by name. A
// sound played once plays again; a held sound or a tune goes on until
// played again, another is tried, or the test stops, which puts back the
// tune that was playing before.
#define SOUND_TESTS (SFX_SOUNDS + TUNES)
extern const char* const sound_test_names[SOUND_TESTS];
void sound_test(uint32_t i);
void sound_test_stop(void);
// Once a frame, from screen_end: lets go of what wasn't held this frame
void sound_frame(void);

#ifdef __cplusplus
}
#endif
