#include "def_enemy.h"
#include "def_world.h"
#include "def_render.h"
#include "def_pallet.h"
#include "def_player.h"
#include "def_human.h"
#include "def_hud.h"
#include "def_fx.h"
#include "def_util.h"
#include "def_scanner.h"
#include "def_art.h"

#define DEF_ENEMY_MAX 64
#define DEF_BULLET_MAX 32
// Frames an enemy takes to materialise
#define DEF_ENEMY_SPAWN_FRAMES 40
#define DEF_BULLET_LIFE 150
#define DEF_MINE_LIFE 300
#define DEF_BULLET_SPEED DEF_PX(3)
// Enemies only shoot when this close to the ship
#define DEF_ENEMY_FIRE_RANGE 360
#define DEF_LANDER_SPEED 320
#define DEF_LANDER_DESCEND DEF_PX(1)
#define DEF_LANDER_ASCEND 140
#define DEF_MUTANT_SPEED DEF_PX(3)
#define DEF_SWARMER_SPEED DEF_PX(5)
#define DEF_BAITER_SPEED DEF_PX(4)
#define DEF_BOMBER_SWING 40

typedef enum {
	DEF_ES_SPAWN = 0,
	DEF_ES_SEEK,
	DEF_ES_DESCEND,
	DEF_ES_ASCEND
} DefEnemyState_e;

typedef struct {
	uint8_t type;
	uint8_t state;
	int8_t human;
	uint16_t timer;
	int32_t x, y;   // world x, screen y, fixed point
	int32_t vx, vy;
	int32_t base_y;
	uint32_t anim;
} DefEnemy_t;

typedef struct {
	bool active;
	bool mine;
	uint16_t life;
	int32_t x, y, vx, vy;
} DefBullet_t;

typedef struct {
	uint8_t w;
	uint8_t h;
	uint8_t colour;
	uint8_t detail;
	uint16_t score;
	const char* name;
	// Chance out of 4096 of firing each frame
	uint16_t fire;
	DefArt_t* art;
} DefEnemyInfo_t;

static DefEnemy_t _enemies[DEF_ENEMY_MAX];
static DefBullet_t _bullets[DEF_BULLET_MAX];
static uint32_t _wave = 1;

static const char* const _art_lander[] = {
	"....GGGGGGGG....",
	"..GGGGGGGGGGGG..",
	".GGGRRGGGGRRGGG.",
	".GGRRRRGGRRRRGG.",
	".GGGRRGGGGRRGGG.",
	"..GGGGGGGGGGGG..",
	"...GG.GGGG.GG...",
	"....GGGGGGGG....",
	"...WW......WW...",
	"..WW........WW..",
	".WW..........WW.",
	"WW............WW",
};

static const char* const _art_baiter[] = {
	"...........GGGGGGGGGG...........",
	"......GGGGGGGGGGGGGGGGGGGG......",
	"...GGGGWWGGWWGGWWGGWWGGWWGGGG...",
	"GGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGG",
	"...GGGGGGGGGGGGGGGGGGGGGGGGGG...",
	"......RR....RR....RR....RR......",
};

static const char* const _art_bomber[] = {
	"..BBBBBBBBBBBB..",
	".BBBBBBBBBBBBBB.",
	"BBYYBBBBBBBBYYBB",
	"BBYYBBBBBBBBYYBB",
	"BBBBBB....BBBBBB",
	"BBBBB.YYYY.BBBBB",
	"BBBBB.YYYY.BBBBB",
	"BBBBB.YYYY.BBBBB",
	"BBBBB.YYYY.BBBBB",
	"BBBBBB....BBBBBB",
	"BBYYBBBBBBBBYYBB",
	"BBYYBBBBBBBBYYBB",
	".BBBBBBBBBBBBBB.",
	"..BBBBBBBBBBBB..",
};

