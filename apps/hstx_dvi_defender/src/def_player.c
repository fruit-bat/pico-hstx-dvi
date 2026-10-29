#include "def_player.h"
#include "def_world.h"
#include "def_render.h"
#include "def_pallet.h"
#include "def_input.h"
#include "def_util.h"
#include "def_fx.h"
#include "def_hud.h"
#include "def_enemy.h"
#include "def_art.h"

#define DEF_SHIP_MAX_VX DEF_PX(10)
#define DEF_SHIP_ACCEL 96
#define DEF_SHIP_VY 4
// Where the ship sits from the edge of the screen it is facing away from
#define DEF_SHIP_OFFSET 140
// How fast the ship swings across the screen when it turns round
#define DEF_SHIP_SWING 10
#define DEF_SHIP_START_Y 220
#define DEF_LASER_COUNT 4
#define DEF_LASER_RANGE 600
#define DEF_LASER_HEAD_SPEED 48
#define DEF_LASER_TAIL_SPEED 28
// Frames before the tail of a beam starts to follow the head
#define DEF_LASER_TAIL_DELAY 3
// Frames between shots while fire is held down
#define DEF_LASER_REPEAT 8
#define DEF_PLAYER_DEAD_FRAMES 150
#define DEF_SMART_FLASH 6
// The exhaust changes colour every 2 to the power of this many frames
#define DEF_FLAME_COLOUR_SHIFT 3

typedef enum {
	DEF_PLAYER_ALIVE = 0,
	DEF_PLAYER_DEAD,
	DEF_PLAYER_OVER
} DefPlayerState_e;

typedef struct {
	bool active;
	int32_t x;    // where it was fired from
	int32_t y;
	int32_t dir;
	int32_t head; // pixels from where it was fired
	int32_t tail;
	uint32_t age;
} DefLaser_t;

static int32_t _x = 0;
static int32_t _y = 0;
static int32_t _vx = 0;
static bool _right = true;
static bool _thrust = false;
static int32_t _offset = DEF_SHIP_OFFSET;
static DefPlayerState_e _state = DEF_PLAYER_ALIVE;
static uint32_t _timer = 0;
static uint32_t _fire_timer = 0;
static DefLaser_t _lasers[DEF_LASER_COUNT];

// Facing right; the left facing ones are made from them
static const char* const _art_ship[] = {
	"WW..............................",
	"WWW.............................",
	"WWWW............CCC.............",
	"WWWWW..........CCCCC............",
	"WWWWWWWWWWWWWWWWWWWWWWWW........",
	"RWWWWWWWWWWWWWWWWWWWWWWWWWWW....",
	"RRPPPPPPPPPPPPPPPPPPPPPWWWWWWWWW",
	"RWWWWWWWWWWWWWWWWWWWWWWWWWWW....",
	"..WWWWWWWWWWWWWWWWWWWWWW........",
	"....GGGGGGGGGGGGGGGGG...........",
};
static const uint8_t _ship_colours[] = { DEF_WHITE, DEF_CYAN, DEF_RED, DEF_PURPLE, DEF_GREY };

// The exhaust, two frames of flicker
static const char* const _art_flame[2][5] = {
	{
		"..........FFFFF.",
		"....FFFFFFFFFFFF",
		"FFFFFFFFFFFFFFFF",
		"....FFFFFFFFFFFF",
		"..........FFFFF.",
	},
	{
		"............FFF.",
		"......FFFFFFFFFF",
		"..FFFFFFFFFFFFFF",
		"......FFFFFFFFFF",
		"............FFF.",
	},
};
static const uint8_t _flame_colours[] = { DEF_YELLOW };

// Right and left facing
static DefArt_t _ship[2];
static DefArt_t _flame[2][2];

// A smaller ship to count the lives with
static Tile16x8p2_t _tile_icon = {{
	0b1100000000000000,
	0b1110000000000000,
	0b1111111110000000,
	0b1111111111111100,
	0b0111111111111111,
	0b0011111111100000,
	0b0000000000000000,
	0b0000000000000000,
}};

static bool _art_made = false;

static void make_art(void) {
	def_art_make(&_ship[0], _art_ship, DEF_ART_ROWS(_art_ship), "WCRPG", _ship_colours);
	def_art_mirror(&_ship[1], &_ship[0]);
	for (uint32_t i = 0; i < 2; ++i) {
		def_art_make(&_flame[0][i], _art_flame[i], DEF_ART_ROWS(_art_flame[i]), "F", _flame_colours);
		def_art_mirror(&_flame[1][i], &_flame[0][i]);
	}
	_art_made = true;
}

