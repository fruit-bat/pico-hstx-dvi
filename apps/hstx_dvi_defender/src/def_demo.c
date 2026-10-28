#include "def_demo.h"
#include "def_input.h"
#include "def_player.h"
#include "def_enemy.h"
#include "def_world.h"
#include "def_util.h"

// Gets in close before opening fire
#define DEF_DEMO_CHASE 260
#define DEF_DEMO_RANGE 560
#define DEF_DEMO_DODGE 72
// Uses a smart bomb when there are this many enemies on the screen
#define DEF_DEMO_CROWD 6

uint8_t __not_in_flash_func(def_demo_input)(uint32_t frame) {
	if (!def_player_alive()) return 0;

	// The middle of the ship, and where its lasers come out
	const int32_t px = def_wrap(def_player_x() + DEF_PX(DEF_SHIP_W >> 1));
	const int32_t py = def_player_y() + DEF_SHIP_NOSE;
	const bool right = def_player_facing_right();
	uint8_t input = 0;

	// Pressed every so often, as it only goes off when the button goes down
	if ((frame & 15) == 0 && def_enemy_on_screen() >= DEF_DEMO_CROWD) {
		input |= DEF_INP_BIT_B;
	}

	int32_t by;
	if (def_enemy_bullet_near(px, py, DEF_DEMO_DODGE, &by)) {
		input |= by < py ? DEF_INP_BIT_D : DEF_INP_BIT_U;
	}

	int32_t ex, ey;
	if (!def_enemy_nearest(px, &ex, &ey)) {
		return input | (right ? DEF_INP_BIT_R : DEF_INP_BIT_L);
	}

	const int32_t dx = def_dx(ex, px) >> DEF_FP;
	const bool want_right = dx > 0;
	if (want_right != right || def_abs(dx) > DEF_DEMO_CHASE) {
		input |= want_right ? DEF_INP_BIT_R : DEF_INP_BIT_L;
	}
	if (!(input & (DEF_INP_BIT_U | DEF_INP_BIT_D))) {
		if (ey < py - 2) input |= DEF_INP_BIT_U;
		else if (ey > py + 2) input |= DEF_INP_BIT_D;
	}
	if (want_right == right && def_abs(ey - py) < 6 && def_abs(dx) < DEF_DEMO_RANGE) {
		input |= DEF_INP_BIT_F;
	}
	return input;
}