static const char* const _art_pod[] = {
	"P......PP......P",
	".P.....PP.....P.",
	"..P..PPPPPP..P..",
	"...PPPPPPPPPP...",
	"...PPRRRRRRPP...",
	"..PPRRWWWWRRPP..",
	"PPPPRWWWWWWRPPPP",
	"PPPPRWWWWWWRPPPP",
	"..PPRRWWWWRRPP..",
	"...PPRRRRRRPP...",
	"...PPPPPPPPPP...",
	"..P..PPPPPP..P..",
	".P.....PP.....P.",
	"P......PP......P",
};

static const char* const _art_swarmer[] = {
	"......RRRR......",
	"....RRRRRRRR....",
	"..RRRRYYYYRRRR..",
	"RRRRRYYYYYYRRRRR",
	"..RRRRYYYYRRRR..",
	"....RRRRRRRR....",
	"......RRRR......",
};

static const uint8_t _lander_colours[] = { DEF_GREEN, DEF_RED, DEF_WHITE };
static const uint8_t _mutant_colours[] = { DEF_PURPLE, DEF_GREEN, DEF_RED };
static const uint8_t _mutant_flicker[] = { DEF_PURPLE, DEF_WHITE, DEF_RED };
static const uint8_t _baiter_colours[] = { DEF_GREEN, DEF_WHITE, DEF_RED };
static const uint8_t _baiter_flicker[] = { DEF_GREEN, DEF_RED, DEF_WHITE };
static const uint8_t _bomber_colours[] = { DEF_BLUE, DEF_YELLOW };
static const uint8_t _pod_colours[] = { DEF_PURPLE, DEF_RED, DEF_WHITE };
static const uint8_t _swarmer_colours[] = { DEF_RED, DEF_YELLOW };

static DefArt_t _lander;
static DefArt_t _mutant;
static DefArt_t _baiter;
static DefArt_t _bomber;
static DefArt_t _pod;
static DefArt_t _swarmer;
static bool _art_made = false;

static Tile8x8p2_t _tile_bullet[2] = {
	{{
		0b11100000,
		0b11100000,
		0b11100000,
	}},
	{{
		0b10100000,
		0b01000000,
		0b10100000,
	}},
};

static const DefEnemyInfo_t _info[DEF_ENEMY_TYPE_COUNT] = {
	{ 0 },
	{ 16, 12, DEF_GREEN,  DEF_RED,    150,  "LANDER",  24,  &_lander },
	{ 16, 12, DEF_PURPLE, DEF_GREEN,  150,  "MUTANT",  60,  &_mutant },
	{ 16, 14, DEF_BLUE,   DEF_YELLOW, 250,  "BOMBER",  0,   &_bomber },
	{ 16, 14, DEF_PURPLE, DEF_RED,    1000, "POD",     0,   &_pod },
	{ 16, 7,  DEF_RED,    DEF_YELLOW, 150,  "SWARMER", 40,  &_swarmer },
	{ 32, 6,  DEF_GREEN,  DEF_WHITE,  200,  "BAITER",  120, &_baiter },
};

static void make_art(void) {
	def_art_make(&_lander, _art_lander, DEF_ART_ROWS(_art_lander), "GRW", _lander_colours);
	def_art_make(&_mutant, _art_lander, DEF_ART_ROWS(_art_lander), "GRW", _mutant_colours);
	def_art_make(&_baiter, _art_baiter, DEF_ART_ROWS(_art_baiter), "GWR", _baiter_colours);
	def_art_make(&_bomber, _art_bomber, DEF_ART_ROWS(_art_bomber), "BY", _bomber_colours);
	def_art_make(&_pod, _art_pod, DEF_ART_ROWS(_art_pod), "PRW", _pod_colours);
	def_art_make(&_swarmer, _art_swarmer, DEF_ART_ROWS(_art_swarmer), "RY", _swarmer_colours);
	_art_made = true;
}

static __force_inline int32_t enemy_w(const DefEnemy_t* e) {
	return _info[e->type].w;
}

static __force_inline int32_t enemy_h(const DefEnemy_t* e) {
	return _info[e->type].h;
}

