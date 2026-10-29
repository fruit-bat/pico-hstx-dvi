#include "inv_invaders.h"
#include "inv_score.h"
#include "inv_collisions.h"
#include "inv_bombs.h"
#include "inv_base.h"
#include "inv_gun.h"
#include "inv_mot.h"
#include "inv_level.h"
#include "game_art.h"
#include "game_fx.h"
#include "game_pallet.h"
#include "game_util.h"

#define INV_INVADER_COLS 8
#define INV_INVADER_ROWS 5
#define INV_INVADER_COUNT (INV_INVADER_COLS * INV_INVADER_ROWS)
#define INV_INVADER_GAP_X 44
#define INV_INVADER_GAP_Y 24
// How far the invaders step down each time they reach the side of the screen
#define INV_INVADER_DROP 8
// How close the formation gets to the sides of the screen
#define INV_INVADER_EDGE 8
// Frames before the first bomb of a wave
#define INV_INVADER_FIRE_DELAY 90
// Frames before the first dive of a wave
#define INV_INVADER_DIVE_DELAY 300
// Frames from being shot until the invader is gone
#define INV_INVADER_EXPLODE 12
// Frames a commander flashes for when it is hit but not killed
#define INV_INVADER_FLASH 8
// How far off to the side the gun has to be for the invaders to look at it
#define INV_INVADER_LOOK 24
// How a diver takes off, how fast it falls and how hard it steers for the gun
#define INV_DIVE_RISE GAME_PX(2)
#define INV_DIVE_GRAVITY 10
#define INV_DIVE_STEER 12
#define INV_DIVE_SIDEWAYS GAME_PX(2)
// Speed a diver flies back to the formation at, at least
#define INV_DIVE_RETURN GAME_PX(3)

typedef enum  {
	INV_STATE_WALK = 0,
	INV_STATE_DIVE,
	INV_STATE_RETURN,
	INV_STATE_EXPLODE,
	INV_STATE_DEAD
} InvInvaderState_e;

typedef struct {
	uint8_t state;
	uint8_t type;
	uint8_t hits;
	// So they do not all blink together
	uint8_t phase;
	uint8_t flash;
	// Left to drop on this dive
	uint8_t bombs;
	uint16_t timer;
	// Fixed point, for when it is out of the formation
	int32_t x, y;
	int32_t vx, vy;
} InvInvader_t;

static const uint8_t _row_type[INV_INVADER_ROWS] = {
	INV_ALIEN_COMMANDER, INV_ALIEN_CRAB, INV_ALIEN_CRAB, INV_ALIEN_JELLY, INV_ALIEN_JELLY
};
static const uint16_t _type_score[INV_INVADER_TYPE_COUNT] = { 40, 20, 10 };
static const char* const _type_name[INV_INVADER_TYPE_COUNT + 1] = { "COMMANDER", "CRAB", "JELLY", "MYSTERY" };

// A commander that has been hit once
static const uint8_t _damaged[INV_ALIEN_LAYERS] = { GAME_RED, GAME_WHITE, GAME_ORANGE };
static const uint8_t _flash[INV_ALIEN_LAYERS] = { GAME_WHITE, GAME_WHITE, GAME_WHITE };

static SpriteId _inv_index = 0;
static InvInvader_t _inv[INV_INVADER_COUNT];
// Top left of the formation
static int32_t _form_x = 0;
static int32_t _form_y = 0;
static int32_t inv_v = 1;
// Lowest walking invader in each column, or -1
static int32_t _inv_lowest[INV_INVADER_COLS];
// Sub pixel march position, in 1/16 pixel
static uint32_t _inv_sub = 0;
// March speed last frame, in 1/16 pixel per frame
static uint32_t _inv_speed = 0;
static uint32_t _inv_alive = 0;
static uint32_t _inv_exploding = 0;
static uint32_t _inv_divers = 0;
static bool _inv_landed = false;
static uint32_t _fire_timer = 0;
static uint32_t _dive_timer = 0;
static bool _art_made = false;

static __force_inline int32_t slot_x(const uint32_t i) {
	return _form_x + (int32_t)(i % INV_INVADER_COLS) * INV_INVADER_GAP_X;
}

static __force_inline int32_t slot_y(const uint32_t i) {
	return _form_y + (int32_t)(i / INV_INVADER_COLS) * INV_INVADER_GAP_Y;
}

