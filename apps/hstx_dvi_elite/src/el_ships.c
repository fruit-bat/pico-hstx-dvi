// Copyright (c) 2024 fruit-bat.

// See el_ships.h.
//
// A destroyed ship may leave a canister, which drifts on as the ship was
// going, more slowly. With fuel scoops we pick one up by flying into it,
// unless it comes in well above the nose; otherwise it breaks against us.
//
// A hostile ship turns towards us, comes in, fires when it is pointing at
// us, and breaks off when it gets too close, to come round again. The light
// fighters weave as they come, and the big ships keep their distance. A
// trader that has been badly hurt runs. Each turns its nose towards where
// it wants to go by a little each frame.

#include "el_ships.h"
#include "el_story.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

Ship_t ships[SHIPS];
Canister_t canisters[CANISTERS];

// How far from the station new ships turn up, and how far from both it and
// us they are left behind and replaced, as we may be far from the station
#define SPAWN_NEAR 3000.0f
#define SPAWN_FAR 12000.0f
#define LEAVE_RANGE 30000.0f
#define RESPAWN_FRAMES (8 * 60)
// How long a ship breaks off for, having come too close, and how far ahead
// it looks, in frames at the speed we are closing, to break off in time to
// miss us, turning across our way and slowing to turn tighter
#define BREAK_FRAMES 70
#define BREAK_AHEAD 25.0f
#define BREAK_SPEED 0.6f
// How well a ship must point at us to fire
#define SHOT_AIM 0.985f
#define BEAM_FRAMES 5
// A laser does all its damage out to this much of its range, then less, to
// this much at the end of it; so a ship holds its fire until it does at least
// this much, closing in
#define LASER_NEAR 0.25f
#define LASER_FAR_DAMAGE 0.2f
#define HOLD_FIRE 0.6f
// A story ship that can be is disabled below this much of its hull; and it
// comes near us within this
#define DISABLED_HULL 0.3f
#define STORY_NEAR 4000.0f
// One following us keeps about this far off, and one racing has got there
// this many planet radii from the planet's middle
#define FOLLOW_DISTANCE 1500.0f
#define RACE_DONE 1.4f
// A ship's bounty is from this to this percent of its class's
#define WANTED_LOW 60
#define WANTED_HIGH 160
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

// The chance a destroyed trader or pirate leaves a canister, in percent; the
// police carry none. It keeps a little of the ship's speed, and tumbles.
#define TRADER_CARGO 80
#define PIRATE_CARGO 40
#define CANISTER_DRIFT 0.25f
#define CANISTER_SPIN 0.03f
// Running into one we do not pick up does this much damage
#define CANISTER_DAMAGE 0.1f
// The scoops take a canister hitting us lower than this, from -1 below to 1 above
#define SCOOP_ABOVE 0.5f
// The chance a new ship is a Thargoid, out of 1000, how many come for us in
// witchspace, and how far off; the chance each frame a Thargoid launches a
// Thargon, when it can, out of 10000, and from how far; and the chance a
// Thargon destroyed leaves alien items, in percent, and when it is dead
#define THARGOID_CHANCE 15
#define WITCH_THARGOIDS 2
#define WITCH_NEAR 6000.0f
#define WITCH_FAR 10000.0f
#define THARGON_CHANCE 60
#define THARGON_RANGE 8000.0f
#define THARGON_ALIEN 40
#define THARGON_DEAD_ALIEN 100

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
	// hard from further off than the rest, keeping their distance
	{ "COBRA MK III",  15.0f, 0.035f,  6.0f, 0.15f, 45, 60, 6000.0f,  800.0f, false, 1,   0 },
	{ "MORAY",         14.0f, 0.035f,  5.0f, 0.13f, 50, 55, 5000.0f,  800.0f, false, 0,   0 },
	{ "PYTHON",        10.0f, 0.015f, 12.0f, 0.30f, 55, 70, 8000.0f, 1500.0f, false, 2,   0 },
	{ "BOA",           10.0f, 0.015f, 12.0f, 0.30f, 55, 65, 8000.0f, 1500.0f, false, 1,   0 },
	{ "ANACONDA",       8.0f, 0.012f, 16.0f, 0.35f, 60, 70, 8000.0f, 1800.0f, false, 2,   0 },
	// Unarmed, and run
	{ "TRANSPORTER",    8.0f, 0.020f,  2.0f, 0.00f, 60,  0,    0.0f, 1000.0f, false, 0,   0 },
	{ "ORBIT SHUTTLE",  8.0f, 0.020f,  2.0f, 0.00f, 60,  0,    0.0f, 1000.0f, false, 0,   0 },
	// Aliens: the Thargoid is big and tough, and launches Thargons, which are
	// fast and weak, and weave
	{ "THARGOID",      14.0f, 0.030f, 12.0f, 0.18f, 40, 60, 7000.0f, 1500.0f, false, 4, 500 },
	{ "THARGON",       22.0f, 0.060f,  1.0f, 0.05f, 50, 40, 3000.0f,  300.0f, true,  0,  50 },
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
#define THARGOID 17
#define THARGON 18

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
// Whether we are in witchspace, where nothing new comes
static bool _witch = false;