// A random speed between lo and hi, either way
static int32_t random_v(const int32_t lo, const int32_t hi) {
	const int32_t v = lo + (int32_t)(def_rand() % (uint32_t)(hi - lo + 1));
	return (def_rand() & 1) ? v : -v;
}

void def_enemy_init(void) {
	if (!_art_made) make_art();
	for (uint32_t i = 0; i < DEF_ENEMY_MAX; ++i) _enemies[i].type = DEF_ENEMY_NONE;
	def_enemy_clear_bullets();
}

void def_enemy_set_wave(uint32_t wave) {
	_wave = wave;
}

void def_enemy_clear_bullets(void) {
	for (uint32_t i = 0; i < DEF_BULLET_MAX; ++i) _bullets[i].active = false;
}

static DefBullet_t* new_bullet(void) {
	for (uint32_t i = 0; i < DEF_BULLET_MAX; ++i) {
		if (!_bullets[i].active) return &_bullets[i];
	}
	return NULL;
}

// Shoots at where the ship is
static void fire_at_player(const DefEnemy_t* e) {
	const int32_t w = enemy_w(e);
	const int32_t x = def_wrap(e->x + DEF_PX(w >> 1));
	const int32_t y = e->y + DEF_PX(enemy_h(e) >> 1);
	const int32_t dx = def_dx(def_player_x() + DEF_PX(DEF_SHIP_W >> 1), x) >> DEF_FP;
	const int32_t dy = def_player_y() + (DEF_SHIP_H >> 1) - (y >> DEF_FP);
	const int32_t m = def_abs(dx) > def_abs(dy) ? def_abs(dx) : def_abs(dy);
	if (m == 0 || m > DEF_ENEMY_FIRE_RANGE) return;
	DefBullet_t* b = new_bullet();
	if (!b) return;
	const int32_t speed = DEF_BULLET_SPEED + (int32_t)(_wave > 8 ? 8 : _wave) * 24;
	b->active = true;
	b->mine = false;
	b->life = DEF_BULLET_LIFE;
	b->x = x;
	b->y = y;
	b->vx = dx * speed / m;
	b->vy = dy * speed / m;
}

static void drop_mine(const DefEnemy_t* e) {
	DefBullet_t* b = new_bullet();
	if (!b) return;
	b->active = true;
	b->mine = true;
	b->life = DEF_MINE_LIFE;
	b->x = def_wrap(e->x + DEF_PX(enemy_w(e) >> 1));
	b->y = e->y + DEF_PX(enemy_h(e));
	b->vx = 0;
	b->vy = 0;
}

bool def_enemy_spawn(uint32_t type, int32_t x, int32_t y) {
	// With the planet gone there is nothing for the landers to do but mutate
	if (type == DEF_ENEMY_LANDER && !def_world_planet_alive()) type = DEF_ENEMY_MUTANT;
	for (uint32_t i = 0; i < DEF_ENEMY_MAX; ++i) {
		DefEnemy_t* e = &_enemies[i];
		if (e->type != DEF_ENEMY_NONE) continue;
		e->type = type;
		e->state = DEF_ES_SPAWN;
		e->timer = DEF_ENEMY_SPAWN_FRAMES;
		e->human = -1;
		e->x = def_wrap(x);
		e->y = y;
		e->vx = 0;
		e->vy = 0;
		e->base_y = y;
		e->anim = def_rand();
		switch (type) {
			case DEF_ENEMY_LANDER: {
				const int32_t s = DEF_LANDER_SPEED + (int32_t)(_wave > 8 ? 8 : _wave) * 16;
				e->vx = random_v(s >> 1, s);
				e->base_y = DEF_PX(DEF_PLAY_TOP + 40 + def_rand() % 160);
				e->human = def_human_pick();
				break;
			}
			case DEF_ENEMY_BOMBER: {
				e->vx = random_v(128, 224);
				e->base_y = DEF_PX(DEF_PLAY_TOP + 80 + def_rand() % 180);
				break;
			}
			case DEF_ENEMY_POD: {
				e->vx = random_v(96, 256);
				e->vy = random_v(64, 160);
				break;
			}
			case DEF_ENEMY_SWARMER: {
				// Swarmers burst straight out of their pod
				e->state = DEF_ES_SEEK;
				e->vx = random_v(DEF_PX(1), DEF_PX(4));
				e->vy = random_v(DEF_PX(1), DEF_PX(2));
				break;
			}
			default: {
				break;
			}
		}
		if (e->state == DEF_ES_SPAWN) {
			def_fx_implode(def_wrap(e->x + DEF_PX(_info[type].w >> 1)), e->y + DEF_PX(_info[type].h >> 1), _info[type].colour, 24, DEF_ENEMY_SPAWN_FRAMES);
		}
		return true;
	}
	return false;
}

