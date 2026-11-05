// Renders each of a game's sounds and tunes, as the sequencer plays them, to
// a MIDI file of its own, to listen to on the PC. Built with the game's
// sounds, its src/*sound*.c, which give game_sounds, by render_sounds.sh.
//
// render_sounds <out dir> [name]
//
// Only those whose names have name in them, if given. A sound that loops is
// held for three seconds; a tune is played once through, or for a minute if
// it never ends. The files are named sfx_<n>_<name>.mid and tune_<n>_<name>.mid.

#include "game_sound.h"
#include "game_seq.h"
#include "game_synth.h"
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

extern const GameSounds_t game_sounds;
const char* host_mode = "render_sounds";

#define HOLD (3 * SEQ_HZ)
#define LONGEST (60 * SEQ_HZ)
// Left to die away at the end
#define TAIL (SEQ_HZ / 2)

static void ticks(uint32_t n) {
	while (n--) game_seq_tick();
}

// Ticks to a track's end, or LONGEST if it loops
static uint32_t length(const SeqStep_t* s) {
	uint32_t t = 0;
	for (;; ++s) {
		t += s->wait;
		if (s->op == SEQ_LOOP) return LONGEST;
		if (s->op == SEQ_END) return t;
	}
}

static void save(const char* dir, const char* kind, uint32_t i, const char* name) {
	char path[512];
	int n = snprintf(path, sizeof(path), "%s/%s_%02u_", dir, kind, (unsigned)i);
	for (const char* c = name; *c && n < (int)sizeof(path) - 5; ++c) {
		path[n++] = isalnum((unsigned char)*c) ? (char)tolower((unsigned char)*c) : '_';
	}
	strcpy(path + n, ".mid");
	game_synth_file_save(path);
	printf("%s\n", path);
}

static bool wanted(const char* name, const char* filter) {
	return !filter || strstr(name, filter);
}

int main(int argc, char** argv) {
	if (argc < 2) {
		fprintf(stderr, "render_sounds <out dir> [name]\n");
		return 1;
	}
	const char* dir = argv[1];
	const char* filter = argc > 2 ? argv[2] : NULL;
	const GameSounds_t* g = &game_sounds;
	game_synth_init();
	for (uint32_t i = 0; i < g->sounds_n; ++i) {
		const char* name = g->names ? g->names[i] : "";
		if (!wanted(name, filter)) continue;
		game_seq_init();
		game_seq_sound(&g->sounds[i], i);
		// One with a level part way in, as the sound test plays it
		if (g->sounds[i].level != SEQ_LEVEL_NONE) game_seq_sound_level(i, 96);
		for (uint32_t t = 0; t < HOLD && game_seq_sound_playing(i); ++t) game_seq_tick();
		game_seq_sound_stop(i);
		ticks(TAIL);
		save(dir, "sfx", i, name);
	}
	for (uint32_t i = 0; i < g->tunes_n; ++i) {
		const char* name = g->names ? g->names[g->sounds_n + i] : "";
		if (!wanted(name, filter)) continue;
		const SeqTune_t* tune = &g->tunes[i];
		uint32_t n = 0;
		for (uint32_t k = 0; k < tune->n; ++k) {
			const uint32_t l = length(tune->tracks[k].steps);
			if (l > n) n = l;
		}
		game_seq_init();
		game_seq_tune(tune, 0);
		// To the tick before its end, which would start it again, and on to
		// its end without the sequencer
		ticks(n - 1);
		game_synth_tick();
		game_seq_tune_stop(0);
		ticks(TAIL);
		save(dir, "tune", i, name);
	}
	return 0;
}
