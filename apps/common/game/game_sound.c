// See game_sound.h.
#include "game_sound.h"
#include "game_synth.h"
#include <stddef.h>

static const GameSounds_t* _s;

// Held this frame, and playing because held, a bit a sound
static uint32_t _held;
static uint32_t _holding;

// The tune the game last asked for, or -1 for none, to go back to after a test
static int32_t _tune = -1;
// What the sound test is playing, or -1
static int32_t _test = -1;
static bool _music = true;

void game_sound_init(const GameSounds_t* sounds) {
	_s = sounds;
	game_synth_init();
	game_seq_init();
}

void game_sound_play(uint32_t s) {
	if (s >= _s->sounds_n) return;
	game_seq_sound(&_s->sounds[s], s);
}

void game_sound_hold(uint32_t s, bool on) {
	if (!on || s >= _s->sounds_n) return;
	const uint32_t bit = 1u << s;
	_held |= bit;
	if (_holding & bit) return;
	_holding |= bit;
	game_seq_sound(&_s->sounds[s], s);
}

void game_sound_hold_level(uint32_t s, bool on, float level) {
	game_sound_hold(s, on);
	if (!on || s >= _s->sounds_n) return;
	level = level < 0.0f ? 0.0f : level > 1.0f ? 1.0f : level;
	game_seq_sound_level(s, (uint8_t)(level * 127.0f));
}

// Whether the sound test is trying a tune
static bool testing_tune(void) {
	return _test >= (int32_t)_s->sounds_n;
}

void game_music_play(uint32_t t) {
	if (t >= _s->tunes_n) return;
	_tune = (int32_t)t;
	if (_music && !testing_tune()) game_seq_tune(&_s->tunes[t], GAME_FADE_QUICK);
}

void game_music_stop(uint32_t fade) {
	_tune = -1;
	if (!testing_tune()) game_seq_tune_stop(fade);
}

bool game_music_on(void) {
	return _music;
}

void game_music_set_on(bool on) {
	if (on == _music) return;
	_music = on;
	// A tune being tried goes on, and what comes back after it is decided then
	if (testing_tune()) return;
	if (on && _tune >= 0) game_seq_tune(&_s->tunes[_tune], GAME_FADE_QUICK);
	else if (!on) game_seq_tune_stop(GAME_FADE_QUICK);
}

uint32_t game_sound_tests(void) {
	return _s->sounds_n + _s->tunes_n;
}

const char* game_sound_test_name(uint32_t i) {
	return i < game_sound_tests() && _s->names ? _s->names[i] : "";
}

// Whether a sound goes on until stopped
static bool loops(const SeqSound_t* s) {
	for (const SeqStep_t* p = s->steps; p->op != SEQ_END; ++p) {
		if (p->op == SEQ_LOOP) return true;
	}
	return false;
}

// Whether a test goes on until stopped: a sound that loops, or a tune
static bool test_held(uint32_t i) {
	return i >= _s->sounds_n || loops(&_s->sounds[i]);
}

void game_sound_test(uint32_t i) {
	if (i >= game_sound_tests()) return;
	// Held, and tried again, it stops
	const bool again = (int32_t)i == _test && test_held(i);
	game_sound_test_stop();
	if (again) return;
	if (i < _s->sounds_n) {
		const SeqSound_t* s = &_s->sounds[i];
		game_seq_sound(s, i);
		// One with a level part way in
		if (s->level != SEQ_LEVEL_NONE) game_seq_sound_level(i, 96);
	}
	else {
		game_seq_tune(&_s->tunes[i - _s->sounds_n], GAME_FADE_QUICK);
	}
	_test = (int32_t)i;
}

void game_sound_test_stop(void) {
	if (_test < 0) return;
	const uint32_t t = (uint32_t)_test;
	_test = -1;
	if (t >= _s->sounds_n) {
		if (_music && _tune >= 0) game_seq_tune(&_s->tunes[_tune], GAME_FADE_QUICK);
		else game_seq_tune_stop(GAME_FADE_QUICK);
	}
	// Those played once are left to finish
	else if (test_held(t)) {
		game_seq_sound_stop(t);
	}
}

void game_sound_frame(void) {
	const uint32_t let_go = _holding & ~_held;
	for (uint32_t s = 0; let_go >> s; ++s) {
		if (let_go & (1u << s)) game_seq_sound_stop(s);
	}
	_holding &= _held;
	_held = 0;
#ifdef GAME_HOST
	for (uint32_t i = 0; i < SEQ_HZ / 60; ++i) game_seq_tick();
#endif
}