static void enemy_kill(DefEnemy_t* e, const bool award, const bool split) {
	const DefEnemyInfo_t* info = &_info[e->type];
	const int32_t cx = def_wrap(e->x + DEF_PX(info->w >> 1));
	const int32_t cy = e->y + DEF_PX(info->h >> 1);
	def_fx_explode(cx, cy, info->colour, 28, DEF_PX(3));
	def_fx_explode(cx, cy, info->detail, 12, DEF_PX(2));
	if (award) def_hud_add_score(info->score);
	if (e->human >= 0 && e->state == DEF_ES_ASCEND) def_human_release(e->human);
	const uint32_t type = e->type;
	e->type = DEF_ENEMY_NONE;
	if (type == DEF_ENEMY_POD && split) {
		const uint32_t n = 3 + (_wave > 6 ? 3 : _wave >> 1);
		for (uint32_t i = 0; i < n; ++i) def_enemy_spawn(DEF_ENEMY_SWARMER, e->x, e->y);
	}
}

static void update_lander(DefEnemy_t* e, const uint32_t frame) {
	switch (e->state) {
		case DEF_ES_SEEK: {
			e->y += def_sign(e->base_y - e->y) * 64;
			if (e->human >= 0 && def_human_on_ground(e->human)) {
				// Heads for the humanoid it is after, the short way round
				const int32_t dx = def_dx(def_human_x(e->human) + DEF_PX(DEF_HUMAN_W >> 1), e->x + DEF_PX(8));
				if (def_abs(dx) < DEF_PX(3)) {
					e->state = DEF_ES_DESCEND;
				}
				else {
					e->vx = def_sign(dx) * def_abs(e->vx);
				}
			}
			else if ((def_rand() & 63) == 0) {
				e->human = def_human_pick();
			}
			e->x = def_wrap(e->x + e->vx);
			break;
		}
		case DEF_ES_DESCEND: {
			if (e->human < 0 || !def_human_on_ground(e->human)) {
				e->state = DEF_ES_SEEK;
				e->human = -1;
				break;
			}
			e->x = def_wrap(e->x + def_clamp(def_dx(def_human_x(e->human) + DEF_PX(DEF_HUMAN_W >> 1), e->x + DEF_PX(8)), -DEF_PX(1), DEF_PX(1)));
			e->y += DEF_LANDER_DESCEND;
			if ((e->y >> DEF_FP) + enemy_h(e) >= (def_human_y(e->human) >> DEF_FP)) {
				def_human_grab(e->human);
				e->state = DEF_ES_ASCEND;
			}
			break;
		}
		case DEF_ES_ASCEND: {
			e->y -= DEF_LANDER_ASCEND;
			def_human_carry(e->human, def_wrap(e->x + DEF_PX(8 - (DEF_HUMAN_W >> 1))), e->y + DEF_PX(enemy_h(e)));
			if (e->y <= DEF_PX(DEF_PLAY_TOP)) {
				// Too late, it has been turned
				def_human_kill(e->human);
				e->human = -1;
				e->type = DEF_ENEMY_MUTANT;
				e->state = DEF_ES_SEEK;
				def_fx_explode(def_wrap(e->x + DEF_PX(8)), e->y + DEF_PX(6), DEF_PURPLE, 16, DEF_PX(2));
			}
			break;
		}
		default: {
			break;
		}
	}
}

