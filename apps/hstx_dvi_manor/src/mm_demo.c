#include "mm_demo.h"
#include "mm_input.h"
#include "mm_map.h"
#include "mm_witch.h"
#include "mm_thing.h"
#include "game_util.h"

// The demo player follows a route through the house, a step at a time.
// Tiles are given by column, and row where it matters.
enum {
	OP_END = 0,
	OP_WALK, // To column a
	OP_JUMP, // Landing at column a
	OP_FLOAT, // The same, holding jump to float
	OP_LIGHT, // Casts across at the lever or candle at a, b until it's on
	OP_JUMP_LIGHT, // The same, casting from the top of a jump
	OP_CHEER, // Casts across at the nearest creature of type a, once it is near column b (or anywhere if b < 0)
	OP_CHEER_UP, // Casts up at the nearest creature of type a once it is overhead
	OP_WAIT_HIGH, // Waits for the nearest creature of type a to be above row b
	OP_WAIT, // For a frames
	OP_FACE, // Turns to face a
	OP_DROP, // Down through the shelf underfoot
};

typedef struct {
	uint8_t op;
	int16_t a;
	int16_t b;
} MmStep_t;

// Gives up on a step that takes longer than this
#define MM_DEMO_STEP_FRAMES 900
#define MM_DEMO_CAST_GAP 20
#define MM_DEMO_NEAR 3
#define MM_DEMO_LANDED 20
// Not quite as far as a spell goes
#define MM_DEMO_REACH 600
// The size of a spell, see mm_witch.c
#define MM_DEMO_BOLT_R 6

static const MmStep_t _route[] = {
	// The hall: cheer the ghost by the ledge and use it as a step up to the kitten
	{ OP_WALK, 13 },
	{ OP_FACE, 1 },
	{ OP_CHEER, MM_T_GHOST, 16 },
	{ OP_JUMP, 16 },
	{ OP_JUMP, 14 },
	{ OP_WALK, 33 },
	// The library: up the shelves to pull the lever
	{ OP_JUMP, 34 },
	{ OP_JUMP, 38 },
	{ OP_JUMP, 35 },
	{ OP_LIGHT, 54, 27 },
	{ OP_WALK, 42 },
	// The kitchen: three candles, and a kitten on the top shelf
	{ OP_WALK, 61 },
	{ OP_LIGHT, 64, 33 },
	{ OP_CHEER, MM_T_PUMPKIN, -1 },
	{ OP_WALK, 68 },
	{ OP_JUMP_LIGHT, 71, 31 },
	{ OP_WALK, 73 },
	{ OP_JUMP, 75 },
	{ OP_JUMP, 79 },
	{ OP_WALK, 80 },
	{ OP_JUMP, 83 },
	{ OP_LIGHT, 86, 28 },
	{ OP_JUMP, 79 },
	{ OP_WALK, 76 },
	// Down the trapdoor to the cellar, and the crate into the hole by the plate
	{ OP_WALK, 59 },
	{ OP_DROP },
	{ OP_WALK, 54 },
	{ OP_WALK, 45 },
	{ OP_WALK, 33 },
	{ OP_CHEER_UP, MM_T_SPIDER },
	{ OP_WALK, 30 },
	{ OP_JUMP, 27 },
	{ OP_WALK, 22 },
	{ OP_CHEER_UP, MM_T_SPIDER },
	{ OP_WALK, 16 },
	// Back up to the library, and up the shelves to the landing
	{ OP_WALK, 60 },
	{ OP_JUMP, 62 },
	{ OP_JUMP, 58 },
	{ OP_JUMP, 60 },
	{ OP_JUMP, 59 },
	{ OP_JUMP, 57 },
	{ OP_WALK, 33 },
	{ OP_JUMP, 34 },
	{ OP_JUMP, 38 },
	{ OP_JUMP, 35 },
	{ OP_JUMP, 38 },
	{ OP_JUMP, 38 },
	{ OP_JUMP, 41 },
	// The bedroom door, then up to the feather
	{ OP_WALK, 48 },
	{ OP_WALK, 53 },
	{ OP_CHEER_UP, MM_T_BAT },
	{ OP_JUMP, 51 },
	{ OP_WALK, 52 },
	{ OP_JUMP, 56 },
	{ OP_WALK, 57 },
	{ OP_CHEER_UP, MM_T_BAT },
	{ OP_JUMP, 61 },
	{ OP_WALK, 62 },
	// Floating over the gallery's broken floor to a kitten, then up to the attic
	{ OP_WALK, 68 },
	{ OP_WAIT_HIGH, MM_T_SPIDER, 16 },
	{ OP_FLOAT, 78 },
	{ OP_JUMP, 79 },
	{ OP_WALK, 80 },
	{ OP_JUMP, 83 },
	{ OP_JUMP, 80 },
	{ OP_JUMP, 82 },
	{ OP_JUMP, 81 },
	{ OP_JUMP, 79 },
	// The attic: a candle, a float over the hole, another candle and the last kitten
	{ OP_WALK, 72 },
	{ OP_CHEER, MM_T_PUMPKIN, -1 },
	{ OP_CHEER, MM_T_GHOST, -1 },
	{ OP_WALK, 59 },
	{ OP_JUMP_LIGHT, 61, 9 },
	{ OP_WALK, 59 },
	{ OP_FLOAT, 50 },
	{ OP_WALK, 46 },
	{ OP_CHEER, MM_T_PUMPKIN, -1 },
	{ OP_WALK, 40 },
	{ OP_CHEER_UP, MM_T_BAT },
	{ OP_JUMP, 37 },
	{ OP_JUMP_LIGHT, 34, 7 },
	{ OP_WALK, 24 },
	{ OP_END },
};

