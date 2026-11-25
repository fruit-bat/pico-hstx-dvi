// Copyright (c) 2024 fruit-bat.

// See rr_settings.h.

#include "rr_settings.h"
#include "rr_input.h"
#include "rr_sound.h"
#include "game_options.h"

typedef enum {
	RR_FLYING_NONE = 0,
	RR_FLYING_LAND,
	RR_FLYING_WARSHIP,
} RrFlying_e;

static const char* const _tune_names[] = { "RIFT RUNNER", "NIGHT ORBIT", "ATOMIC SURF", "CLOCKWORK" };
static const uint8_t _tunes[] = { RR_TUNE_STAGE, RR_TUNE_NIGHT_ORBIT, RR_TUNE_ATOMIC_SURF, RR_TUNE_CLOCKWORK };
#define RR_FLYING_TUNES (sizeof(_tunes) / sizeof(_tunes[0]))

static const char* const _rounds[] = { "1", "2", "3" };
static const char* const _demo_sound[] = { "ON", "SILENT" };

static uint8_t _tune = 0;
static uint8_t _start_round = 0;
static uint8_t _demo_silent = 0;
static RrFlying_e _flying = RR_FLYING_NONE;

static void play(void) {
	if (_flying == RR_FLYING_NONE) return;
	game_music_play(_flying == RR_FLYING_WARSHIP ? RR_TUNE_BOSS : _tunes[_tune % RR_FLYING_TUNES]);
}

static const GameOption_t _options[] = {
	{ "FLYING TUNE", _tune_names, RR_FLYING_TUNES, &_tune, play },
	{ "START ROUND", _rounds, sizeof(_rounds) / sizeof(_rounds[0]), &_start_round, NULL },
	{ "DEMO SOUND", _demo_sound, 2, &_demo_silent, NULL },
};

void rr_settings_run(void (*draw_back)(uint32_t frame)) {
	const GameOptions_t o = {
		RR_INP_BIT_U, RR_INP_BIT_D, RR_INP_BIT_L, RR_INP_BIT_R, RR_INP_BIT_F, RR_INP_BIT_B, RR_INP_BIT_START,
		rr_wait_for_frame, draw_back, _options, sizeof(_options) / sizeof(_options[0])
	};
	game_options_run(&o);
}

void rr_music_flying(void) {
	_flying = RR_FLYING_LAND;
	play();
}

void rr_music_warship(void) {
	_flying = RR_FLYING_WARSHIP;
	play();
}

void rr_music_landed(void) {
	_flying = RR_FLYING_NONE;
}

uint32_t rr_settings_start_round(void) {
	return _start_round;
}

bool rr_settings_demo_silent(void) {
	return _demo_silent != 0;
}