// Heads for the ship, jittering about as it goes
static void update_mutant(DefEnemy_t* e) {
	const int32_t speed = DEF_MUTANT_SPEED + (int32_t)(_wave > 8 ? 8 : _wave) * 32;
	const int32_t dx = def_dx(def_player_x(), e->x);
	const int32_t dy = DEF_PX(def_player_y()) - e->y;
	e->vx += def_sign(dx) * 48;
	e->vx = def_clamp(e->vx, -speed, speed);
	e->x = def_wrap(e->x + e->vx);
	e->y += def_sign(dy) * 320 + (int32_t)((def_rand() % 5) - 2) * DEF_PX(1);
}

static void update_bomber(DefEnemy_t* e, const uint32_t frame) {
	e->x = def_wrap(e->x + e->vx);
	e->y = e->base_y + DEF_PX(def_sin(e->anim) * DEF_BOMBER_SWING / 127);
	e->anim += 1;
	if ((e->anim % 50) == 0) drop_mine(e);
}

static void update_pod(DefEnemy_t* e) {
	e->x = def_wrap(e->x + e->vx);
	e->y += e->vy;
	if (e->y < DEF_PX(DEF_PLAY_TOP + 16) || e->y > DEF_PX(DEF_TERRAIN_TOP - 60)) e->vy = -e->vy;
}

// Weaves after the ship, overshooting as it goes
static void update_swarmer(DefEnemy_t* e) {
	const int32_t dx = def_dx(def_player_x(), e->x);
	e->vx += def_sign(dx) * 40;
	e->vx = def_clamp(e->vx, -DEF_SWARMER_SPEED, DEF_SWARMER_SPEED);
	e->x = def_wrap(e->x + e->vx);
	e->anim += 8;
	e->y += def_sin(e->anim) * 2 + def_sign(DEF_PX(def_player_y()) - e->y) * 96;
}

// Fast and persistent, sent to hurry the player up
static void update_baiter(DefEnemy_t* e) {
	const int32_t dx = def_dx(def_player_x(), e->x);
	const int32_t target = def_player_vx() + def_sign(dx) * DEF_BAITER_SPEED;
	e->vx += def_clamp(target - e->vx, -96, 96);
	e->x = def_wrap(e->x + e->vx);
	e->y += def_clamp(DEF_PX(def_player_y()) - e->y, -DEF_PX(2), DEF_PX(2));
}

void __not_in_flash_func(def_enemy_update)(uint32_t frame, bool frozen) {
	for (uint32_t i = 0; i < DEF_ENEMY_MAX; ++i) {
		DefEnemy_t* e = &_enemies[i];
		if (e->type == DEF_ENEMY_NONE || frozen) continue;
		if (e->state == DEF_ES_SPAWN) {
			if (--e->timer == 0) e->state = DEF_ES_SEEK;
			continue;
		}
		switch (e->type) {
			case DEF_ENEMY_LANDER: update_lander(e, frame); break;
			case DEF_ENEMY_MUTANT: update_mutant(e); break;
			case DEF_ENEMY_BOMBER: update_bomber(e, frame); break;
			case DEF_ENEMY_POD: update_pod(e); break;
			case DEF_ENEMY_SWARMER: update_swarmer(e); break;
			case DEF_ENEMY_BAITER: update_baiter(e); break;
			default: break;
		}
		// A lander may have just turned into a mutant
		const DefEnemyInfo_t* info = &_info[e->type];
		// Landers go down amongst the mountains for the humanoids
		if (e->state == DEF_ES_SEEK) {
			e->y = def_clamp(e->y, DEF_PX(DEF_PLAY_TOP), DEF_PX(DEF_TERRAIN_TOP - info->h));
		}

		const int32_t y = e->y >> DEF_FP;
		if (def_player_laser_hit(e->x, y, info->w, info->h)) {
			enemy_kill(e, true, true);
			continue;
		}
		if (def_player_hit(e->x, y, info->w, info->h)) {
			enemy_kill(e, true, false);
			def_player_kill();
			continue;
		}
		const uint32_t chance = info->fire + (info->fire ? (_wave > 10 ? 10 : _wave) * 4 : 0);
		if (chance && def_player_alive() && (def_rand() & 4095) < chance) {
			fire_at_player(e);
		}
	}

	for (uint32_t i = 0; i < DEF_BULLET_MAX; ++i) {
		DefBullet_t* b = &_bullets[i];
		if (!b->active) continue;
		if (frozen || --b->life == 0) {
			b->active = false;
			continue;
		}
		b->x = def_wrap(b->x + b->vx);
		b->y += b->vy;
		const int32_t y = b->y >> DEF_FP;
		if (y < DEF_PLAY_TOP || y >= DEF_SCREEN_H) {
			b->active = false;
		}
		else if (def_player_hit(b->x, y, 3, 3)) {
			b->active = false;
			def_player_kill();
		}
	}
}