void def_player_clear_lasers(void) {
	for (uint32_t i = 0; i < DEF_LASER_COUNT; ++i) _lasers[i].active = false;
}

void def_player_init(void) {
	if (!_art_made) make_art();
	_x = 0;
	_y = DEF_SHIP_START_Y;
	_vx = 0;
	_right = true;
	_thrust = false;
	_offset = DEF_SHIP_OFFSET;
	_state = DEF_PLAYER_ALIVE;
	_fire_timer = 0;
	def_player_clear_lasers();
	def_world_set_camera(_x - DEF_PX(_offset));
}

static void fire_laser(void) {
	for (uint32_t i = 0; i < DEF_LASER_COUNT; ++i) {
		DefLaser_t* l = &_lasers[i];
		if (l->active) continue;
		l->active = true;
		l->dir = _right ? 1 : -1;
		l->x = def_wrap(_x + (_right ? DEF_PX(DEF_SHIP_W) : 0));
		l->y = _y + DEF_SHIP_NOSE;
		l->head = 0;
		l->tail = 0;
		l->age = 0;
		return;
	}
}

static void update_lasers(void) {
	for (uint32_t i = 0; i < DEF_LASER_COUNT; ++i) {
		DefLaser_t* l = &_lasers[i];
		if (!l->active) continue;
		++l->age;
		l->head += DEF_LASER_HEAD_SPEED;
		if (l->head > DEF_LASER_RANGE) l->head = DEF_LASER_RANGE;
		if (l->age > DEF_LASER_TAIL_DELAY) l->tail += DEF_LASER_TAIL_SPEED;
		if (l->tail >= l->head) l->active = false;
	}
}

static void hyperspace(void) {
	def_fx_explode(def_wrap(_x + DEF_PX(DEF_SHIP_W >> 1)), DEF_PX(_y + (DEF_SHIP_H >> 1)), DEF_WHITE, 16, DEF_PX(2));
	_x = def_rand() % DEF_WORLD_W_FP;
	_y = DEF_PLAY_TOP + def_rand() % (DEF_TERRAIN_TOP - DEF_PLAY_TOP);
	_vx = 0;
	def_player_clear_lasers();
	def_fx_implode(def_wrap(_x + DEF_PX(DEF_SHIP_W >> 1)), DEF_PX(_y + (DEF_SHIP_H >> 1)), DEF_WHITE, 16, 20);
}

void __not_in_flash_func(def_player_update)(uint32_t frame, uint8_t input, uint8_t pressed) {
	update_lasers();
	switch (_state) {
		case DEF_PLAYER_ALIVE: {
			// Pushing left or right turns that way and thrusts
			_thrust = false;
			if (input & DEF_INP_BIT_L) {
				_right = false;
				_thrust = true;
				_vx -= DEF_SHIP_ACCEL;
			}
			else if (input & DEF_INP_BIT_R) {
				_right = true;
				_thrust = true;
				_vx += DEF_SHIP_ACCEL;
			}
			else {
				_vx -= _vx / 24;
				if (def_abs(_vx) < 16) _vx = 0;
			}
			_vx = def_clamp(_vx, -DEF_SHIP_MAX_VX, DEF_SHIP_MAX_VX);
			_x = def_wrap(_x + _vx);

			// Up and down together is hyperspace
			if ((input & (DEF_INP_BIT_U | DEF_INP_BIT_D)) == (DEF_INP_BIT_U | DEF_INP_BIT_D)) {
				if (pressed & (DEF_INP_BIT_U | DEF_INP_BIT_D)) hyperspace();
			}
			else if (input & DEF_INP_BIT_U) {
				_y -= DEF_SHIP_VY;
			}
			else if (input & DEF_INP_BIT_D) {
				_y += DEF_SHIP_VY;
			}
			_y = def_clamp(_y, DEF_PLAY_TOP, DEF_SCREEN_H - DEF_SHIP_H - 2);

			if (_fire_timer) --_fire_timer;
			if ((pressed & DEF_INP_BIT_F) || ((input & DEF_INP_BIT_F) && !_fire_timer)) {
				fire_laser();
				_fire_timer = DEF_LASER_REPEAT;
			}

			if ((pressed & DEF_INP_BIT_B) && def_hud_use_smart_bomb()) {
				def_enemy_smart_bomb();
				def_fx_flash(DEF_SMART_FLASH);
			}
			break;
		}
		case DEF_PLAYER_DEAD: {
			if (--_timer == 0) {
				if (def_hud_take_life()) {
					_state = DEF_PLAYER_ALIVE;
					_vx = 0;
					_y = DEF_SHIP_START_Y;
					def_enemy_clear_bullets();
					def_fx_implode(def_wrap(_x + DEF_PX(DEF_SHIP_W >> 1)), DEF_PX(_y + (DEF_SHIP_H >> 1)), DEF_WHITE, 24, 30);
				}
				else {
					_state = DEF_PLAYER_OVER;
				}
			}
			break;
		}
		default: {
			break;
		}
	}

	// The ship swings across the screen to see what is ahead
	const int32_t target = _right ? DEF_SHIP_OFFSET : DEF_SCREEN_W - DEF_SHIP_OFFSET - DEF_SHIP_W;
	_offset += def_clamp(target - _offset, -DEF_SHIP_SWING, DEF_SHIP_SWING);
	def_world_set_camera(_x - DEF_PX(_offset));
}

