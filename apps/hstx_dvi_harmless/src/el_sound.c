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

void music_play(Tune_e t) {
	game_seq_tune(&tune_data[t], FADE_QUICK);
}

void music_stop(uint32_t fade) {
	game_seq_tune_stop(fade);
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
