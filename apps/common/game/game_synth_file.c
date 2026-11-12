// The MIDI FILE driver (game_synth.h), on the PC: a MIDI file, <mode>.mid,
// written as the program ends, timed in the sequencer's ticks.
#include "game_synth.h"

#ifdef GAME_HOST

#include "game_seq.h"
#include <stdio.h>
#include <stdlib.h>

extern const char* host_mode;

static uint8_t* _mid;
static size_t _n, _max;
// Ticks since the last message
static uint32_t _delta;
static int _saving;

static void put(uint8_t b) {
	if (_n == _max) {
		_max = _max ? _max * 2 : 4096;
		_mid = realloc(_mid, _max);
	}
	_mid[_n++] = b;
}

// A number in sevens, the top first, all but the last with the top bit set
static void put_number(uint32_t v) {
	uint8_t b[5];
	uint32_t n = 0;
	do {
		b[n++] = v & 0x7f;
		v >>= 7;
	} while (v);
	while (n--) put(b[n] | (n ? 0x80 : 0));
}

static void put_be(FILE* f, uint32_t v, uint32_t bytes) {
	while (bytes--) fputc((int)((v >> (8 * bytes)) & 0xff), f);
}

// One track, with a tick a quarter note's SEQ_HZ / 2th, as the tempo is the
// usual half a second a quarter note
static void write(const char* name) {
	if (!_n) return;
	FILE* f = fopen(name, "wb");
	if (!f) return;
	static const uint8_t end[] = { 0x00, 0xff, 0x2f, 0x00 };
	fwrite("MThd", 1, 4, f);
	put_be(f, 6, 4);
	put_be(f, 0, 2);
	put_be(f, 1, 2);
	put_be(f, SEQ_HZ / 2, 2);
	fwrite("MTrk", 1, 4, f);
	put_be(f, (uint32_t)(_n + sizeof(end)), 4);
	fwrite(_mid, 1, _n, f);
	fwrite(end, 1, sizeof(end), f);
	fclose(f);
}

void game_synth_file_save(const char* name) {
	write(name);
	_n = 0;
	_delta = 0;
}

static void save(void) {
	char name[64];
	snprintf(name, sizeof(name), "%s.mid", host_mode);
	write(name);
}

// Saved at the end, whichever driver is playing then
static void start(void) {
	if (!_saving) atexit(save);
	_saving = 1;
}

static void stop(void) {
}

static void send(uint8_t status, uint8_t a, uint8_t b) {
	put_number(_delta);
	_delta = 0;
	put(status);
	put(a);
	if (game_synth_message_length(status) == 3) put(b);
}

static void sysex(const uint8_t* data, uint32_t n) {
	if (n < 2) return;
	put_number(_delta);
	_delta = 0;
	put(0xf0);
	put_number(n - 1);
	for (uint32_t i = 1; i < n; ++i) put(data[i]);
}

static void tick(void) {
	++_delta;
}

const GameSynthDriver_t game_synth_file = { "MIDI FILE", start, stop, send, sysex, tick };

#endif
