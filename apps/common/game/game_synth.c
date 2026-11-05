// See game_synth.h.
#include "game_synth.h"
#include <stddef.h>

extern const GameSynthDriver_t game_synth_uart;
extern const GameSynthDriver_t game_synth_file;

static void none_start(void) {
}

static void none_send(uint8_t status, uint8_t a, uint8_t b) {
	(void)status;
	(void)a;
	(void)b;
}

static void none_sysex(const uint8_t* data, uint32_t n) {
	(void)data;
	(void)n;
}

static const GameSynthDriver_t _none = { "NONE", none_start, none_start, none_send, none_sysex, NULL };

const GameSynthDriver_t* const game_synth_drivers[] = {
#ifdef GAME_MIDI
	&game_synth_uart,
#endif
#ifdef GAME_HOST
	&game_synth_file,
#endif
	&_none,
};
const uint32_t game_synth_drivers_n = sizeof(game_synth_drivers) / sizeof(game_synth_drivers[0]);

static uint32_t _i;
static const GameSynthDriver_t* _d = &_none;

void game_synth_init(void) {
	_i = 0;
	_d = game_synth_drivers[0];
	_d->start();
}

uint32_t game_synth_driver(void) {
	return _i;
}

void game_synth_select(uint32_t i) {
	if (i >= game_synth_drivers_n || i == _i) return;
	_d->stop();
	_i = i;
	_d = game_synth_drivers[i];
	_d->start();
}

void game_synth_send(uint8_t status, uint8_t a, uint8_t b) {
	_d->send(status, a, b);
}

void game_synth_sysex(const uint8_t* data, uint32_t n) {
	_d->sysex(data, n);
}

void game_synth_tick(void) {
	if (_d->tick) _d->tick();
}
