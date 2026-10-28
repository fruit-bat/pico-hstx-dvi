#include "inv_util.h"

#define INV_RAND_DEFAULT_SEED 0x2545f491

static uint32_t _rand_state = INV_RAND_DEFAULT_SEED;

// A quarter of a sine wave scaled to 127, 0 to 64 inclusive
static const int8_t _sin_q[65] = {
	0, 3, 6, 9, 12, 16, 19, 22, 25, 28, 31, 34, 37, 40, 43, 46,
	49, 51, 54, 57, 60, 63, 65, 68, 71, 73, 76, 78, 81, 83, 85, 88,
	90, 92, 94, 96, 98, 100, 102, 104, 106, 107, 109, 111, 112, 113, 115, 116,
	117, 118, 120, 121, 122, 122, 123, 124, 125, 125, 126, 126, 126, 127, 127, 127,
	127
};

void inv_rand_seed(uint32_t seed) {
	_rand_state = seed ? seed : INV_RAND_DEFAULT_SEED;
}

// xorshift32
uint32_t __not_in_flash_func(inv_rand)(void) {
	uint32_t x = _rand_state;
	x ^= x << 13;
	x ^= x >> 17;
	x ^= x << 5;
	_rand_state = x;
	return x;
}

int32_t __not_in_flash_func(inv_sin)(uint32_t a) {
	a &= 255;
	const uint32_t q = a & 63;
	switch (a >> 6) {
		case 0: return _sin_q[q];
		case 1: return _sin_q[64 - q];
		case 2: return -_sin_q[q];
		default: return -_sin_q[64 - q];
	}
}
