#include "def_human.h"
#include "def_world.h"
#include "def_render.h"
#include "def_pallet.h"
#include "def_player.h"
#include "def_scanner.h"
#include "def_hud.h"
#include "def_fx.h"
#include "def_util.h"
#include "def_art.h"

#define DEF_HUMAN_GRAVITY 20
#define DEF_HUMAN_MAX_FALL DEF_PX(3)
// Falling further than this is fatal
#define DEF_HUMAN_SAFE_FALL 120

typedef enum {
	DEF_HS_DEAD = 0,
	DEF_HS_GROUND,
	DEF_HS_CARRIED,
	DEF_HS_FALLING,
	DEF_HS_CAUGHT
} DefHumanState_e;

typedef struct {
	uint8_t state;
	int8_t dir;
	int32_t x, y; // world x, screen y, fixed point
	int32_t vy;
	int32_t fall_y; // where it started falling from, in pixels
} DefHuman_t;

static DefHuman_t _humans[DEF_HUMAN_MAX];

static const char* const _art_human[] = {
	"..KK............",
	".KKKK...........",
	"..KK............",
	".GGGG...........",
	"GGGGGG..........",
	"G.GG.G..........",
	"G.GG.G..........",
	"..GG............",
	".PPPP...........",
	".P..P...........",
	".P..P...........",
	"PP..PP..........",
};
static const uint8_t _human_colours[] = { DEF_PINK, DEF_GREEN, DEF_PURPLE };
// Waving for help when in trouble
static const uint8_t _human_alarm[] = { DEF_WHITE, DEF_YELLOW, DEF_PURPLE };

static DefArt_t _human;
static bool _art_made = false;

static __force_inline int32_t standing_y(const int32_t x) {
	return DEF_PX(def_world_ground_y(x + DEF_PX(DEF_HUMAN_W >> 1)) - DEF_HUMAN_H);
}

void def_human_init(uint32_t count) {
	if (!_art_made) {
		def_art_make(&_human, _art_human, DEF_ART_ROWS(_art_human), "KGP", _human_colours);
		_art_made = true;
	}
	for (uint32_t i = 0; i < DEF_HUMAN_MAX; ++i) {
		DefHuman_t* h = &_humans[i];
		if (i >= count) {
			h->state = DEF_HS_DEAD;
			continue;
		}
		h->state = DEF_HS_GROUND;
		h->dir = (def_rand() & 1) ? 1 : -1;
		h->x = def_wrap(DEF_PX(i * (DEF_WORLD_W / count) + def_rand() % 200));
		h->y = standing_y(h->x);
		h->vy = 0;
	}
}

static void land(DefHuman_t* h, const uint32_t points) {
	h->state = DEF_HS_GROUND;
	h->vy = 0;
	h->y = standing_y(h->x);
	def_hud_add_score(points);
	def_fx_popup(h->x, (h->y >> DEF_FP) - 12, points);
}

void __not_in_flash_func(def_human_update)(uint32_t frame) {
	for (uint32_t i = 0; i < DEF_HUMAN_MAX; ++i) {
		DefHuman_t* h = &_humans[i];
		switch (h->state) {
			case DEF_HS_GROUND: {
				// Wander about
				if ((frame & 7) == 0) h->x = def_wrap(h->x + DEF_PX(h->dir));
				if ((def_rand() & 255) == 0) h->dir = -h->dir;
				h->y = standing_y(h->x);
				break;
			}
			case DEF_HS_FALLING: {
				h->vy += DEF_HUMAN_GRAVITY;
				if (h->vy > DEF_HUMAN_MAX_FALL) h->vy = DEF_HUMAN_MAX_FALL;
				h->y += h->vy;
				if (def_player_hit(h->x, h->y >> DEF_FP, DEF_HUMAN_W, DEF_HUMAN_H)) {
					h->state = DEF_HS_CAUGHT;
					def_hud_add_score(DEF_HUMAN_CATCH_SCORE);
					def_fx_popup(h->x, (h->y >> DEF_FP) - 12, DEF_HUMAN_CATCH_SCORE);
				}
				else if (h->y >= standing_y(h->x)) {
					if (def_world_ground_y(h->x) - h->fall_y > DEF_HUMAN_SAFE_FALL) {
						def_human_kill(i);
					}
					else {
						land(h, DEF_HUMAN_LAND_SCORE);
					}
				}
				break;
			}
			case DEF_HS_CAUGHT: {
				if (!def_player_alive()) {
					def_human_release(i);
					break;
				}
				// Hangs below the ship until it is put down
				h->x = def_wrap(def_player_x() + DEF_PX((DEF_SHIP_W - DEF_HUMAN_W) >> 1));
				h->y = DEF_PX(def_player_y() + DEF_SHIP_H);
				if (h->y >= standing_y(h->x)) land(h, DEF_HUMAN_RETURN_SCORE);
				break;
			}
			default: {
				break;
			}
		}
	}
}

