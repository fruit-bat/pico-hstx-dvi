// Copyright (c) 2024 fruit-bat.

// See el_ships.h.
//
// A hostile ship turns towards us, comes in, fires when it is pointing at
// us, and breaks off when it gets too close, to come round again. The light
// fighters weave as they come, and the big ships keep their distance. A
// trader that has been badly hurt runs. Each turns its nose towards where
// it wants to go by a little each frame.

#include "el_ships.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

Ship_t ships[SHIPS];

// How far from the station new ships turn up, and how far from both it and
// us they are left behind and replaced, as we may be far from the station
#define SPAWN_NEAR 3000.0f
#define SPAWN_FAR 12000.0f
#define LEAVE_RANGE 30000.0f
#define RESPAWN_FRAMES (8 * 60)
// How long a ship breaks off for, having come too close
#define BREAK_FRAMES 70
// How well a ship must point at us to fire
#define SHOT_AIM 0.985f
#define BEAM_FRAMES 5
// How far a ship's weaving takes its aim off us
#define JINK 0.35f
// A trader with less than this of its hull left runs
#define RUN 0.4f
// The chance each frame that a ship with missiles fires one, when in range
// and pointing at us, out of 10000
#define MISSILE_CHANCE 40
#define MISSILE_RANGE 7000.0f
// Running into a ship does this much damage to us
#define RAM_DAMAGE 0.5f
#define RAM_MARGIN 40.0f

// The missiles: how many can fly at once, how fast, how quickly they turn,
// for how long, and what one does to us
#define MISSILES 4
#define MISSILE_SPEED 55.0f
#define MISSILE_TURN 0.06f
#define MISSILE_FRAMES (10 * 60)
#define MISSILE_DAMAGE 0.7f
// Fired at us, a missile is a little slower, so we can get away
#define MISSILE_SPEED_AT_US 32.0f

// The pieces of a destroyed ship flying apart
#define EXPLOSIONS 4
#define DEBRIS 10
#define EXPLOSION_FRAMES 70

// The types of ship, loosely after the original game
static const ShipClass_t _classes[] = {
	// Light fighters: fast and agile, small so hard to hit, but fragile and weak
	{ "SIDEWINDER",    20.0f, 0.050f,  2.0f, 0.08f, 40, 45, 4000.0f,  500.0f, true,  0,  50 },
	{ "GECKO",         19.0f, 0.050f,  2.0f, 0.08f, 40, 45, 4000.0f,  500.0f, true,  0,  55 },
	{ "WORM",          18.0f, 0.050f,  1.5f, 0.06f, 45, 40, 3500.0f,  500.0f, true,  0,  30 },
	// All-rounders
	{ "MAMBA",         17.0f, 0.040f,  4.0f, 0.12f, 45, 55, 5000.0f,  700.0f, false, 1, 150 },
	{ "KRAIT",         17.0f, 0.040f,  3.5f, 0.12f, 45, 55, 5000.0f,  700.0f, false, 0, 100 },
	{ "ADDER",         14.0f, 0.035f,  4.0f, 0.12f, 50, 55, 5000.0f,  700.0f, false, 0,  40 },
	{ "COBRA MK I",    15.0f, 0.035f,  4.0f, 0.14f, 50, 55, 5000.0f,  700.0f, false, 1,  75 },
	// Heavy fighters: fast, tough and hard hitting
	{ "ASP MK II",     22.0f, 0.045f,  7.0f, 0.20f, 35, 65, 6000.0f,  800.0f, false, 1, 200 },
	{ "FER-DE-LANCE",  18.0f, 0.040f,  7.0f, 0.20f, 35, 65, 6000.0f,  800.0f, false, 1, 250 },
	// The police
	{ "VIPER",         20.0f, 0.045f,  6.0f, 0.16f, 35, 70, 6000.0f,  700.0f, false, 1,   0 },
	// Traders. The big ones are slow and turn badly, but are tough and hit
	// hard from a long way off, keeping their distance
	{ "COBRA MK III",  15.0f, 0.035f,  6.0f, 0.15f, 45, 60, 6000.0f,  800.0f, false, 1,   0 },
	{ "MORAY",         14.0f, 0.035f,  5.0f, 0.13f, 50, 55, 5000.0f,  800.0f, false, 0,   0 },
	{ "PYTHON",        10.0f, 0.015f, 12.0f, 0.30f, 55, 70, 8000.0f, 1500.0f, false, 2,   0 },
	{ "BOA",           10.0f, 0.015f, 12.0f, 0.30f, 55, 65, 8000.0f, 1500.0f, false, 1,   0 },
	{ "ANACONDA",       8.0f, 0.012f, 16.0f, 0.35f, 60, 70, 8000.0f, 1800.0f, false, 2,   0 },
	// Unarmed, and run
	{ "TRANSPORTER",    8.0f, 0.020f,  2.0f, 0.00f, 60,  0,    0.0f, 1000.0f, false, 0,   0 },
	{ "ORBIT SHUTTLE",  8.0f, 0.020f,  2.0f, 0.00f, 60,  0,    0.0f, 1000.0f, false, 0,   0 },
};
#define CLASSES (sizeof(_classes) / sizeof(_classes[0]))

