// Copyright (c) 2024 fruit-bat.

// See el_sound.h.

#include "el_sound.h"
#include "game_seq.h"
#include "game_synth.h"

extern const SeqSound_t sound_data[SFX_SOUNDS];
extern const SeqTune_t tune_data[TUNES];

// Held this frame, and playing because held
static bool _held[SFX_SOUNDS];
static bool _holding[SFX_SOUNDS];

void sound_init(void) {
	game_synth_init();
	game_seq_init();
}

void sound_play(Sfx_e s) {
	game_seq_sound(&sound_data[s], s);
}

void sound_hold(Sfx_e s, bool on) {
	if (!on) return;
	_held[s] = true;
	if (_holding[s]) return;
	_holding[s] = true;
	game_seq_sound(&sound_data[s], s);
}

void sound_hold_level(Sfx_e s, bool on, float level) {
	sound_hold(s, on);
	if (!on) return;
	level = level < 0.0f ? 0.0f : level > 1.0f ? 1.0f : level;
	game_seq_sound_level(s, (uint8_t)(level * 127.0f));
}

// The tune the game last asked for, or -1 for none, to go back to after a test
static int32_t _tune = -1;
// What the sound test is playing, or -1
static int32_t _test = -1;
static bool _music = true;

void music_play(Tune_e t) {
	_tune = (int32_t)t;
	if (_music && _test < SFX_SOUNDS) game_seq_tune(&tune_data[t], FADE_QUICK);
}

void music_stop(uint32_t fade) {
	_tune = -1;
	if (_test < SFX_SOUNDS) game_seq_tune_stop(fade);
}

bool sound_music(void) {
	return _music;
}

void sound_set_music(bool on) {
	if (on == _music) return;
	_music = on;
	// A tune being tried goes on, and what comes back after it is decided then
	if (_test >= SFX_SOUNDS) return;
	if (on && _tune >= 0) game_seq_tune(&tune_data[_tune], FADE_QUICK);
	else if (!on) game_seq_tune_stop(FADE_QUICK);
}

const char* const sound_test_names[SOUND_TESTS] = {
	[SFX_LASER] = "LASER",
	[SFX_HIT] = "HIT",
	[SFX_EXPLOSION] = "EXPLOSION",
	[SFX_MISSILE] = "MISSILE",
	[SFX_ECM] = "ECM",
	[SFX_BOMB] = "ENERGY BOMB",
	[SFX_SCOOPED] = "SCOOPED",
	[SFX_CHOOSE] = "CHOOSE",
	[SFX_PAGE] = "PAGE",
	[SFX_COUNT] = "COUNTDOWN",
	[SFX_HYPERSPACE] = "HYPERSPACE",
	[SFX_TUNNEL] = "TUNNEL",
	[SFX_DENIED] = "DENIED",
	[SFX_DEAD] = "DEAD",
	[SFX_INCOMING] = "INCOMING",
	[SFX_LOW_ENERGY] = "LOW ENERGY",
	[SFX_HEAT] = "HEAT",
	[SFX_SCOOPING] = "FUEL SCOOP",
	[SFX_OVERDRIVE] = "OVERDRIVE",
	[SFX_SOUNDS + TUNE_TITLE] = "TITLE TUNE",
	[SFX_SOUNDS + TUNE_DANUBE] = "DOCKING TUNE",
};

void sound_test(uint32_t i) {
	if (i >= SOUND_TESTS) return;
	// A held sound or a tune, played again, stops
	const bool again = (int32_t)i == _test && i >= SFX_INCOMING;
	sound_test_stop();
	if (again) return;
	if (i < SFX_SOUNDS) {
		game_seq_sound(&sound_data[i], i);
		// Overdrive half way in
		if (sound_data[i].level != SEQ_LEVEL_NONE) game_seq_sound_level(i, 96);
	}
	else {
		game_seq_tune(&tune_data[i - SFX_SOUNDS], FADE_QUICK);
	}
	_test = (int32_t)i;
}

void sound_test_stop(void) {
	if (_test < 0) return;
	const int32_t t = _test;
	_test = -1;
	// Those played once are left to finish
	if (t >= SFX_INCOMING && t < SFX_SOUNDS) game_seq_sound_stop((uint32_t)t);
	if (t >= SFX_SOUNDS) {
		if (_music && _tune >= 0) game_seq_tune(&tune_data[_tune], FADE_QUICK);
		else game_seq_tune_stop(FADE_QUICK);
	}
}

void sound_frame(void) {
	for (uint32_t s = 0; s < SFX_SOUNDS; ++s) {
		if (_holding[s] && !_held[s]) {
			game_seq_sound_stop(s);
			_holding[s] = false;
		}
		_held[s] = false;
	}
#ifdef GAME_HOST
	// On the PC, time is the frames, at 60 a second
	for (uint32_t i = 0; i < SEQ_HZ / 60; ++i) game_seq_tick();
#endif
}
