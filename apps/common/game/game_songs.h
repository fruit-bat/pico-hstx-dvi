// Copyright (c) 2024 fruit-bat.

// A small library of songs, as tunes for the sequencer (game_seq.h), for any
// game to use. Each is its tracks; GAME_SONG_<NAME> makes a SeqTune_t of it,
// looping, for a game's table of tunes:
//
//   [MY_TUNE_ATTRACT] = GAME_SONG_SAUCER_MARCH,
//
// The songs use MIDI channels 1 to 5 and the drums, at the sequencer's 120
// ticks a second. A game links in only those it uses.
#pragma once

#include "game_seq.h"

#ifdef __cplusplus
extern "C" {
#endif

// A menacing parade in D minor: whistle for a theremin over tuba, brass
// stabs and a snare, 120 a minute, 16 seconds round
extern const SeqTrack_t game_song_saucer_march[4];
#define GAME_SONG_SAUCER_MARCH { game_song_saucer_march, 4, true }

// Surf rock down D minor, C, B flat, A: tremolo-picked guitar in reverb,
// organ on the off-beats, driving bass, 150 a minute, 13 seconds round
extern const SeqTrack_t game_song_atomic_surf[4];
#define GAME_SONG_ATOMIC_SURF { game_song_atomic_surf, 4, true }

// Baroque precision in G major, round the circle of fifths: harpsichord
// sixteenths, pizzicato bass, a recorder and a woodblock going tick tock,
// 120 a minute, 32 seconds round
extern const SeqTrack_t game_song_clockwork_invention[4];
#define GAME_SONG_CLOCKWORK_INVENTION { game_song_clockwork_invention, 4, true }

// A music box drifting in 6/8, F major, over a harp, strings and a cello,
// 80 dotted crotchets a minute, 24 seconds round
extern const SeqTrack_t game_song_zero_g_lullaby[4];
#define GAME_SONG_ZERO_G_LULLABY { game_song_zero_g_lullaby, 4, true }

// Stately and orchestral in E flat major: a horn over strings and double
// basses, a choir joining for the second half, timpani, 32 seconds round
extern const SeqTrack_t game_song_grand_approach[6];
#define GAME_SONG_GRAND_APPROACH { game_song_grand_approach, 6, true }

// 80s synthwave in A minor: pumping octave bass, a polysynth pad and
// arpeggio, a saw lead from bar 5, gated snare, 129 a minute, 30 seconds round
extern const SeqTrack_t game_song_night_orbit[5];
#define GAME_SONG_NIGHT_ORBIT { game_song_night_orbit, 5, true }

#ifdef __cplusplus
}
#endif
