// Copyright (c) 2024 fruit-bat.

// The options, shown over the screen while everything else waits: the way
// of drawing the view, dithering, the dashboard, the shape of the screen,
// and in a game, the docking computer, MAX EQUIP for trying things out, and
// ending it. They are saved, if there is a card, when left.
#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Shows them, with the in-game ones only in one
void options_open(bool in_game);
void options_close(void);
bool options_shown(void);
// Takes the buttons pressed while they are shown: up and down pick, left and
// right change, and start leaves. Returns true if the game is to end.
bool options_input(uint16_t pressed);
// Whether the docking computer was asked for since last called
bool options_dock(void);
// Adds them, last, as text draws straight into the row
void options_draw(void);

#ifdef __cplusplus
}
#endif