// The ranges of _classes for each kind
#define LIGHT_FIRST 0
#define LIGHT_COUNT 3
#define MEDIUM_FIRST 3
#define MEDIUM_COUNT 4
#define HEAVY_FIRST 7
#define HEAVY_COUNT 2
#define POLICE 9
#define SMALL_TRADER_FIRST 10
#define SMALL_TRADER_COUNT 2
#define BIG_TRADER_FIRST 12
#define BIG_TRADER_COUNT 3
#define UNARMED_FIRST 15
#define UNARMED_COUNT 2

typedef struct {
	Vec3_t pos;
	Vec3_t dir[DEBRIS];
	float size;
	uint32_t frame;
	bool on;
} Explosion_t;

typedef struct {
	Frame_t fr;
	int32_t target; // a ship, or -1 for us
	uint32_t frames;
	bool on;
} Missile_t;

static Explosion_t _explosions[EXPLOSIONS];
static Missile_t _missiles[MISSILES];
// How many ships the system has room for, from how busy it is
static uint32_t _traffic = SHIPS;
static uint32_t _frame = 0;

static float rand_range(float lo, float hi) {
	return lo + (hi - lo) * (float)(rand() & 0xffff) / 65535.0f;
}

static Vec3_t rand_unit(void) {
	return vec_unit((Vec3_t){ rand_range(-1.0f, 1.0f), rand_range(-1.0f, 1.0f), rand_range(-1.0f, 1.0f) });
}

static uint32_t percent(void) {
	return (uint32_t)(rand() % 100);
}

static const WireModel_t* model_named(const char* name) {
	for (uint32_t i = 0; i < MODEL_SHIP_COUNT; ++i) {
		if (!strcmp(model_ships[i]->name, name)) return model_ships[i];
	}
	return &model_cobra;
}

static const ShipClass_t* class_named(const char* name) {
	for (uint32_t i = 0; i < CLASSES; ++i) {
		if (!strcmp(_classes[i].name, name)) return &_classes[i];
	}
	return &_classes[MEDIUM_FIRST];
}

static const ShipClass_t* pick(uint32_t first, uint32_t count) {
	return &_classes[first + (uint32_t)rand() % count];
}

static void set_class(Ship_t* s, const ShipClass_t* c) {
	s->cls = c;
	s->model = model_named(c->name);
	s->energy = c->hull;
	s->missiles = c->missiles;
}

// A ship somewhere round the station, as the system goes: pirates where the
// law is weak, heavier the weaker it is, police where it is strong, and big
// freighters where there is industry
static void new_ship(Ship_t* s, const ShipsWorld_t* w) {
	const uint32_t r = percent();
	const uint32_t pirates = (8 - w->government) * 7;
	const uint32_t police = 5 + w->government * 3;
	if (r < pirates) {
		s->role = ROLE_PIRATE;
		const uint32_t heavy = w->government < 3 ? (3 - w->government) * 10 : 3;
		const uint32_t k = percent();
		if (k < heavy) set_class(s, pick(HEAVY_FIRST, HEAVY_COUNT));
		else if (k < heavy + 45) set_class(s, pick(MEDIUM_FIRST, MEDIUM_COUNT));
		else set_class(s, pick(LIGHT_FIRST, LIGHT_COUNT));
	}
	else if (r < pirates + police) {
		s->role = ROLE_POLICE;
		set_class(s, &_classes[POLICE]);
	}
	else {
		s->role = ROLE_TRADER;
		const uint32_t big = w->economy < 4 ? 40 : 15;
		const uint32_t k = percent();
		if (k < big) set_class(s, pick(BIG_TRADER_FIRST, BIG_TRADER_COUNT));
		else if (k < big + 15) set_class(s, pick(UNARMED_FIRST, UNARMED_COUNT));
		else set_class(s, pick(SMALL_TRADER_FIRST, SMALL_TRADER_COUNT));
	}
	const Vec3_t pos = vec_add(w->station, vec_scale(rand_unit(), rand_range(SPAWN_NEAR, SPAWN_FAR)));
	frame_set(&s->fr, pos, rand_unit(), rand_unit());
	s->speed = rand_range(0.2f, 0.5f) * s->cls->speed;
	s->roll = rand_range(-0.01f, 0.01f);
	s->pitch = rand_range(-0.004f, 0.004f);
	s->alive = true;
	s->hostile = s->role == ROLE_PIRATE || (s->role == ROLE_POLICE && w->offender);
	s->cooldown = s->cls->rate;
	s->beam = 0;
	s->retreat = 0;
	s->respawn = 0;
}

