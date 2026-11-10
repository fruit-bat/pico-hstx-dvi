#pragma once

// A settings screen for the sprite games, opened from their screens between
// games: music on or off, and a sound test (left and right pick a sound or
// tune, fire plays it). Up and down pick, left and right change, and start
// or B goes back. It is drawn with sprites, over whatever the game draws
// behind it.

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	// The game's input bits for each button
	uint16_t up, down, left, right, fire, back, start;
	// Waits for the next frame, moving the sounds on
	void (*wait)(void);
	// Adds what goes behind, after the settings as later sprites go behind,
	// or NULL
	void (*draw_back)(uint32_t frame);
} GameOptions_t;

// Runs the screen until it is left
void game_options_run(const GameOptions_t* o);

#ifdef __cplusplus
}
#endif
