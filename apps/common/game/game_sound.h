#pragma once

// A game's sounds and music, on the sequencer (game_seq.h), known by their
// place in the game's lists. A sound is played once, or held: held, it goes
// on while game_sound_hold says so each frame, and stops the first frame
// nobody does, so a stop is never missed. A tune plays until stopped, fading
// out, or until another is played, which waits for it to fade. Music can be
// turned off, an option, and the sounds still play.
//
// There is a sound test, for the options: every sound, then every tune, by
// name. A sound played once plays again; one that loops, or a tune, goes on
// until tried again, another is tried, or the test stops, which puts back the
// tune that was playing before.

#include "game_seq.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Fading out, in the sequencer's ticks
#define GAME_FADE_QUICK 60
#define GAME_FADE_SLOW 240

// Up to this many sounds
#define GAME_SOUNDS_MAX 32

typedef struct {
	const SeqSound_t* sounds;
	uint32_t sounds_n;
	const SeqTune_t* tunes;
	uint32_t tunes_n;
	// For the sound test: the sounds', then the tunes'
	const char* const* names;
} GameSounds_t;

// Starts the synth and the sequencer, after the display has set the clocks,
// which a MIDI UART's baud rate comes from
void game_sound_init(const GameSounds_t* sounds);
void game_sound_play(uint32_t s);
void game_sound_hold(uint32_t s, bool on);
// Held, with a level from 0 to 1, as its SeqLevel_e says
void game_sound_hold_level(uint32_t s, bool on, float level);
// The game's tune: played only while music is on
void game_music_play(uint32_t t);
void game_music_stop(uint32_t fade);
bool game_music_on(void);
void game_music_set_on(bool on);
// Sounds then tunes
uint32_t game_sound_tests(void);
const char* game_sound_test_name(uint32_t i);
void game_sound_test(uint32_t i);
void game_sound_test_stop(void);
// Once a frame: lets go of what wasn't held this frame, and on the PC moves
// the sequencer on a frame, at 60 a second
void game_sound_frame(void);

#ifdef __cplusplus
}
#endif
