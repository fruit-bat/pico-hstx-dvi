#pragma once

#include "pico/stdlib.h"
#include "game_fx.h"

#ifdef __cplusplus
extern "C" {
#endif

// Where the view is in the house. Positions are in pixels unless they say otherwise.

// Puts the view straight on to x, y
void mm_camera_reset(int32_t x, int32_t y);
// Eases towards keeping x, y, the bottom middle of whoever is being
// followed, in view. facing is -1 or 1, to see more of the way they're going.
void mm_camera_follow(int32_t x, int32_t y, int32_t facing, bool grounded);
// Drifts towards x, y as the middle of the view, for the title
void mm_camera_glide(int32_t x, int32_t y, int32_t speed);

// The house pixel at the top left of the view
int32_t mm_camera_x(void);
int32_t mm_camera_y(void);

__force_inline int32_t mm_screen_x(const int32_t x) {
	return x - mm_camera_x();
}
int32_t mm_screen_y(int32_t y);

// For the particles, which are in fixed point house positions
const GameFxView_t* mm_camera_fx_view(void);

#ifdef __cplusplus
}
#endif
