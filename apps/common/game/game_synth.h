#pragma once

// The synth the sequencer (game_seq.h) plays on, by MIDI messages, through
// one of the drivers built in, which can be changed while playing:
//
// - MIDI OUT, built with GAME_MIDI: a General MIDI chip such as a SAM2695 or
//   a VS1053b on a UART's TX, at 31250 baud
// - MIDI FILE, on the PC: a MIDI file, timed in the sequencer's ticks
// - NONE: nothing, always there, last
//
// A driver takes its hardware when started and gives it back when stopped.

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	const char* name;
	void (*start)(void);
	void (*stop)(void);
	// A message, its length from its status: two bytes for a program or
	// channel pressure, else three. Never waits.
	void (*send)(uint8_t status, uint8_t a, uint8_t b);
	// A system exclusive message, from its F0 to its F7
	void (*sysex)(const uint8_t* data, uint32_t n);
	// One of the sequencer's ticks has gone by, or NULL
	void (*tick)(void);
} GameSynthDriver_t;

// The drivers built in
extern const GameSynthDriver_t* const game_synth_drivers[];
extern const uint32_t game_synth_drivers_n;

// Starts the first driver, the best built in
void game_synth_init(void);
// The driver playing, as its place in game_synth_drivers
uint32_t game_synth_driver(void);
// Stops the driver playing and starts another. Called with interrupts off,
// as the sequencer does, so no message goes to a driver half started.
void game_synth_select(uint32_t i);

void game_synth_send(uint8_t status, uint8_t a, uint8_t b);
void game_synth_sysex(const uint8_t* data, uint32_t n);
void game_synth_tick(void);

#ifdef GAME_HOST
// The MIDI FILE driver's messages so far to a file, then forgotten; what is
// left is saved to <mode>.mid at the end
void game_synth_file_save(const char* name);
#endif

// For the drivers: a message's length from its status
static inline uint32_t game_synth_message_length(uint8_t status) {
	const uint8_t kind = status & 0xf0;
	return kind == 0xc0 || kind == 0xd0 ? 2 : 3;
}

#ifdef __cplusplus
}
#endif