void ships_debug_place(uint32_t i, const char* name, Vec3_t pos, Vec3_t f, Vec3_t u, const ShipsWorld_t* w) {
	Ship_t* s = &ships[i % SHIPS];
	new_ship(s, w);
	s->role = ROLE_PIRATE;
	set_class(s, class_named(name));
	s->hostile = true;
	frame_set(&s->fr, pos, f, u);
}

void ships_enter(const ShipsWorld_t* w) {
	// Busier the more people there are
	_traffic = 3 + w->population / 16;
	if (_traffic > SHIPS) _traffic = SHIPS;
	for (uint32_t i = 0; i < SHIPS; ++i) {
		if (i < _traffic) {
			new_ship(&ships[i], w);
		}
		else {
			ships[i].alive = false;
			ships[i].respawn = 0;
		}
	}
	for (uint32_t i = 0; i < EXPLOSIONS; ++i) _explosions[i].on = false;
	for (uint32_t i = 0; i < MISSILES; ++i) _missiles[i].on = false;
}

static void explode(Vec3_t pos, float size) {
	Explosion_t* e = &_explosions[0];
	for (uint32_t i = 0; i < EXPLOSIONS; ++i) {
		if (!_explosions[i].on) {
			e = &_explosions[i];
			break;
		}
	}
	e->on = true;
	e->pos = pos;
	e->size = size;
	e->frame = 0;
	for (uint32_t i = 0; i < DEBRIS; ++i) e->dir[i] = rand_unit();
}

static void destroy(Ship_t* s, ShipsEvents_t* ev, bool by_us) {
	explode(s->fr.pos, s->model->radius);
	s->alive = false;
	s->beam = 0;
	s->respawn = RESPAWN_FRAMES;
	if (by_us) {
		++ev->kills;
		if (s->role == ROLE_PIRATE) ev->bounty += s->cls->bounty;
		else ev->innocent = true;
	}
}

void ships_hit(uint32_t i, float damage, ShipsEvents_t* ev) {
	Ship_t* s = &ships[i];
	if (!s->alive) return;
	// Anyone shot at shoots back, or runs
	s->hostile = true;
	s->energy -= damage;
	if (s->energy <= 0.0f) destroy(s, ev, true);
}

// Turns a frame's nose towards a way by at most turn, keeping it upright as it can
static void turn_towards(Frame_t* fr, Vec3_t dir, float turn) {
	const Vec3_t d = vec_sub(dir, fr->f);
	const float l = vec_length(d);
	if (l > turn) fr->f = vec_add(fr->f, vec_scale(d, turn / l));
	else fr->f = dir;
	frame_tidy(fr);
}

static bool launch_missile(Frame_t from, int32_t target) {
	for (uint32_t i = 0; i < MISSILES; ++i) {
		Missile_t* m = &_missiles[i];
		if (m->on) continue;
		m->on = true;
		m->fr = from;
		m->target = target;
		m->frames = 0;
		return true;
	}
	return false;
}