void __not_in_flash_func(def_human_draw)(uint32_t frame) {
	for (uint32_t i = 0; i < DEF_HUMAN_MAX; ++i) {
		const DefHuman_t* h = &_humans[i];
		if (h->state == DEF_HS_DEAD) continue;
		const int32_t sx = def_world_screen_x(h->x);
		if (sx <= -8 || sx >= DEF_SCREEN_W) continue;
		const bool alarm = (h->state == DEF_HS_CARRIED || h->state == DEF_HS_FALLING) && (frame & 8);
		def_art_draw(&_human, sx, h->y >> DEF_FP, alarm ? _human_alarm : NULL);
	}
}

void def_human_scan(void) {
	for (uint32_t i = 0; i < DEF_HUMAN_MAX; ++i) {
		const DefHuman_t* h = &_humans[i];
		if (h->state != DEF_HS_DEAD) def_scanner_dot(h->x, h->y >> DEF_FP, DEF_PINK);
	}
}

uint32_t def_human_alive(void) {
	uint32_t n = 0;
	for (uint32_t i = 0; i < DEF_HUMAN_MAX; ++i) {
		if (_humans[i].state != DEF_HS_DEAD) ++n;
	}
	return n;
}

int32_t def_human_pick(void) {
	uint32_t n = 0;
	for (uint32_t i = 0; i < DEF_HUMAN_MAX; ++i) {
		if (_humans[i].state == DEF_HS_GROUND) ++n;
	}
	if (!n) return -1;
	uint32_t k = def_rand() % n;
	for (uint32_t i = 0; i < DEF_HUMAN_MAX; ++i) {
		if (_humans[i].state == DEF_HS_GROUND && k-- == 0) return i;
	}
	return -1;
}

bool def_human_on_ground(int32_t i) {
	return i >= 0 && i < DEF_HUMAN_MAX && _humans[i].state == DEF_HS_GROUND;
}

int32_t def_human_x(int32_t i) {
	return _humans[i].x;
}

int32_t def_human_y(int32_t i) {
	return _humans[i].y;
}

void def_human_grab(int32_t i) {
	_humans[i].state = DEF_HS_CARRIED;
}

void def_human_carry(int32_t i, int32_t x, int32_t y) {
	if (i < 0 || _humans[i].state != DEF_HS_CARRIED) return;
	_humans[i].x = x;
	_humans[i].y = y;
}

void def_human_release(int32_t i) {
	if (i < 0) return;
	DefHuman_t* h = &_humans[i];
	if (h->state != DEF_HS_CARRIED && h->state != DEF_HS_CAUGHT) return;
	h->state = DEF_HS_FALLING;
	h->vy = 0;
	h->fall_y = h->y >> DEF_FP;
}

void def_human_kill(int32_t i) {
	if (i < 0) return;
	DefHuman_t* h = &_humans[i];
	if (h->state == DEF_HS_DEAD) return;
	h->state = DEF_HS_DEAD;
	def_fx_explode(def_wrap(h->x + DEF_PX(DEF_HUMAN_W >> 1)), h->y + DEF_PX(DEF_HUMAN_H >> 1), DEF_PINK, 20, DEF_PX(2));
}

void def_human_draw_icon(int32_t sx, int32_t sy) {
	def_art_draw(&_human, sx, sy, NULL);
}