static __force_inline SpriteId invader_sprite(const uint32_t i) {
	return _inv_index + i * INV_ALIEN_LAYERS;
}

// Where it is on the screen, in pixels
static void invader_pos(const InvInvader_t* v, const uint32_t i, int32_t* x, int32_t* y) {
	if (v->state == INV_STATE_WALK) {
		*x = slot_x(i);
		*y = slot_y(i);
	}
	else {
		*x = v->x >> GAME_FP;
		*y = v->y >> GAME_FP;
	}
}

const char* inv_invaders_type_name(uint32_t type) {
	return _type_name[type <= INV_INVADER_TYPE_MOT ? type : INV_INVADER_TYPE_MOT];
}

uint32_t inv_invaders_type_score(uint32_t type) {
	return type < INV_INVADER_TYPE_COUNT ? _type_score[type] : 0;
}

// Eyes on the gun, with the odd blink
static uint32_t invader_look(const int32_t x, const uint32_t phase, const uint32_t frame) {
	if (((frame + phase) & 127) < 5) return INV_ALIEN_BLINK;
	const int32_t d = inv_gun_get_x() + (INV_GUN_W >> 1) - (x + (INV_ALIEN_W >> 1));
	if (d < -INV_INVADER_LOOK) return INV_ALIEN_LOOK_LEFT;
	if (d > INV_INVADER_LOOK) return INV_ALIEN_LOOK_RIGHT;
	return INV_ALIEN_LOOK_AHEAD;
}

void inv_invaders_draw_icon(uint32_t type, int32_t x, int32_t y, uint32_t frame) {
	if (type >= INV_INVADER_TYPE_MOT) {
		inv_mot_draw_icon(x, y, frame);
		return;
	}
	if (!_art_made) {
		inv_alien_init();
		_art_made = true;
	}
	// Looking about, as there is no gun to watch
	static const uint8_t looks[4] = { INV_ALIEN_LOOK_LEFT, INV_ALIEN_LOOK_AHEAD, INV_ALIEN_LOOK_RIGHT, INV_ALIEN_LOOK_AHEAD };
	const uint32_t look = ((frame + type * 40) & 127) < 5 ? INV_ALIEN_BLINK : looks[(frame >> 5) & 3];
	game_art_draw(inv_alien_art(type, frame >> 3, look), x, y, NULL);
}

SpriteId inv_invaders_init(SpriteId start) {
	if (!_art_made) {
		inv_alien_init();
		_art_made = true;
	}
	_inv_index = start;
	_form_x = INV_INVADER_EDGE;
	_form_y = inv_level_invader_top();

	for (uint32_t i = 0; i < INV_INVADER_COUNT; ++i) {
		InvInvader_t* v = &_inv[i];
		v->state = INV_STATE_WALK;
		v->type = _row_type[i / INV_INVADER_COLS];
		v->hits = 0;
		v->phase = game_rand();
		v->flash = 0;
		v->bombs = 0;
		v->timer = 0;
		const SpriteId si = invader_sprite(i);
		for (uint32_t l = 0; l < INV_ALIEN_LAYERS; ++l) {
			init_sprite(si + l, 0, 0, 0, 0, 0, NULL, NULL, NULL);
			hstx_dvi_sprite_set_sprite_collision_mask(si + l, INV_INVADER_COLLISION_MASK);
		}
		game_art_sprites(si, INV_ALIEN_LAYERS, inv_alien_art(v->type, 0, INV_ALIEN_LOOK_AHEAD), slot_x(i), slot_y(i), NULL);
	}
	for (uint32_t x = 0; x < INV_INVADER_COLS; ++x) {
		_inv_lowest[x] = -1;
	}

	// Set the invaders to move right
	inv_v = 1;
	_inv_sub = 0;
	_inv_speed = 0;
	_inv_alive = INV_INVADER_COUNT;
	_inv_exploding = 0;
	_inv_divers = 0;
	_inv_landed = false;
	_fire_timer = INV_INVADER_FIRE_DELAY;
	_dive_timer = INV_INVADER_DIVE_DELAY;

	return start + INV_INVADER_COUNT * INV_ALIEN_LAYERS;
}

// The march speeds up as the invaders are thinned out, and the last one really goes for it
static uint32_t __not_in_flash_func(march_speed)(void) {
	uint32_t s = inv_level_invader_speed();
	s += ((INV_INVADER_COUNT - _inv_alive) * 24) / INV_INVADER_COUNT;
	if (_inv_alive <= 4) s += 8;
	if (_inv_alive <= 1) s += 24;
	return s;
}