static uint32_t _step;
static uint32_t _frames;
static bool _airborne;
static uint32_t _last_cast;
static uint8_t _last;
static uint32_t _hold;
static int32_t _last_x;
// Where the jump step started from
static int32_t _jump_y;

void mm_demo_reset(void) {
	_step = 0;
	_frames = 0;
	_airborne = false;
	_last_cast = 0;
	_last = 0;
	_last_x = 0;
}

static int32_t tile_x(int32_t cx) {
	return cx * MM_TILE + MM_TILE / 2;
}

// Heads for x, letting go in time to stop there
static uint8_t steer(int32_t x, int32_t v) {
	const int32_t dx = x - mm_witch_x();
	const int32_t brake = v * v;
	if (dx > MM_DEMO_NEAR + (v > 0 ? brake : 0)) return MM_INP_BIT_R;
	if (dx < -MM_DEMO_NEAR - (v < 0 ? brake : 0)) return MM_INP_BIT_L;
	return 0;
}

static uint8_t face(int32_t dir) {
	if (dir == mm_witch_facing()) return 0;
	return dir < 0 ? MM_INP_BIT_L : MM_INP_BIT_R;
}

// Taps cast every so often
static uint8_t cast(uint8_t extra) {
	if (mm_witch_casting() || _frames - _last_cast < MM_DEMO_CAST_GAP) return 0;
	_last_cast = _frames;
	return MM_INP_BIT_F | extra;
}

// Presses jump and holds it until off the ground, letting go first if it
// was held from before
static uint8_t jump(void) {
	if (!(_last & MM_INP_BIT_J)) {
		_hold = 0;
		return MM_INP_BIT_J;
	}
	return ++_hold < 3 ? MM_INP_BIT_J : 0;
}

// Height of a creature's box, to know whether a spell across will hit it
static int32_t creature_h(int32_t type) {
	switch (type) {
	case MM_T_GHOST: return 36;
	case MM_T_PUMPKIN: return 28;
	default: return 20;
	}
}

static void next(void) {
	++_step;
	_frames = 0;
	_airborne = false;
	_last_cast = 0;
}

uint8_t mm_demo_input(uint32_t frame) {
	const MmStep_t* s = &_route[_step];
	const int32_t x = mm_witch_x(), y = mm_witch_y();
	const int32_t v = x - _last_x;
	_last_x = x;
	uint8_t in = 0;
	if (s->op != OP_END && ++_frames > MM_DEMO_STEP_FRAMES) next();
	switch (s->op) {
	case OP_WALK:
		in = steer(tile_x(s->a), v);
		if (!in && v == 0 && mm_witch_grounded()) next();
		break;
	case OP_JUMP:
	case OP_FLOAT:
		if (_frames == 1) _jump_y = y;
		if (!mm_witch_grounded()) _airborne = true;
		if (!_airborne) {
			in = jump();
		}
		else {
			if (s->op == OP_FLOAT || mm_witch_vy() < 0) in = MM_INP_BIT_J;
			if (mm_witch_grounded() && _frames > 4) {
				// Fell short, so has another go, or walks the rest of the way
				if (y > _jump_y) _airborne = false;
				else if (game_abs(x - tile_x(s->a)) <= MM_DEMO_LANDED) next();
				else in = 0;
			}
		}
		in |= steer(tile_x(s->a), v);
		break;
	case OP_LIGHT:
		if (mm_map_on(s->a, s->b)) {
			next();
			break;
		}
		in = face(game_sign(tile_x(s->a) - x));
		if (!in) in = cast(0);
		break;
	case OP_JUMP_LIGHT:
		if (mm_map_on(s->a, s->b)) {
			next();
			break;
		}
		in = face(game_sign(tile_x(s->a) - x));
		if (in) break;
		if (mm_witch_grounded()) in = jump();
		else if (mm_witch_vy() < 0) in = MM_INP_BIT_J;
		else in = cast(0);
		break;
	case OP_CHEER: {
		int32_t cx, cy;
		bool happy;
		if (!mm_thing_find(s->a, x, y, &cx, &cy, &happy) || happy) {
			next();
			break;
		}
		in = face(game_sign(cx - x));
		if (in) break;
		const int32_t by = y - 32;
		const bool in_line = by + MM_DEMO_BOLT_R > cy - creature_h(s->a) && by - MM_DEMO_BOLT_R < cy && game_abs(cx - x) < MM_DEMO_REACH;
		if (in_line && (s->b < 0 || game_abs(cx - tile_x(s->b)) < 12)) in = cast(0);
		break;
	}
	case OP_CHEER_UP: {
		int32_t cx, cy;
		bool happy;
		if (!mm_thing_find(s->a, x, y, &cx, &cy, &happy) || happy || game_abs(cx - x) > 6 * MM_TILE) {
			next();
			break;
		}
		if (cy < y && game_abs(cx - x) < 12) in = cast(MM_INP_BIT_U);
		break;
	}
	case OP_WAIT_HIGH: {
		int32_t cx, cy;
		bool happy;
		if (!mm_thing_find(s->a, x, y, &cx, &cy, &happy) || happy || cy < s->b * MM_TILE) next();
		break;
	}
	case OP_WAIT:
		if (_frames >= (uint32_t)s->a) next();
		break;
	case OP_DROP:
		if (!mm_witch_grounded()) _airborne = true;
		if (!_airborne) in = jump() | MM_INP_BIT_D;
		else if (mm_witch_grounded()) next();
		break;
	case OP_FACE:
		in = face(s->a);
		if (!in) next();
		break;
	default:
		break;
	}
	_last = in;
	return in;
}

uint32_t mm_demo_step(void) {
	return _step;
}