static void attack(Ship_t* s, uint32_t i, const ShipsWorld_t* w, ShipsEvents_t* ev) {
	const ShipClass_t* c = s->cls;
	const Vec3_t to = vec_sub(w->me->pos, s->fr.pos);
	const float d = vec_length(to);
	const Vec3_t dir = vec_scale(to, 1.0f / fmaxf(d, 1.0f));
	s->speed = c->speed;

	// Traders run once badly hurt, and the unarmed ones straight away
	const bool runs = s->role == ROLE_TRADER && (c->damage == 0.0f || s->energy < c->hull * RUN);
	if (runs) {
		turn_towards(&s->fr, vec_scale(dir, -1.0f), c->turn);
		return;
	}
	if (d < c->close && !s->retreat) s->retreat = BREAK_FRAMES;
	if (s->retreat) {
		--s->retreat;
		turn_towards(&s->fr, vec_scale(dir, -1.0f), c->turn);
	}
	else {
		Vec3_t aim = dir;
		if (c->jinks) {
			// Weaving across the way to us
			const float t = (float)_frame * 0.05f + (float)i * 1.7f;
			aim = vec_unit(vec_add(dir, vec_add(vec_scale(s->fr.r, JINK * sinf(t)), vec_scale(s->fr.u, JINK * cosf(t * 1.3f)))));
		}
		turn_towards(&s->fr, aim, c->turn);
	}
	frame_roll(&s->fr, 0.02f);
	frame_tidy(&s->fr);

	// Fires when pointing at us
	if (s->cooldown) --s->cooldown;
	const bool pointing = vec_dot(s->fr.f, dir) > SHOT_AIM;
	if (!s->cooldown && pointing && d < c->range) {
		s->cooldown = c->rate;
		s->beam = BEAM_FRAMES;
		if (percent() < c->aim) {
			// On the shield on the side it is
			if (vec_dot(vec_scale(to, -1.0f), w->me->f) >= 0.0f) ev->fore += c->damage;
			else ev->aft += c->damage;
		}
	}
	// And now and then a missile
	if (s->missiles && pointing && d < MISSILE_RANGE && (uint32_t)(rand() % 10000) < MISSILE_CHANCE) {
		Frame_t from = s->fr;
		from.pos = vec_add(s->fr.pos, vec_scale(s->fr.f, s->model->radius + 20.0f));
		if (launch_missile(from, -1)) {
			--s->missiles;
			ev->missile = true;
		}
	}
}

static void move_missiles(const ShipsWorld_t* w, ShipsEvents_t* ev) {
	for (uint32_t i = 0; i < MISSILES; ++i) {
		Missile_t* m = &_missiles[i];
		if (!m->on) continue;
		const bool at_us = m->target < 0;
		if (!at_us && !ships[m->target].alive) m->frames = MISSILE_FRAMES;
		if (++m->frames > MISSILE_FRAMES) {
			explode(m->fr.pos, model_missile.radius);
			m->on = false;
			continue;
		}
		const Vec3_t target = at_us ? w->me->pos : ships[m->target].fr.pos;
		const float radius = at_us ? RAM_MARGIN : ships[m->target].model->radius;
		const float speed = at_us ? MISSILE_SPEED_AT_US : MISSILE_SPEED;
		const Vec3_t to = vec_sub(target, m->fr.pos);
		const float d = vec_length(to);
		if (d < radius + speed) {
			if (at_us) {
				explode(m->fr.pos, model_missile.radius * 3.0f);
				if (vec_dot(vec_scale(to, -1.0f), w->me->f) >= 0.0f) ev->fore += MISSILE_DAMAGE;
				else ev->aft += MISSILE_DAMAGE;
			}
			else {
				destroy(&ships[m->target], ev, true);
			}
			m->on = false;
			continue;
		}
		turn_towards(&m->fr, vec_scale(to, 1.0f / d), MISSILE_TURN);
		m->fr.pos = vec_add(m->fr.pos, vec_scale(m->fr.f, speed));
	}
}

void ships_move(const ShipsWorld_t* w, ShipsEvents_t* ev) {
	++_frame;
	for (uint32_t i = 0; i < _traffic; ++i) {
		Ship_t* s = &ships[i];
		if (!s->alive) {
			if (s->respawn && --s->respawn == 0) new_ship(s, w);
			continue;
		}
		if (s->beam) --s->beam;
		// The police come for us once we are an offender
		if (s->role == ROLE_POLICE && w->offender) s->hostile = true;
		if (s->hostile) {
			attack(s, i, w, ev);
		}
		else {
			frame_roll(&s->fr, s->roll);
			frame_pitch(&s->fr, s->pitch);
			frame_tidy(&s->fr);
		}
		s->fr.pos = vec_add(s->fr.pos, vec_scale(s->fr.f, s->speed));
		const float d = vec_length(vec_sub(s->fr.pos, w->me->pos));
		if (d < s->model->radius + RAM_MARGIN) {
			ev->fore += RAM_DAMAGE;
			destroy(s, ev, true);
		}
		else if (d > LEAVE_RANGE && vec_length(vec_sub(s->fr.pos, w->station)) > LEAVE_RANGE) {
			new_ship(s, w);
		}
	}
	move_missiles(w, ev);
	for (uint32_t i = 0; i < EXPLOSIONS; ++i) {
		if (_explosions[i].on && ++_explosions[i].frame > EXPLOSION_FRAMES) _explosions[i].on = false;
	}
}