static float rand_range(float lo, float hi) {
	return lo + (hi - lo) * (float)(rand() & 0xffff) / 65535.0f;
}

static Vec3_t rand_unit(void) {
	return vec_unit((Vec3_t){ rand_range(-1.0f, 1.0f), rand_range(-1.0f, 1.0f), rand_range(-1.0f, 1.0f) });
}

static uint32_t percent(void) {
	return (uint32_t)(rand() % 100);
}

static const WireModel_t* model_named(const char* name);

const WireModel_t* ships_model(const char* name) {
	return model_named(name);
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
	s->wanted = WANTED_LOW + (uint32_t)rand() % (WANTED_HIGH - WANTED_LOW + 1);
}

float ships_laser_falloff(float d, float range) {
	const float near = range * LASER_NEAR;
	if (d <= near) return 1.0f;
	const float t = fminf((d - near) / (range - near), 1.0f);
	return 1.0f - t * (1.0f - LASER_FAR_DAMAGE);
}

uint32_t ships_bounty(uint32_t i) {
	const Ship_t* s = &ships[i];
	if (s->role != ROLE_PIRATE && s->role != ROLE_THARGOID && s->role != ROLE_THARGON) return 0;
	return s->cls->bounty * s->wanted / 100;
}

// A ship somewhere round the station, as the system goes: pirates where the
// law is weak, heavier the weaker it is, police where it is strong, and big
// freighters where there is industry
static void new_ship(Ship_t* s, const ShipsWorld_t* w) {
	const uint32_t r = percent();
	s->mother = -1;
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
	// Rarely, a Thargoid
	if ((uint32_t)(rand() % 1000) < THARGOID_CHANCE) {
		s->role = ROLE_THARGOID;
		set_class(s, &_classes[THARGOID]);
	}
	const Vec3_t pos = vec_add(w->station, vec_scale(rand_unit(), rand_range(SPAWN_NEAR, SPAWN_FAR)));
	frame_set(&s->fr, pos, rand_unit(), rand_unit());
	s->speed = rand_range(0.2f, 0.5f) * s->cls->speed;
	s->roll = rand_range(-0.01f, 0.01f);
	s->pitch = rand_range(-0.004f, 0.004f);
	s->alive = true;
	s->hostile = (s->role == ROLE_PIRATE && !w->pirates_friendly) || (s->role == ROLE_THARGOID && !w->aliens_friendly) ||
		(s->role == ROLE_POLICE && w->offender);
	s->story = 0;
	s->goal = GOAL_FIGHT;
	s->can_disable = s->disabled = s->near = false;
	s->drop = 0;
	s->label = NULL;
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
	_witch = w->witch;
	// In witchspace, Thargoids round us, and room for their Thargons
	if (_witch) _traffic = WITCH_THARGOIDS + (uint32_t)rand() % 2;
	for (uint32_t i = 0; i < SHIPS; ++i) {
		if (i < _traffic && _witch) {
			Ship_t* s = &ships[i];
			new_ship(s, w);
			s->role = ROLE_THARGOID;
			set_class(s, &_classes[THARGOID]);
			s->hostile = !w->aliens_friendly;
			const Vec3_t pos = vec_add(w->me->pos, vec_scale(rand_unit(), rand_range(WITCH_NEAR, WITCH_FAR)));
			frame_set(&s->fr, pos, vec_unit(vec_sub(w->me->pos, pos)), rand_unit());
		}
		else if (i < _traffic) {
			new_ship(&ships[i], w);
		}
		else {
			ships[i].alive = false;
			ships[i].respawn = 0;
		}
	}
	for (uint32_t i = 0; i < EXPLOSIONS; ++i) _explosions[i].on = false;
	for (uint32_t i = 0; i < MISSILES; ++i) _missiles[i].on = false;
	for (uint32_t i = 0; i < CANISTERS; ++i) canisters[i].on = false;
}