static void draw_enemy(const uint32_t type, const int32_t sx, const int32_t sy, const uint32_t frame) {
	const DefEnemyInfo_t* info = &_info[type];
	// Mutants and baiters flicker
	const uint8_t* colours = NULL;
	if (frame & 4) {
		if (type == DEF_ENEMY_MUTANT) colours = _mutant_flicker;
		else if (type == DEF_ENEMY_BAITER) colours = _baiter_flicker;
	}
	def_art_draw(info->art, sx, sy, colours);
}

void __not_in_flash_func(def_enemy_draw)(uint32_t frame) {
	for (uint32_t i = 0; i < DEF_ENEMY_MAX; ++i) {
		const DefEnemy_t* e = &_enemies[i];
		if (e->type == DEF_ENEMY_NONE || e->state == DEF_ES_SPAWN) continue;
		int32_t sx = def_world_screen_x(e->x);
		if (sx <= -_info[e->type].w || sx >= DEF_SCREEN_W) continue;
		if (e->type == DEF_ENEMY_MUTANT) sx += (int32_t)(def_rand() % 3) - 1;
		draw_enemy(e->type, sx, e->y >> DEF_FP, frame);
	}
}

void __not_in_flash_func(def_enemy_draw_bullets)(uint32_t frame) {
	for (uint32_t i = 0; i < DEF_BULLET_MAX; ++i) {
		const DefBullet_t* b = &_bullets[i];
		if (!b->active) continue;
		const int32_t sx = def_world_screen_x(b->x);
		if (sx <= -4 || sx >= DEF_SCREEN_W) continue;
		const uint32_t c = b->mine ? ((frame & 8) ? DEF_PURPLE : DEF_WHITE) : ((frame & 2) ? DEF_WHITE : DEF_YELLOW);
		def_spr_add(sx, b->y >> DEF_FP, 8, 3, &_tile_bullet[b->mine ? 1 : 0], def_pallet(c), sprite_renderer_sprite_8x8_p1);
	}
}

void def_enemy_scan(void) {
	for (uint32_t i = 0; i < DEF_ENEMY_MAX; ++i) {
		const DefEnemy_t* e = &_enemies[i];
		if (e->type == DEF_ENEMY_NONE) continue;
		def_scanner_dot(e->x, e->y >> DEF_FP, _info[e->type].colour);
	}
}

uint32_t def_enemy_count_hostile(void) {
	uint32_t n = 0;
	for (uint32_t i = 0; i < DEF_ENEMY_MAX; ++i) {
		const uint32_t t = _enemies[i].type;
		if (t != DEF_ENEMY_NONE && t != DEF_ENEMY_BAITER) ++n;
	}
	return n;
}

uint32_t def_enemy_count(uint32_t type) {
	uint32_t n = 0;
	for (uint32_t i = 0; i < DEF_ENEMY_MAX; ++i) {
		if (_enemies[i].type == type) ++n;
	}
	return n;
}

