// Copyright (c) 2024 fruit-bat.

// The start and end of each frame: the title at the top, the meter above it
// showing how well the renderer is keeping up, and waiting for the display.
#pragma once

#include "el_draw.h"

#ifdef __cplusplus
extern "C" {
#endif

void screen_init(void);
void screen_begin(void);
// Adds the title, and the meter with label for what is shown, then waits
// for the display to take the frame
void screen_end(const char* title, const char* label);

// A line of text in a colour from game_pallet, in the font as it is or
// smoothed at twice the size, from a pool kept until the renderer has drawn
// it. Text draws straight into the row, so add it after what it goes over.
// Returns how wide it is.
int32_t screen_text(int32_t x, int32_t y, const char* s, uint32_t colour, bool big);
int32_t screen_text_centre(int32_t y, const char* s, uint32_t colour, bool big);
// Text centred on x, kept on the screen
int32_t screen_text_centre_on(int32_t x, int32_t y, const char* s, uint32_t colour, bool big);
// Marks the chosen line of a list, of small text at x, y and w across, with a
// bar behind it and arrows either side pointing at it. As text draws straight
// into the row, it goes before the text of the line.
void screen_select(int32_t x, int32_t y, int32_t w);

#ifdef __cplusplus
}
#endif