static void invader_kill(InvInvader_t* v, const uint32_t i, const int32_t x, const int32_t y, const bool award) {
	const int32_t h = inv_alien_height(v->type);
	const int32_t cx = GAME_PX(x + (INV_ALIEN_W >> 1));
	const int32_t cy = GAME_PX(y + (h >> 1));
	game_fx_explode(cx, cy, inv_alien_colour(v->type), 20, GAME_PX(2));
	game_fx_explode(cx, cy, inv_alien_detail(v->type), 8, GAME_PX(1));
	if (award) {
		// Worth double when caught out of the formation
		const bool diving = v->state != INV_STATE_WALK;
		const uint32_t score = _type_score[v->type] << (diving ? 1 : 0);
		inv_score_add(score);
		if (diving || v->type == INV_ALIEN_COMMANDER) game_fx_popup(cx - GAME_PX(8), y, score);
	}
	v->state = INV_STATE_EXPLODE;
	v->timer = INV_INVADER_EXPLODE;
	game_art_sprites_hide(invader_sprite(i), INV_ALIEN_LAYERS);
}

// Returns true if the invader was killed. Commanders take two hits.
static bool invader_shot(InvInvader_t* v, const uint32_t i, const int32_t x, const int32_t y) {
	if (v->type == INV_ALIEN_COMMANDER && v->hits == 0) {
		v->hits = 1;
		v->flash = INV_INVADER_FLASH;
		game_fx_explode(GAME_PX(x + (INV_ALIEN_W >> 1)), GAME_PX(y + 6), GAME_WHITE, 6, GAME_PX(1));
		return false;
	}
	invader_kill(v, i, x, y, true);
	return true;
}

// Picks a commander or crab still in the formation to come down after the gun
static void start_dive(void) {
	const uint32_t first = game_rand() % INV_INVADER_COUNT;
	for (uint32_t k = 0; k < INV_INVADER_COUNT; ++k) {
		uint32_t i = first + k;
		if (i >= INV_INVADER_COUNT) i -= INV_INVADER_COUNT;
		InvInvader_t* v = &_inv[i];
		if (v->state != INV_STATE_WALK || v->type == INV_ALIEN_JELLY) continue;
		v->state = INV_STATE_DIVE;
		v->x = GAME_PX(slot_x(i));
		v->y = GAME_PX(slot_y(i));
		v->vx = 0;
		// Up out of the formation first
		v->vy = -INV_DIVE_RISE;
		v->bombs = inv_level_dive_bombs();
		return;
	}
}

static void update_dive(InvInvader_t* v) {
	if (!inv_gun_is_alive()) {
		v->state = INV_STATE_RETURN;
		return;
	}
	const int32_t gx = GAME_PX(inv_gun_get_x() + (INV_GUN_W >> 1));
	v->vx = game_clamp(v->vx + game_sign(gx - (v->x + GAME_PX(INV_ALIEN_W >> 1))) * INV_DIVE_STEER, -INV_DIVE_SIDEWAYS, INV_DIVE_SIDEWAYS);
	v->vy = game_clamp(v->vy + INV_DIVE_GRAVITY, -INV_DIVE_RISE, inv_level_dive_speed());
	v->x = game_clamp(v->x + v->vx, 0, GAME_PX(MODE_H_ACTIVE_PIXELS - INV_ALIEN_W));
	v->y += v->vy;
	const int32_t y = v->y >> GAME_FP;
	// Bombs on the way down, while there is still time to get out of the way
	if (v->bombs && v->vy > 0 && y < inv_gun_get_y() - 120 && (game_rand() & 15) == 0) {
		inv_bombs_fire((v->x >> GAME_FP) + (INV_ALIEN_W >> 1) - 4, y + inv_alien_height(v->type), v->type);
		--v->bombs;
	}
	// Off the bottom and round to the top again
	if (y > MODE_V_ACTIVE_LINES) {
		v->y = -GAME_PX(16);
		v->state = INV_STATE_RETURN;
	}
}