// A canister at pos, drifting along drift, if there is room for another
static void new_canister(Vec3_t pos, Vec3_t drift, bool alien) {
	for (uint32_t i = 0; i < CANISTERS; ++i) {
		Canister_t* c = &canisters[i];
		if (c->on) continue;
		c->on = true;
		frame_set(&c->fr, pos, rand_unit(), rand_unit());
		c->drift = drift;
		c->alien = alien;
		c->item = 0;
		c->roll = rand_range(-CANISTER_SPIN, CANISTER_SPIN);
		c->pitch = rand_range(-CANISTER_SPIN, CANISTER_SPIN);
		return;
	}
}

void ships_story_canister(uint8_t item, Vec3_t pos) {
	// In a space, else in place of an ordinary one
	Canister_t* c = NULL;
	for (uint32_t i = 0; i < CANISTERS && !c; ++i) {
		if (!canisters[i].on) c = &canisters[i];
	}
	for (uint32_t i = 0; i < CANISTERS && !c; ++i) {
		if (!canisters[i].item) c = &canisters[i];
	}
	if (!c) c = &canisters[0];
	c->on = true;
	frame_set(&c->fr, pos, rand_unit(), rand_unit());
	c->drift = (Vec3_t){ 0.0f, 0.0f, 0.0f };
	c->alien = false;
	c->item = item;
	c->roll = rand_range(-CANISTER_SPIN, CANISTER_SPIN);
	c->pitch = rand_range(-CANISTER_SPIN, CANISTER_SPIN);
}

