#include "inv_demo.h"
#include "inv_input.h"
#include "inv_gun.h"
#include "inv_bombs.h"
#include "inv_invaders.h"
#include "inv_util.h"
#include "hstx_dvi_core.h"

// How close a bomb has to be before the demo player gets out of the way
#define INV_DEMO_DODGE_WIDTH 14
#define INV_DEMO_DODGE_HEIGHT 100
// Keep away from the edges when dodging, so as not to get stuck
#define INV_DEMO_EDGE 40

static int32_t _aim_error = 0;

void inv_demo_init(void) {
	_aim_error = 0;
}

uint8_t __not_in_flash_func(inv_demo_input)(uint32_t frame) {
	if (!inv_gun_is_alive()) return 0;

	const int32_t gx = inv_gun_get_x() + 8;
	const int32_t gy = inv_gun_get_y();

	// Get out of the way of bombs first
	int32_t bx;
	if (inv_bombs_threat(gx - INV_DEMO_DODGE_WIDTH, gx + INV_DEMO_DODGE_WIDTH, gy - INV_DEMO_DODGE_HEIGHT, &bx)) {
		const bool go_right = (bx < gx && gx < MODE_H_ACTIVE_PIXELS - INV_DEMO_EDGE) || gx < INV_DEMO_EDGE;
		return go_right ? INV_INP_BIT_R : INV_INP_BIT_L;
	}

	// Aim a little off now and then so it does not look too good
	if ((frame & 63) == 0) {
		_aim_error = (int32_t)(inv_rand() % 13) - 6;
	}

	int32_t tx;
	if (!inv_invaders_aim(gx, &tx)) return 0;
	tx += _aim_error;

	uint8_t input = 0;
	if (tx < gx - 1) input |= INV_INP_BIT_L;
	else if (tx > gx + 1) input |= INV_INP_BIT_R;
	if (tx > gx - 6 && tx < gx + 6) input |= INV_INP_BIT_F;
	return input;
}
