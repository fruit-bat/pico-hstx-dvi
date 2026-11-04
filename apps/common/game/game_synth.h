#pragma once

// A synth that takes MIDI messages: built with GAME_MIDI, a General MIDI
// chip such as a SAM2695 or a VS1053b on a UART's TX, at 31250 baud; on the
// PC, a MIDI file the sequencer's ticks are timed in; otherwise nothing.

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void game_synth_init(void);
// A message, its length from its status: two bytes for a program or channel
// pressure, else three. Never waits: on the Pico it goes out by interrupt.
void game_synth_send(uint8_t status, uint8_t a, uint8_t b);
// A system exclusive message, from its F0 to its F7
void game_synth_sysex(const uint8_t* data, uint32_t n);
// One of the sequencer's ticks has gone by
void game_synth_tick(void);

#ifdef __cplusplus
}
#endif