uint32_t def_enemy_on_screen(void) {
	uint32_t n = 0;
	for (uint32_t i = 0; i < DEF_ENEMY_MAX; ++i) {
		const DefEnemy_t* e = &_enemies[i];
		if (e->type != DEF_ENEMY_NONE && e->state != DEF_ES_SPAWN && def_world_on_screen(e->x, _info[e->type].w)) ++n;
	}
	return n;
}

void def_enemy_smart_bomb(void) {
	for (uint32_t i = 0; i < DEF_ENEMY_MAX; ++i) {
		DefEnemy_t* e = &_enemies[i];
		if (e->type != DEF_ENEMY_NONE && e->state != DEF_ES_SPAWN && def_world_on_screen(e->x, _info[e->type].w)) {
			enemy_kill(e, true, false);
		}
	}
	// Mines and bullets on the screen go too
	for (uint32_t i = 0; i < DEF_BULLET_MAX; ++i) {
		if (def_world_on_screen(_bullets[i].x, 4)) _bullets[i].active = false;
	}
}

void def_enemy_remove(uint32_t type) {
	for (uint32_t i = 0; i < DEF_ENEMY_MAX; ++i) {
		if (_enemies[i].type == type) _enemies[i].type = DEF_ENEMY_NONE;
	}
}

void def_enemy_mutate_all(void) {
	for (uint32_t i = 0; i < DEF_ENEMY_MAX; ++i) {
		DefEnemy_t* e = &_enemies[i];
		if (e->type != DEF_ENEMY_LANDER) continue;
		e->type = DEF_ENEMY_MUTANT;
		e->human = -1;
		if (e->state != DEF_ES_SPAWN) e->state = DEF_ES_SEEK;
	}
}

bool def_enemy_nearest(int32_t x, int32_t* ex, int32_t* ey) {
	int32_t best = INT32_MAX;
	for (uint32_t i = 0; i < DEF_ENEMY_MAX; ++i) {
		const DefEnemy_t* e = &_enemies[i];
		if (e->type == DEF_ENEMY_NONE || e->state == DEF_ES_SPAWN) continue;
		const int32_t d = def_abs(def_dx(e->x, x));
		if (d < best) {
			best = d;
			*ex = def_wrap(e->x + DEF_PX(_info[e->type].w >> 1));
			*ey = (e->y >> DEF_FP) + (_info[e->type].h >> 1);
		}
	}
	return best != INT32_MAX;
}

bool def_enemy_bullet_near(int32_t x, int32_t y, int32_t range, int32_t* by) {
	for (uint32_t i = 0; i < DEF_BULLET_MAX; ++i) {
		const DefBullet_t* b = &_bullets[i];
		if (!b->active) continue;
		const int32_t dx = def_abs(def_dx(b->x, x)) >> DEF_FP;
		const int32_t dy = def_abs((b->y >> DEF_FP) - y);
		if (dx < range && dy < (range >> 1)) {
			*by = b->y >> DEF_FP;
			return true;
		}
	}
	return false;
}

void def_enemy_draw_icon(uint32_t type, int32_t sx, int32_t sy, uint32_t frame) {
	if (type == DEF_ENEMY_NONE || type >= DEF_ENEMY_TYPE_COUNT) return;
	if (!_art_made) make_art();
	draw_enemy(type, sx, sy, frame);
}

uint32_t def_enemy_width(uint32_t type) {
	return type < DEF_ENEMY_TYPE_COUNT ? _info[type].w : 0;
}

uint32_t def_enemy_height(uint32_t type) {
	return type < DEF_ENEMY_TYPE_COUNT ? _info[type].h : 0;
}

const char* def_enemy_name(uint32_t type) {
	return type < DEF_ENEMY_TYPE_COUNT && _info[type].name ? _info[type].name : "";
}

uint32_t def_enemy_score(uint32_t type) {
	return type < DEF_ENEMY_TYPE_COUNT ? _info[type].score : 0;
}

uint32_t def_enemy_colour(uint32_t type) {
	return type < DEF_ENEMY_TYPE_COUNT ? _info[type].colour : DEF_WHITE;
}
