#pragma once

// A settings screen for the sprite games, opened from their screens between
// games: music on or off, any settings of the game's own, and a sound test
// (left and right pick a sound or tune, fire plays it). Up and down pick,
// left and right change, and start or B goes back. It is drawn with sprites,
// over whatever the game draws behind it.

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// A setting of the game's own, shown between music and the sound test: one
// of count choices, kept in *value
typedef struct {
	const char* name;
	const char* const* choices;
	uint8_t count;
	uint8_t* value;
	// Called when it changes, or NULL
	void (*changed)(void);
} GameOption_t;

typedef struct {
	// The game's input bits for each button
	uint16_t up, down, left, right, fire, back, start;
	// Waits for the next frame, moving the sounds on
	void (*wait)(void);
	// Adds what goes behind, or NULL. It is added before the settings' text,
	// as text draws straight into the row over what was drawn before it, so
	// sprites of its own should not go where the text does.
	void (*draw_back)(uint32_t frame);
	// The game's own settings, or NULL
	const GameOption_t* extra;
	uint32_t extra_n;
} GameOptions_t;

// Runs the screen until it is left
void game_options_run(const GameOptions_t* o);

#ifdef __cplusplus
}
#endif
