#include "mm_camera.h"
#include "mm_map.h"
#include "game_util.h"

// How far ahead to look, and where to keep the feet from the top of the view
#define MM_CAM_AHEAD 96
#define MM_CAM_FEET (MM_VIEW_H * 5 / 8)
// Follows when the feet get this close to the top or bottom of the view
#define MM_CAM_MARGIN 96
#define MM_CAM_MAX_SPEED GAME_PX(12)

static int32_t _x, _y; // fixed point
static int32_t _ahead;
static int32_t _focus_y;

static int32_t clamp_x(int32_t x) {
	return game_clamp(x, 0, GAME_PX(MM_MAP_PX_W - MM_VIEW_W));
}

static int32_t clamp_y(int32_t y) {
	return game_clamp(y, 0, GAME_PX(MM_MAP_PX_H - MM_VIEW_H));
}

void mm_camera_reset(int32_t x, int32_t y) {
	_ahead = 0;
	_focus_y = y;
	_x = clamp_x(GAME_PX(x - MM_VIEW_W / 2));
	_y = clamp_y(GAME_PX(y - MM_CAM_FEET));
}

static int32_t ease(int32_t from, int32_t to, int32_t shift) {
	return from + game_clamp((to - from) >> shift, -MM_CAM_MAX_SPEED, MM_CAM_MAX_SPEED);
}

void mm_camera_follow(int32_t x, int32_t y, int32_t facing, bool grounded) {
	_ahead += (GAME_PX(facing * MM_CAM_AHEAD) - _ahead) >> 5;
	// Only moves up and down for a jump once it has landed, unless it's
	// getting near the edge of the view
	const int32_t sy = y - (_y >> GAME_FP);
	if (grounded || sy > MM_VIEW_H - MM_CAM_MARGIN || sy < MM_CAM_MARGIN) _focus_y = y;
	_x = clamp_x(ease(_x, GAME_PX(x - MM_VIEW_W / 2) + _ahead, 3));
	_y = clamp_y(ease(_y, GAME_PX(_focus_y - MM_CAM_FEET), 3));
}

void mm_camera_glide(int32_t x, int32_t y, int32_t speed) {
	const int32_t tx = clamp_x(GAME_PX(x - MM_VIEW_W / 2));
	const int32_t ty = clamp_y(GAME_PX(y - MM_VIEW_H / 2));
	_x += game_clamp(tx - _x, -speed, speed);
	_y += game_clamp(ty - _y, -speed, speed);
}

int32_t mm_camera_x(void) {
	return _x >> GAME_FP;
}

int32_t mm_camera_y(void) {
	return _y >> GAME_FP;
}

int32_t mm_screen_y(int32_t y) {
	return y - (_y >> GAME_FP) + MM_HUD_H;
}

static int32_t fx_screen_x(int32_t x) {
	return (x >> GAME_FP) - (_x >> GAME_FP);
}

static int32_t fx_wrap(int32_t x) {
	return x;
}

static int32_t fx_screen_y(int32_t y) {
	return mm_screen_y(y >> GAME_FP);
}

static const GameFxView_t _fx_view = { fx_screen_x, fx_wrap, MM_HUD_H, GAME_SCREEN_H, fx_screen_y };

const GameFxView_t* mm_camera_fx_view(void) {
	return &_fx_view;
}
