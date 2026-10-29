#include "inv_level.h"
#include "game_util.h"

static uint32_t _wave = 1;

static __force_inline uint32_t min_u32(const uint32_t a, const uint32_t b) {
	return a < b ? a : b;
}

void inv_level_reset(void) {
	_wave = 1;
}

void inv_level_next(void) {
	++_wave;
}

uint32_t __not_in_flash_func(inv_level_get)(void) {
	return _wave;
}

// Each wave starts a row lower, up to 5 rows
int32_t inv_level_invader_top(void) {
	return 60 + 16 * min_u32(_wave - 1, 5);
}

uint32_t __not_in_flash_func(inv_level_invader_speed)(void) {
	return 12 + 2 * min_u32(_wave - 1, 10);
}

uint32_t __not_in_flash_func(inv_level_bomb_max)(void) {
	return min_u32(2 + _wave, 8);
}

uint32_t __not_in_flash_func(inv_level_bomb_interval)(void) {
	return 40 - 4 * min_u32(_wave - 1, 8);
}

int32_t __not_in_flash_func(inv_level_bomb_speed)(void) {
	return 2 + min_u32((_wave + 1) >> 1, 3);
}

uint32_t __not_in_flash_func(inv_level_aim_chance)(void) {
	return min_u32(32 + 24 * (_wave - 1), 192);
}

uint32_t inv_level_mot_interval(void) {
	return 900 + (game_rand() % 600);
}

uint32_t __not_in_flash_func(inv_level_dive_interval)(void) {
	return 360 - 30 * min_u32(_wave - 1, 8);
}

uint32_t __not_in_flash_func(inv_level_dive_max)(void) {
	return min_u32(1 + (_wave >> 1), 4);
}

int32_t __not_in_flash_func(inv_level_dive_speed)(void) {
	return GAME_PX(2) + 32 * min_u32(_wave - 1, 6);
}

uint32_t __not_in_flash_func(inv_level_dive_bombs)(void) {
	return min_u32(1 + (_wave >> 2), 3);
}