void ships_debug_canister(Vec3_t pos) {
	new_canister(pos, (Vec3_t){ 0.0f, 0.0f, 0.0f }, false);
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

void ships_explode(Vec3_t pos, float size) {
	explode(pos, size);
}

// A story ship leaves its item, once
static void drop(Ship_t* s) {
	if (!s->drop) return;
	ships_story_canister(s->drop, vec_add(s->fr.pos, vec_scale(s->fr.u, s->model->radius * 1.5f)));
	s->drop = 0;
}

static void destroy(Ship_t* s, ShipsEvents_t* ev, bool by_us) {
	explode(s->fr.pos, s->model->radius);
	if (s->story) {
		ev->destroyed |= (uint8_t)(1u << (s->story - 1));
		drop(s);
		s->story = 0;
	}
	const bool thargon = s->role == ROLE_THARGON;
	const uint32_t cargo = s->role == ROLE_TRADER ? TRADER_CARGO : s->role == ROLE_PIRATE ? PIRATE_CARGO :
		thargon ? (s->hostile ? THARGON_ALIEN : THARGON_DEAD_ALIEN) : 0;
	if (percent() < cargo) new_canister(s->fr.pos, vec_scale(s->fr.f, s->speed * CANISTER_DRIFT), thargon);
	s->alive = false;
	s->beam = 0;
	// Nothing new comes in witchspace, or past the traffic, where only Thargons go
	s->respawn = _witch || (uint32_t)(s - ships) >= _traffic ? 0 : RESPAWN_FRAMES;
	if (by_us) {
		++ev->kills;
		const uint32_t bounty = ships_bounty((uint32_t)(s - ships));
		if (s->role == ROLE_PIRATE || s->role == ROLE_THARGOID || thargon) ev->bounty += bounty;
		else ev->innocent = true;
	}
	// A Thargoid's Thargons go dead, and drift, tumbling
	if (s->role == ROLE_THARGOID) {
		const int32_t me = (int32_t)(s - ships);
		for (uint32_t i = 0; i < SHIPS; ++i) {
			Ship_t* t = &ships[i];
			if (!t->alive || t->role != ROLE_THARGON || t->mother != me) continue;
			t->mother = -1;
			t->hostile = false;
			t->beam = 0;
			t->speed *= 0.2f;
			t->roll = rand_range(-0.04f, 0.04f);
			t->pitch = rand_range(-0.03f, 0.03f);
		}
	}
}

void ships_hit(uint32_t i, float damage, ShipsEvents_t* ev) {
	Ship_t* s = &ships[i];
	if (!s->alive) return;
	// Anyone shot at shoots back, or runs
	s->hostile = !s->disabled;
	s->energy -= damage;
	// A story ship that can be is disabled first, and drifts
	if (s->story && s->can_disable && !s->disabled && s->energy < s->cls->hull * DISABLED_HULL) {
		s->energy = fmaxf(s->energy, s->cls->hull * 0.1f);
		s->disabled = true;
		s->hostile = false;
		s->goal = GOAL_DRIFT;
		s->speed *= 0.2f;
		ev->disabled |= (uint8_t)(1u << (s->story - 1));
		drop(s);
		return;
	}
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

static void launch_thargon(Ship_t* mother, uint32_t i) {
	for (uint32_t k = 0; k < SHIPS; ++k) {
		Ship_t* t = &ships[k];
		if (t->alive) continue;
		set_class(t, &_classes[THARGON]);
		t->role = ROLE_THARGON;
		t->mother = (int32_t)i;
		t->fr = mother->fr;
		t->fr.pos = vec_add(mother->fr.pos, vec_scale(mother->fr.f, mother->model->radius + t->model->radius));
		t->speed = t->cls->speed;
		t->alive = true;
		t->hostile = true;
		t->cooldown = t->cls->rate;
		t->beam = 0;
		t->retreat = 0;
		t->respawn = 0;
		--mother->missiles;
		return;
	}
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
	// How fast it and we are closing, so it breaks off sooner the faster it is
	const Vec3_t closing_v = vec_sub(vec_scale(s->fr.f, s->speed), w->velocity);
	const float closing = fmaxf(vec_dot(closing_v, dir), 0.0f);
	if (d < c->close + closing * BREAK_AHEAD && !s->retreat) s->retreat = BREAK_FRAMES;
	if (s->retreat) {
		--s->retreat;
		// Across our way first, out of it, then away
		Vec3_t side = vec_sub(s->fr.f, vec_scale(dir, vec_dot(s->fr.f, dir)));
		if (vec_length(side) < 0.1f) side = s->fr.r;
		const Vec3_t away = vec_unit(vec_sub(vec_unit(side), vec_scale(dir, 0.5f)));
		turn_towards(&s->fr, away, c->turn * 1.5f);
		s->speed = c->speed * BREAK_SPEED;
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

	// Fires when pointing at us, and close enough to do some good, as a laser
	// does less the further it goes
	if (s->cooldown) --s->cooldown;
	const bool pointing = vec_dot(s->fr.f, dir) > SHOT_AIM;
	const float falloff = ships_laser_falloff(d, c->range);
	if (!s->cooldown && pointing && d < c->range && falloff >= HOLD_FIRE) {
		s->cooldown = c->rate;
		s->beam = BEAM_FRAMES;
		if (percent() < c->aim) {
			// On the shield on the side it is
			if (vec_dot(vec_scale(to, -1.0f), w->me->f) >= 0.0f) ev->fore += c->damage * falloff;
			else ev->aft += c->damage * falloff;
		}
	}
	// A Thargoid launches its Thargons into any room there is
	if (s->role == ROLE_THARGOID) {
		if (s->missiles && d < THARGON_RANGE && (uint32_t)(rand() % 10000) < THARGON_CHANCE) launch_thargon(s, i);
		return;
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

// A story ship going about what it does, when not fighting
static void story_ship(Ship_t* s, const ShipsWorld_t* w, ShipsEvents_t* ev) {
	const uint8_t bit = (uint8_t)(1u << (s->story - 1));
	const Vec3_t to_me = vec_sub(w->me->pos, s->fr.pos);
	const float d = vec_length(to_me);
	if (!s->near && d < STORY_NEAR) {
		s->near = true;
		ev->near |= bit;
	}
	if (s->hostile) return;
	switch (s->goal) {
		case GOAL_FIGHT:
			s->hostile = true;
			break;
		case GOAL_RACE: {
			const Vec3_t to = vec_sub(w->planet, s->fr.pos);
			const float l = vec_length(to);
			turn_towards(&s->fr, vec_scale(to, 1.0f / fmaxf(l, 1.0f)), s->cls->turn);
			s->speed = s->cls->speed;
			if (l < w->planet_radius * RACE_DONE) {
				s->alive = false;
				s->story = 0;
				ev->escaped |= bit;
			}
			break;
		}
		case GOAL_DRIFT:
			s->speed = fminf(s->speed, 2.0f);
			frame_roll(&s->fr, 0.004f);
			frame_pitch(&s->fr, 0.002f);
			frame_tidy(&s->fr);
			break;
		case GOAL_FOLLOW: {
			// To a point beside us, slowing as it gets there
			const Vec3_t at = vec_add(w->me->pos, vec_add(vec_scale(w->me->r, FOLLOW_DISTANCE), vec_scale(w->me->f, -FOLLOW_DISTANCE * 0.5f)));
			const Vec3_t to = vec_sub(at, s->fr.pos);
			const float l = vec_length(to);
			if (l > 1.0f) turn_towards(&s->fr, vec_scale(to, 1.0f / l), s->cls->turn);
			s->speed = fminf(s->cls->speed, l * 0.02f + vec_length(w->velocity));
			break;
		}
		case GOAL_WEAVE: {
			const float t = (float)_frame * 0.03f;
			frame_roll(&s->fr, 0.03f * sinf(t));
			frame_pitch(&s->fr, 0.02f * cosf(t * 1.7f));
			frame_tidy(&s->fr);
			s->speed = s->cls->speed * 0.4f;
			break;
		}
		default:
			break;
	}
}

void ships_story_spawn(uint8_t id, const char* label, const char* cls, uint8_t goal, bool can_disable, uint8_t item,
	Vec3_t pos, Vec3_t f, Vec3_t u, const ShipsWorld_t* w) {
	// In place of one of the same id, else in a space, else in place of an
	// ordinary ship minding its business
	Ship_t* s = NULL;
	for (uint32_t i = 0; i < SHIPS && !s; ++i) {
		if (ships[i].alive && ships[i].story == id) s = &ships[i];
	}
	for (uint32_t i = 0; i < SHIPS && !s; ++i) {
		if (!ships[i].alive) s = &ships[i];
	}
	for (uint32_t i = 0; i < SHIPS && !s; ++i) {
		if (!ships[i].story && !ships[i].hostile) s = &ships[i];
	}
	if (!s) s = &ships[SHIPS - 1];
	new_ship(s, w);
	set_class(s, class_named(cls));
	s->role = s->cls == &_classes[THARGOID] ? ROLE_THARGOID : goal == GOAL_FIGHT ? ROLE_PIRATE : ROLE_TRADER;
	s->hostile = false;
	s->story = id;
	s->goal = goal;
	s->can_disable = can_disable;
	s->disabled = goal == GOAL_DRIFT;
	s->drop = item;
	s->label = label;
	frame_set(&s->fr, pos, f, u);
	s->speed = goal == GOAL_DRIFT ? 1.0f : s->cls->speed * 0.5f;
}

void ships_calm_aliens(void) {
	for (uint32_t i = 0; i < SHIPS; ++i) {
		if (ships[i].alive && (ships[i].role == ROLE_THARGOID || ships[i].role == ROLE_THARGON)) ships[i].hostile = false;
	}
}

void ships_story_calm(void) {
	for (uint32_t i = 0; i < SHIPS; ++i) {
		Ship_t* s = &ships[i];
		if (!s->alive || !s->story) continue;
		s->hostile = false;
		if (s->goal == GOAL_FIGHT) s->goal = GOAL_IDLE;
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

static void move_canisters(const ShipsWorld_t* w, ShipsEvents_t* ev) {
	for (uint32_t i = 0; i < CANISTERS; ++i) {
		Canister_t* c = &canisters[i];
		if (!c->on) continue;
		frame_roll(&c->fr, c->roll);
		frame_pitch(&c->fr, c->pitch);
		frame_tidy(&c->fr);
		c->fr.pos = vec_add(c->fr.pos, c->drift);
		const Vec3_t to = vec_sub(c->fr.pos, w->me->pos);
		const float d = vec_length(to);
		if (d < model_canister.radius + RAM_MARGIN) {
			c->on = false;
			// A story's, taken aboard gently, scoops or not
			if (c->item) {
				if (ev->items_n < 2) ev->items[ev->items_n++] = c->item;
			}
			// Not well above us, it goes into the scoops
			else if (w->scoops && vec_dot(to, w->me->u) < SCOOP_ABOVE * d) {
				++ev->scooped;
				if (c->alien) ++ev->aliens;
			}
			else {
				++ev->smashed;
				explode(c->fr.pos, model_canister.radius);
				if (vec_dot(to, w->me->f) >= 0.0f) ev->fore += CANISTER_DAMAGE;
				else ev->aft += CANISTER_DAMAGE;
			}
		}
		else if (d > LEAVE_RANGE) {
			c->on = false;
		}
	}
}

void ships_move(const ShipsWorld_t* w, ShipsEvents_t* ev) {
	++_frame;
	// All of them, as Thargons can be past the traffic
	for (uint32_t i = 0; i < SHIPS; ++i) {
		Ship_t* s = &ships[i];
		if (!s->alive) {
			if (s->respawn && --s->respawn == 0) new_ship(s, w);
			continue;
		}
		if (s->beam) --s->beam;
		// The police come for us once we are an offender
		if (s->role == ROLE_POLICE && w->offender) s->hostile = true;
		if (s->story) story_ship(s, w, ev);
		if (s->hostile) {
			attack(s, i, w, ev);
		}
		else if (s->story && s->goal != GOAL_IDLE) {
			// Gone its own way, above
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
			ev->rammed = true;
			destroy(s, ev, true);
		}
		else if (!_witch && !s->story && d > LEAVE_RANGE && vec_length(vec_sub(s->fr.pos, w->station)) > LEAVE_RANGE) {
			new_ship(s, w);
		}
	}
	move_missiles(w, ev);
	move_canisters(w, ev);
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

// How far a ship is off the middle of the view, as across for each ahead, or
// -1 if it is out of range or the cone
static float off_middle(const Frame_t* me, uint32_t i, float range, float cone) {
	const Ship_t* s = &ships[i];
	if (!s->alive) return -1.0f;
	const Vec3_t v = frame_view(me, s->fr.pos);
	if (v.z <= 0.0f || v.z > range) return -1.0f;
	const float off = sqrtf(v.x * v.x + v.y * v.y) / v.z;
	return off < cone ? off : -1.0f;
}

int32_t ships_tracked(const Frame_t* me, float range, float cone, int32_t keep) {
	if (keep >= 0 && off_middle(me, (uint32_t)keep, range, cone) >= 0.0f) return keep;
	int32_t best = -1;
	float best_off = cone;
	for (uint32_t i = 0; i < SHIPS; ++i) {
		const float off = off_middle(me, i, range, cone);
		if (off >= 0.0f && off < best_off) {
			best = (int32_t)i;
			best_off = off;
		}
	}
	return best;
}

bool ships_launch_police(const Frame_t* from, const ShipsWorld_t* w) {
	// Into a space, else in place of a trader minding its business
	Ship_t* s = NULL;
	for (uint32_t i = 0; i < SHIPS && !s; ++i) {
		if (!ships[i].alive) s = &ships[i];
	}
	for (uint32_t i = 0; i < SHIPS && !s; ++i) {
		if (ships[i].role == ROLE_TRADER && !ships[i].hostile) s = &ships[i];
	}
	if (!s) return false;
	new_ship(s, w);
	s->role = ROLE_POLICE;
	set_class(s, &_classes[POLICE]);
	s->hostile = true;
	s->fr = *from;
	s->speed = s->cls->speed * 0.5f;
	return true;
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

void ships_bomb(Vec3_t me, float range, ShipsEvents_t* ev) {
	for (uint32_t i = 0; i < SHIPS; ++i) {
		Ship_t* s = &ships[i];
		if (s->alive && vec_length(vec_sub(s->fr.pos, me)) < range) destroy(s, ev, true);
	}
	ships_ecm();
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