void __not_in_flash_func(def_player_draw)(uint32_t frame) {
	if (_state != DEF_PLAYER_ALIVE) return;
	const int32_t sx = def_world_screen_x(_x);
	const uint32_t f = _right ? 0 : 1;
	def_art_draw(&_ship[f], sx, _y, NULL);
	if (_thrust) {
		// The colour changes slowly enough to see, while the shape flickers
		const uint8_t c[1] = { ((frame >> DEF_FLAME_COLOUR_SHIFT) & 1) ? DEF_YELLOW : DEF_RED };
		def_art_draw(&_flame[f][(frame >> 1) & 1], _right ? sx - 16 : sx + DEF_SHIP_W, _y + 4, c);
	}
}

void __not_in_flash_func(def_player_draw_lasers)(uint32_t frame) {
	for (uint32_t i = 0; i < DEF_LASER_COUNT; ++i) {
		DefLaser_t* l = &_lasers[i];
		if (!l->active) continue;
		const int32_t len = l->head - l->tail;
		if (len <= 0) continue;
		const int32_t sx = def_world_screen_x(l->x);
		def_spr_laser(l->dir > 0 ? sx + l->tail : sx - l->head, l->y, len, l->dir < 0, def_pallet_rainbow(frame + i));
	}
}

void def_player_kill(void) {
	if (_state != DEF_PLAYER_ALIVE) return;
	_state = DEF_PLAYER_DEAD;
	_timer = DEF_PLAYER_DEAD_FRAMES;
	const int32_t cx = def_wrap(_x + DEF_PX(DEF_SHIP_W >> 1));
	const int32_t cy = DEF_PX(_y + (DEF_SHIP_H >> 1));
	def_fx_explode(cx, cy, DEF_COLOUR_RANDOM, 64, DEF_PX(4));
	def_fx_explode(cx, cy, DEF_WHITE, 32, DEF_PX(2));
	def_player_clear_lasers();
}

bool __not_in_flash_func(def_player_alive)(void) {
	return _state == DEF_PLAYER_ALIVE;
}

bool def_player_over(void) {
	return _state == DEF_PLAYER_OVER;
}

int32_t __not_in_flash_func(def_player_x)(void) {
	return _x;
}

int32_t __not_in_flash_func(def_player_y)(void) {
	return _y;
}

int32_t __not_in_flash_func(def_player_vx)(void) {
	return _vx;
}

bool def_player_facing_right(void) {
	return _right;
}

bool __not_in_flash_func(def_player_hit)(int32_t x, int32_t y, int32_t w, int32_t h) {
	if (_state != DEF_PLAYER_ALIVE) return false;
	const int32_t dx = def_dx(x, _x) >> DEF_FP;
	// A slightly smaller box than the ship, to be fair
	return dx < DEF_SHIP_W - 3 && dx + w > 3 && y < _y + DEF_SHIP_H - 2 && y + h > _y + 2;
}

bool __not_in_flash_func(def_player_laser_hit)(int32_t x, int32_t y, int32_t w, int32_t h) {
	for (uint32_t i = 0; i < DEF_LASER_COUNT; ++i) {
		DefLaser_t* l = &_lasers[i];
		if (!l->active || l->y < y || l->y >= y + h) continue;
		// The box measured along the beam
		const int32_t d = def_dx(x, l->x) >> DEF_FP;
		const int32_t u0 = l->dir > 0 ? d : -d - w;
		const int32_t u1 = u0 + w;
		if (u1 > l->tail && u0 < l->head) {
			l->active = false;
			return true;
		}
	}
	return false;
}

Tile16x8p2_t* def_player_icon(void) {
	return &_tile_icon;
}