int32_t ships_in_sights(const Frame_t* me, float range) {
	int32_t best = -1;
	float best_z = range;
	for (uint32_t i = 0; i < SHIPS; ++i) {
		const Ship_t* s = &ships[i];
		if (!s->alive) continue;
		const Vec3_t v = frame_view(me, s->fr.pos);
		if (v.z <= 0.0f || v.z > best_z) continue;
		// Across the middle of the view, as near as its size
		if (v.x * v.x + v.y * v.y < s->model->radius * s->model->radius * 0.6f) {
			best = (int32_t)i;
			best_z = v.z;
		}
	}
	return best;
}

int32_t ships_nearest_ahead(const Frame_t* me) {
	int32_t best = -1;
	float best_c = 0.95f;
	for (uint32_t i = 0; i < SHIPS; ++i) {
		const Ship_t* s = &ships[i];
		if (!s->alive) continue;
		const Vec3_t v = frame_view(me, s->fr.pos);
		const float c = v.z / fmaxf(vec_length(v), 1.0f);
		if (c > best_c) {
			best = (int32_t)i;
			best_c = c;
		}
	}
	return best;
}

bool ships_fire_missile(const Frame_t* me, uint32_t target) {
	Frame_t from = *me;
	from.pos = vec_add(me->pos, vec_sub(vec_scale(me->f, 60.0f), vec_scale(me->u, 30.0f)));
	return launch_missile(from, (int32_t)target);
}

void ships_ecm(void) {
	for (uint32_t i = 0; i < MISSILES; ++i) {
		if (!_missiles[i].on) continue;
		explode(_missiles[i].fr.pos, model_missile.radius * 2.0f);
		_missiles[i].on = false;
	}
}

bool ships_hostile_near(const Frame_t* me, float range) {
	for (uint32_t i = 0; i < MISSILES; ++i) {
		if (_missiles[i].on && _missiles[i].target < 0) return true;
	}
	for (uint32_t i = 0; i < SHIPS; ++i) {
		if (ships[i].alive && ships[i].hostile && vec_length(vec_sub(ships[i].fr.pos, me->pos)) < range) return true;
	}
	return false;
}

void ships_draw_effects(const Frame_t* me, Style_e style, uint32_t frame) {
	// Lasers fired at us, from the ship towards us, ending a little off to one side
	const hstx_dvi_pixel_t beam = hstx_dvi_pixel_rgb(255, 73, 73);
	for (uint32_t i = 0; i < SHIPS; ++i) {
		const Ship_t* s = &ships[i];
		if (!s->alive || !s->beam) continue;
		const Vec3_t from = frame_view(me, vec_add(s->fr.pos, vec_scale(s->fr.f, s->model->radius)));
		const float side = (i & 1) ? 1.0f : -1.0f;
		const Vec3_t to = { side * 30.0f, -20.0f, NEAR_Z * 2.0f };
		if (!object_begin()) break;
		add_edge(from, to, beam);
		object_end();
	}

	// The missiles
	for (uint32_t i = 0; i < MISSILES; ++i) {
		const Missile_t* m = &_missiles[i];
		if (!m->on) continue;
		float mm[3][3];
		frame_matrix(me, &m->fr, mm);
		draw_object(&model_missile, frame_view(me, m->fr.pos), mm, model_missile.rgb, style);
	}

	// The pieces flying apart, fading
	for (uint32_t i = 0; i < EXPLOSIONS; ++i) {
		const Explosion_t* e = &_explosions[i];
		if (!e->on) continue;
		const float t = (float)e->frame / (float)EXPLOSION_FRAMES;
		const float r = e->size * (0.3f + 2.5f * t);
		const float len = e->size * 0.4f * (1.0f - t);
		const uint8_t hot[3] = { 255, (uint8_t)(219 * (1.0f - t)), (uint8_t)(73 * (1.0f - t)) };
		const hstx_dvi_pixel_t c = shade(hot, 1.0f - 0.6f * t);
		if (!object_begin()) break;
		for (uint32_t k = 0; k < DEBRIS; ++k) {
			// Each piece tumbles as it goes
			const Vec3_t a = vec_add(e->pos, vec_scale(e->dir[k], r));
			const Vec3_t b = vec_add(a, vec_scale(e->dir[(k + 3 + frame / 8) % DEBRIS], len));
			add_edge(frame_view(me, a), frame_view(me, b), c);
		}
		object_end();
	}
}
