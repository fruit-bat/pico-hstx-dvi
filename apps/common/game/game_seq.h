#pragma once

// The sequencer: plays tunes and sounds, made of steps, as MIDI messages to
// the synth (game_synth.h), ticking SEQ_HZ times a second, on the Pico from a
// timer. Tunes go on MIDI channels 1 to 10, 10 being General MIDI's drums,
// and sounds on 11 to 16, one each, so six at once; a sound with a higher
// priority takes the channel of one with a lower, the oldest first. Tunes
// fade with the channels' volume (control 7), which the steps leave alone,
// and sounds are bent two octaves either way.

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SEQ_HZ 120

// What a step does, after waiting its wait, in ticks, after the step before
typedef enum {
	SEQ_END = 0, // a sound stops; a track is done
	SEQ_REST,    // nothing, for waiting longer
	SEQ_ON,      // note a, b hard
	SEQ_OFF,     // note a let go
	SEQ_PROGRAM, // the instrument, General MIDI's a, from 0
	SEQ_CC,      // control a to b
	SEQ_BEND,    // bent at once to a, from 0 to 127, 64 being none
	SEQ_SWEEP,   // bent to a over b ticks, while the steps go on
	SEQ_MARK,    // where SEQ_LOOP goes back to, else the start
	SEQ_LOOP,
} SeqOp_e;

typedef struct {
	uint8_t wait;
	uint8_t op;
	uint8_t a, b;
} SeqStep_t;

// What a sound's level, given while it plays, changes
typedef enum {
	SEQ_LEVEL_NONE = 0,
	SEQ_LEVEL_BEND,
	SEQ_LEVEL_VOLUME, // its expression, control 11
} SeqLevel_e;

typedef struct {
	const SeqStep_t* steps;
	uint8_t priority;
	uint8_t level; // SeqLevel_e
} SeqSound_t;

// A tune's track plays on a channel from 0, and the tune ends, or starts
// again, when all its tracks are done
typedef struct {
	uint8_t channel;
	const SeqStep_t* steps;
} SeqTrack_t;

typedef struct {
	const SeqTrack_t* tracks;
	uint8_t n;
	bool loop;
} SeqTune_t;

// Resets the synth, and on the Pico starts the timer
void game_seq_init(void);
void game_seq_tick(void);
// A sound, known by id: the same id again starts it again on its channel
void game_seq_sound(const SeqSound_t* s, uint32_t id);
// Lets go of its notes, which then die away
void game_seq_sound_stop(uint32_t id);
bool game_seq_sound_playing(uint32_t id);
// From 0 to 127, as its SeqLevel_e says
void game_seq_sound_level(uint32_t id, uint8_t level);
// A tune, after any other has faded out over fade ticks; the tune playing
// goes on, and comes back if fading
void game_seq_tune(const SeqTune_t* t, uint32_t fade);
// Fades out over fade ticks, or stops at once
void game_seq_tune_stop(uint32_t fade);
// Plays on another of the synth's drivers (game_synth.h) from now on, telling
// it all it needs, and starting the tune playing again
void game_seq_synth(uint32_t i);
// From 0 to 127
void game_seq_volume(uint8_t music, uint8_t sounds);

#ifdef __cplusplus
}
#endif