static void update_return(InvInvader_t* v, const uint32_t i) {
	// Fast enough to catch up with the formation
	const int32_t speed = INV_DIVE_RETURN + GAME_PX(_inv_speed >> 4);
	const int32_t dx = GAME_PX(slot_x(i)) - v->x;
	const int32_t dy = GAME_PX(slot_y(i)) - v->y;
	if (game_abs(dx) <= speed && game_abs(dy) <= speed) {
		v->state = INV_STATE_WALK;
		return;
	}
	v->x += game_clamp(dx, -speed, speed);
	v->y += game_clamp(dy, -speed, speed);
}

// Picks a column to drop a bomb from, either the one nearest the gun or one at random
static int32_t __not_in_flash_func(pick_bomb_column)(void) {
	if ((game_rand() & 255) < inv_level_aim_chance()) {
		const int32_t gx = inv_gun_get_x();
		int32_t best = -1;
		int32_t best_d = INT32_MAX;
		for (uint32_t x = 0; x < INV_INVADER_COLS; ++x) {
			if (_inv_lowest[x] < 0) continue;
			const int32_t d = game_abs(slot_x(_inv_lowest[x]) - gx);
			if (d < best_d) {
				best_d = d;
				best = x;
			}
		}
		return best;
	}
	const uint32_t first = game_rand() % INV_INVADER_COLS;
	for (uint32_t i = 0; i < INV_INVADER_COLS; ++i) {
		uint32_t x = first + i;
		if (x >= INV_INVADER_COLS) x -= INV_INVADER_COLS;
		if (_inv_lowest[x] >= 0) return x;
	}
	return -1;
}

void __not_in_flash_func(inv_invader_update)(uint32_t frame) {

	// The formation holds still while the gun is being replaced
	const bool frozen = _inv_landed || !inv_gun_is_alive();
	int32_t dx = 0;
	if (!frozen) {
		_inv_speed = march_speed();
		_inv_sub += _inv_speed;
		dx = (int32_t)(_inv_sub >> 4) * inv_v;
		_inv_sub &= 15;
	}
	_form_x += dx;

	for (uint32_t i = 0; i < INV_INVADER_COLS; ++i) {
		_inv_lowest[i] = -1;
	}

	uint32_t alive = 0;
	uint32_t exploding = 0;
	uint32_t divers = 0;
	int32_t left = INT32_MAX;
	int32_t right = INT32_MIN;
	int32_t bottom = 0;
	for (uint32_t i = 0; i < INV_INVADER_COUNT; ++i) {
		InvInvader_t* v = &_inv[i];
		if (v->state == INV_STATE_DEAD) continue;
		if (v->state == INV_STATE_EXPLODE) {
			if (--v->timer == 0) v->state = INV_STATE_DEAD;
			else ++exploding;
			continue;
		}

		// What it ran in to last frame
		const SpriteId si = invader_sprite(i);
		SpriteCollisionMask m = 0;
		for (uint32_t l = 0; l < INV_ALIEN_LAYERS; ++l) m |= _spriteCollisions.m[si + l];
		int32_t x, y;
		invader_pos(v, i, &x, &y);
		const int32_t h = inv_alien_height(v->type);
		if ((m & INV_BULLET_COLLISION_MASK) && invader_shot(v, i, x, y)) {
			++exploding;
			continue;
		}
		// A diver that flies in to the gun goes too
		if ((m & INV_GUN_COLLISION_MASK) && v->state != INV_STATE_WALK) {
			invader_kill(v, i, x, y, false);
			++exploding;
			continue;
		}
		if (m & INV_BASE_COLLISION_MASKS) {
			inv_base_invader_hit(x, y, INV_ALIEN_W, h, m);
		}
		++alive;

		switch (v->state) {
			case INV_STATE_WALK: {
				if (x < left) left = x;
				if (x + INV_ALIEN_W > right) right = x + INV_ALIEN_W;
				if (y + h > bottom) bottom = y + h;
				// Rows are visited top to bottom, so the last one seen is the lowest
				_inv_lowest[i % INV_INVADER_COLS] = i;
				break;
			}
			case INV_STATE_DIVE: {
				update_dive(v);
				++divers;
				break;
			}
			case INV_STATE_RETURN: {
				update_return(v, i);
				++divers;
				break;
			}
			default: {
				break;
			}
		}
	}

	if ((dx > 0 && right >= MODE_H_ACTIVE_PIXELS - INV_INVADER_EDGE) || (dx < 0 && left <= INV_INVADER_EDGE)) {
		inv_v = -inv_v;
		_form_y += INV_INVADER_DROP;
		bottom += INV_INVADER_DROP;
	}
	_inv_alive = alive;
	_inv_exploding = exploding;
	_inv_divers = divers;

	if (right > left && bottom >= inv_gun_get_y()) {
		_inv_landed = true;
	}

	// Draw them where they have got to. The marchers step as the formation moves.
	const uint32_t step = (_form_x >> 3) & 3;
	for (uint32_t i = 0; i < INV_INVADER_COUNT; ++i) {
		InvInvader_t* v = &_inv[i];
		if (v->state >= INV_STATE_EXPLODE) continue;
		int32_t x, y;
		invader_pos(v, i, &x, &y);
		const uint32_t anim = v->state == INV_STATE_WALK ? step : frame >> 3;
		const uint8_t* colours = v->flash ? _flash : v->hits ? _damaged : NULL;
		if (v->flash) --v->flash;
		game_art_sprites(invader_sprite(i), INV_ALIEN_LAYERS, inv_alien_art(v->type, anim, invader_look(x, v->phase, frame)), x, y, colours);
	}

	if (frozen || !alive) return;

	if (_fire_timer) {
		--_fire_timer;
	}
	else if (inv_bombs_active() < inv_level_bomb_max()) {
		const int32_t x = pick_bomb_column();
		if (x >= 0) {
			const uint32_t i = _inv_lowest[x];
			inv_bombs_fire(slot_x(i) + (INV_ALIEN_W >> 1) - 4, slot_y(i) + inv_alien_height(_inv[i].type), _inv[i].type);
		}
		const uint32_t interval = inv_level_bomb_interval();
		_fire_timer = (interval >> 1) + (game_rand() % interval);
	}

	if (_dive_timer) {
		--_dive_timer;
	}
	else {
		if (divers < inv_level_dive_max()) start_dive();
		const uint32_t interval = inv_level_dive_interval();
		_dive_timer = (interval >> 1) + (game_rand() % interval);
	}
}

uint32_t __not_in_flash_func(inv_invaders_alive)(void) {
	return _inv_alive;
}

bool inv_invaders_cleared(void) {
	return !_inv_alive && !_inv_exploding;
}

bool inv_invaders_landed(void) {
	return _inv_landed;
}

bool __not_in_flash_func(inv_invaders_aim)(const int32_t gx, int32_t* tx) {
	// Divers on the way down come first
	for (uint32_t i = 0; i < INV_INVADER_COUNT; ++i) {
		const InvInvader_t* v = &_inv[i];
		if (v->state != INV_STATE_DIVE || v->vy <= 0) continue;
		const int32_t y = v->y >> GAME_FP;
		if (y < 160 || y > inv_gun_get_y() - 60) continue;
		const int32_t frames = (inv_gun_get_y() - y) >> 2;
		*tx = ((v->x + v->vx * frames) >> GAME_FP) + (INV_ALIEN_W >> 1);
		return true;
	}
	int32_t best = -1;
	int32_t best_d = INT32_MAX;
	for (uint32_t x = 0; x < INV_INVADER_COLS; ++x) {
		if (_inv_lowest[x] < 0) continue;
		const int32_t d = game_abs(slot_x(_inv_lowest[x]) + (INV_ALIEN_W >> 1) - gx);
		if (d < best_d) {
			best_d = d;
			best = _inv_lowest[x];
		}
	}
	if (best < 0) return false;
	// Lead the target by how far it will march while a bullet climbs up to it
	const int32_t frames = (inv_gun_get_y() - slot_y(best)) >> 2;
	*tx = slot_x(best) + (INV_ALIEN_W >> 1) + ((frames * (int32_t)_inv_speed * inv_v) >> 4);
	return true;
}

bool __not_in_flash_func(inv_invaders_diver_threat)(int32_t x0, int32_t x1, int32_t y0, int32_t* dx) {
	for (uint32_t i = 0; i < INV_INVADER_COUNT; ++i) {
		const InvInvader_t* v = &_inv[i];
		if (v->state != INV_STATE_DIVE) continue;
		const int32_t x = (v->x >> GAME_FP) + (INV_ALIEN_W >> 1);
		const int32_t y = (v->y >> GAME_FP) + inv_alien_height(v->type);
		if (x < x0 - (INV_ALIEN_W >> 1) || x > x1 + (INV_ALIEN_W >> 1) || y < y0) continue;
		*dx = x;
		return true;
	}
	return false;
}
