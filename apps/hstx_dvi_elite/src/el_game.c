// Copyright (c) 2024 fruit-bat.

// See el_game.h.
//
// As in the original game, we are always at the middle of things: each frame
// everything is seen from our ship, along its axes.
//
// A game starts docked at Lave, as in the original. To dock again, fly into
// the station's slot, pointing at the station from in front of the slot,
// with the slot level: the station spins, so roll to match it. Touching the
// station any other way, or the planet, is the end of us.
//
// The controls:
//   left and right roll, and up and down dive and climb, as with a joystick
//   B held with up or down speeds up or slows down
//   B held with fire launches a missile, and with select sets off the ECM
//   docked, select also shows the market, where up and down pick a good,
//   right buys and left sells, and the equipment, where fire buys
//   A launches us when docked, from the status, and on the charts or data starts the
//   countdown to hyperspace, to the system the cursor is on, if it is in
//   range of our fuel
//   select shows the next screen: the view, or our status when docked, the
//   charts and the data on the system the cursor is on, which the pad moves
//   on the charts
//   start shows the options

#include "el_game.h"
#include "el_chart.h"
#include "el_draw.h"
#include "el_hud.h"
#include "el_input.h"
#include "el_screen.h"
#include "el_save.h"
#include "el_ships.h"
#include "el_space.h"
#include "el_trade.h"
#include "game_pallet.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

// The system: a planet as big as its data says, in units for each km, the
// station going round it with its slot facing it, this many planet radii
// from its middle, and the sun far beyond. Where the sun and the station are
// comes from the system's numbers, so it is the same each time we visit.
#define PLANET_SCALE 3.0f
#define STATION_ORBIT 2.2f
#define STATION_SPIN 0.005f
#define SUN_RADIUS 30000.0f
#define SUN_DISTANCE 330000.0f
// Close to the station the compass points to it, and otherwise to the
// planet, in planet radii
#define SAFE_ZONE 3.3f
// Out of hyperspace this many planet radii from the planet's middle, facing it
#define ARRIVAL 6.0f
// The countdown to hyperspace, in seconds as in the original, and the jump
#define HYPER_COUNT 10
#define HYPER_FRAMES 150
// Hyperspace is not allowed this close to anything: within this many planet
// radii of the planet's surface, sun radii of the sun's middle, or units of
// the station or another ship
#define LOCK_PLANET 1.0f
#define LOCK_SUN 3.0f
#define LOCK_STATION 8000.0f
#define LOCK_SHIP 6000.0f
// How long a message, such as that a system is too far, is shown for
#define MESSAGE_FRAMES 120
// Where we start, in front of the slot, in station radii from its middle
#define LAUNCH_DISTANCE 2.0f
// The station is a cuboctahedron: its square faces this far from its middle,
// and its triangles twice that along all three axes together. The slot is
// in the middle of the front face.
#define STATION_FACE 400.0f
// How close to the station we can be, as our ship has some size
#define STATION_MARGIN 30.0f
// To dock we must be within this of straight in front of the slot, and
// pointing at the station within it, as cosines, about 26 degrees
#define DOCK_CONE 0.9f
// and the slot must be level to within this, as a sine, about 25 degrees
#define DOCK_LEVEL 0.42f
// The squares that guide us in to the slot, shown when in front of it and
// this close: how many, from how far out from the station's middle, how far
// apart, and how big, half across, nearest the slot and growing outwards
#define GUIDE_RANGE 6000.0f
#define GUIDES 8
#define GUIDE_NEAR 600.0f
#define GUIDE_GAP 500.0f
#define GUIDE_SIZE 150.0f
#define GUIDE_GROWTH 0.07f
// How long going through the tunnel takes, launching or docking, and how
// long the end of a game is shown for
#define TUNNEL_FRAMES 90
#define DEAD_FRAMES (4 * 60)
// Where we start in the galaxy: Lave
#define HOME 7

// How fast we can go, in units a frame, and turn, in radians a frame, and
// how quickly each changes while the buttons are held
#define MAX_SPEED 40.0f
#define SPEED_STEP 0.6f
#define MAX_ROLL 0.035f
#define ROLL_STEP 0.003f
#define MAX_PITCH 0.022f
#define PITCH_STEP 0.002f
// How far the cursor moves on the charts each frame, in steps across the galaxy
#define CURSOR_STEP_LOCAL 0.25f
#define CURSOR_STEP_GALAXY 0.75f
#define CURSOR_FAST 3.0f

#define SCANNER_RANGE 12000.0f
// Further than this, the other ships are not seen at all
#define SHIP_SEEN 20000.0f

// Our shields and energy go from 0 to 1, and build up again by this much a frame
#define RECHARGE 0.0006f
// Our laser: how far it reaches, for how long a shot is seen, how fast it
// cools, and how cool it must be to fire again once too hot
#define LASER_RANGE 8000.0f
#define LASER_SEEN 4
#define LASER_COOL 0.0025f
#define LASER_COOLED 0.6f

// Each laser, as Laser_e: what a shot does, how often it fires while fire
// is held, and how much a shot heats it. The beam and military lasers fire
// all the time, so they are seen all the time.
typedef struct {
	float damage;
	uint32_t every;
	float heat;
} Laser_t;

static const Laser_t _lasers[] = {
	{ 1.0f, 10, 0.05f },
	{ 0.6f, 4, 0.025f },
	{ 1.0f, 4, 0.022f },
};

// The front view is our status when docked
// The market and equipment are only seen docked
typedef enum {
	VIEW_FRONT = 0,
	VIEW_MARKET,
	VIEW_EQUIPMENT,
	VIEW_LOCAL,
	VIEW_GALAXY,
	VIEW_DATA,
	VIEWS
} View_e;

static const char* const _view_labels[VIEWS] = {
	"FRONT VIEW", "MARKET PRICES", "EQUIP SHIP", "SHORT RANGE CHART", "GALACTIC CHART", "DATA ON SYSTEM"
};

typedef enum {
	STATE_DOCKED = 0,
	STATE_LAUNCHING,
	STATE_FLYING,
	STATE_DOCKING,
	STATE_HYPERSPACE,
	STATE_DEAD
} State_e;

typedef enum {
	OPTION_STYLE = 0,
	OPTION_DITHER,
	OPTION_SCREEN,
	OPTION_END,
	OPTIONS
} Option_e;

static Frame_t _me;
static float _speed, _roll, _pitch;
static Frame_t _planet, _station;
static float _planet_radius;
static Vec3_t _sun_pos;
// Frames left counting down to hyperspace, and where to
static uint32_t _hyper_frames;
static uint32_t _hyper_to;
static const char* _message;
static uint32_t _message_frames;
// Our ship and how we have done
static float _fore, _aft, _energy;
static float _laser_temp;
static bool _overheated;
static uint32_t _laser_wait, _laser_beam;
static uint32_t _kills;
// The slot we save to, or -1 if there is nowhere to save
static int32_t _slot = -1;
static bool _offender;
static View_e _view;
static State_e _state;
// Frames into the state, for the tunnel and the end
static uint32_t _state_frames;
static bool _options;
static uint32_t _option;
static uint32_t _frame;
static GameText_t _option_text[2][OPTIONS + 1];

// Moves a value towards a target by at most step
static float towards(float v, float target, float step) {
	if (v < target) return fminf(v + step, target);
	return fmaxf(v - step, target);
}

// What the other ships need to know
static ShipsWorld_t world(void) {
	const GalaxySystem_t* sys = chart_system(chart_home());
	return (ShipsWorld_t){ &_me, _station.pos, sys->government, sys->economy, sys->population, _offender };
}

// Repaired, as when docked
static void refit(void) {
	_fore = _aft = _energy = 1.0f;
	_laser_temp = 0.0f;
	_overheated = false;
}

// A number from -1 to 1 made from the system's numbers, the k-th of several
static float system_number(const GalaxySystem_t* sys, uint32_t k) {
	uint32_t h = sys->seed[0] * 2654435761u ^ sys->seed[1] * 40503u ^ (uint32_t)sys->seed[2] << 7 ^ k * 0x9e3779b9u;
	h ^= h >> 15;
	h *= 0x2c1b3c6du;
	h ^= h >> 12;
	return (float)(h & 0xffff) / 32767.5f - 1.0f;
}

// A way from the angle round from +z and the angle up
static Vec3_t direction(float round, float up) {
	return (Vec3_t){ sinf(round) * cosf(up), sinf(up), cosf(round) * cosf(up) };
}

// Sets up the system we are at, with the planet in the middle and coming in
// from -z: the sun ahead of us off to one side, and the station on our side
// of the planet
static void enter_system(uint32_t i) {
	chart_set_home(i);
	const GalaxySystem_t* sys = chart_system(i);
	_planet_radius = PLANET_SCALE * (float)sys->radius;
	frame_set(&_planet, (Vec3_t){ 0.0f, 0.0f, 0.0f }, (Vec3_t){ 0.0f, 0.0f, 1.0f },
		(Vec3_t){ 0.3f * system_number(sys, 0), 1.0f, 0.0f });
	float round = 1.2f * system_number(sys, 1);
	if (fabsf(round) < 0.3f) round = round < 0.0f ? -0.3f : 0.3f;
	_sun_pos = vec_scale(direction(round, 0.4f * system_number(sys, 2)), SUN_DISTANCE);
	const Vec3_t sp = vec_scale(direction(PI_F + 0.8f * system_number(sys, 3), 0.5f * system_number(sys, 4)),
		STATION_ORBIT * _planet_radius);
	// Its slot faces the planet
	frame_set(&_station, sp, vec_unit(vec_sub(_planet.pos, sp)), (Vec3_t){ 0.0f, 1.0f, 0.0f });
	const ShipsWorld_t w = world();
	ships_enter(&w);
}

static void start(void) {
	_offender = false;
	chart_set_galaxy(0);
	enter_system(HOME);
	frame_set(&_me, _station.pos, _station.f, _station.u);
	_speed = 0.0f;
	_roll = 0.0f;
	_pitch = 0.0f;
	_kills = 0;
	trade_new_commander();
	trade_arrive(chart_system(chart_home()));
	chart_set_fuel(cmdr.fuel);
	refit();
	_laser_wait = _laser_beam = 0;
	_hyper_frames = 0;
	_message_frames = 0;
	_view = VIEW_FRONT;
	_state = STATE_DOCKED;
	_state_frames = 0;
	_options = false;
	_option = 0;
	_frame = 0;
	dust_init();
}

static void set_state(State_e state) {
	_state = state;
	_state_frames = 0;
	// Docking or dying stops a countdown to hyperspace
	if (state == STATE_DOCKING || state == STATE_DEAD) _hyper_frames = 0;
	// The tunnels are seen ahead
	if (state == STATE_LAUNCHING || state == STATE_DOCKING || state == STATE_HYPERSPACE) _view = VIEW_FRONT;
}

static void message(const char* m) {
	_message = m;
	_message_frames = MESSAGE_FRAMES;
}

// Whether we are too close to something to go into hyperspace
static bool mass_locked(void) {
	if (vec_length(vec_sub(_me.pos, _planet.pos)) < (1.0f + LOCK_PLANET) * _planet_radius) return true;
	if (vec_length(vec_sub(_me.pos, _sun_pos)) < LOCK_SUN * SUN_RADIUS) return true;
	if (vec_length(vec_sub(_me.pos, _station.pos)) < LOCK_STATION) return true;
	for (uint32_t i = 0; i < SHIPS; ++i) {
		if (ships[i].alive && vec_length(vec_sub(_me.pos, ships[i].fr.pos)) < LOCK_SHIP) return true;
	}
	return false;
}

// Starts the countdown to the system the cursor is on, if we can reach it
static void start_hyperspace(void) {
	if (_hyper_frames) return;
	if (mass_locked()) {
		message("MASS LOCKED");
		return;
	}
	const uint32_t to = chart_selected();
	if (to == chart_home()) {
		message("WE ARE HERE");
		return;
	}
	if (galaxy_distance(chart_system(chart_home()), chart_system(to)) > cmdr.fuel) {
		message("OUT OF RANGE");
		return;
	}
	_hyper_to = to;
	_hyper_frames = HYPER_COUNT * 60;
}

// Out of hyperspace in the new system, facing its planet
static void arrive(void) {
	cmdr.fuel -= galaxy_distance(chart_system(chart_home()), chart_system(_hyper_to));
	chart_set_fuel(cmdr.fuel);
	// The police of the new system do not know us, for now
	_offender = false;
	enter_system(_hyper_to);
	trade_arrive(chart_system(_hyper_to));
	frame_set(&_me, (Vec3_t){ 0.0f, 0.0f, -ARRIVAL * _planet_radius }, (Vec3_t){ 0.0f, 0.0f, 1.0f }, (Vec3_t){ 0.0f, 1.0f, 0.0f });
	_speed = MAX_SPEED * 0.25f;
	_roll = 0.0f;
	_pitch = 0.0f;
	_view = VIEW_FRONT;
	dust_init();
	set_state(STATE_FLYING);
}

// Saves us, if there is somewhere to, as when docked
static void save(void) {
	if (_slot < 0) return;
	static Save_t s;
	s.cmdr = cmdr;
	s.galaxy = chart_galaxy();
	s.home = chart_home();
	s.kills = _kills;
	s.offender = _offender;
	if (!save_write((uint32_t)_slot, &s)) message("SAVE FAILED");
}

// Out through the slot, facing the way it does, which is towards the planet,
// saving first what we bought
static void launch(void) {
	save();
	frame_set(&_me, vec_add(_station.pos, vec_scale(_station.f, model_coriolis.radius * LAUNCH_DISTANCE)), _station.f, _station.u);
	_speed = MAX_SPEED * 0.25f;
	_roll = 0.0f;
	_pitch = 0.0f;
	_view = VIEW_FRONT;
	dust_init();
	set_state(STATE_LAUNCHING);
}

// For trying docking on the PC: puts us on a run at the slot, from far
// enough out to be going at full speed, rolled from level by roll
void game_debug_approach(float roll) {
	const Vec3_t out = vec_add(_station.pos, vec_scale(_station.f, 1500.0f));
	frame_set(&_me, out, vec_scale(_station.f, -1.0f), _station.u);
	frame_roll(&_me, roll);
	frame_tidy(&_me);
	_speed = MAX_SPEED;
	_roll = 0.0f;
	_pitch = 0.0f;
	_view = VIEW_FRONT;
	set_state(STATE_FLYING);
}

// Whether a point is inside the station, or close enough to touch it
static bool in_station(Vec3_t local) {
	const float x = fabsf(local.x), y = fabsf(local.y), z = fabsf(local.z);
	const float f = STATION_FACE + STATION_MARGIN;
	return x <= f && y <= f && z <= f && x + y + z <= 2.0f * STATION_FACE + STATION_MARGIN * 1.7f;
}

// Docks us if we have come in through the slot, and ends the game if we have hit the station
static void touch_station(void) {
	const Vec3_t d = vec_sub(_me.pos, _station.pos);
	const Vec3_t local = { vec_dot(d, _station.r), vec_dot(d, _station.u), vec_dot(d, _station.f) };
	if (!in_station(local)) return;
	const float dist = vec_length(d);
	// In front of the slot, pointing at the station, with the slot level
	const bool in_front = dist > 0.0f && local.z / dist > DOCK_CONE;
	const bool pointing = dist > 0.0f && -vec_dot(d, _me.f) / dist > DOCK_CONE;
	const bool level = fabsf(vec_dot(_station.r, _me.u)) < DOCK_LEVEL;
	if (in_front && pointing && level) set_state(STATE_DOCKING);
	else set_state(STATE_DEAD);
}

// ----------------------------------------------------------------------------
// Moving

// The pad moves the cursor on the charts, faster with B held
static void move_cursor(uint8_t held) {
	if (_view != VIEW_LOCAL && _view != VIEW_GALAXY) return;
	const bool local = _view == VIEW_LOCAL;
	float step = local ? CURSOR_STEP_LOCAL : CURSOR_STEP_GALAXY;
	if (held & EL_INP_B) step *= CURSOR_FAST;
	float dx = 0.0f, dy = 0.0f;
	if (held & EL_INP_RIGHT) dx += step;
	if (held & EL_INP_LEFT) dx -= step;
	if (held & EL_INP_DOWN) dy += step;
	if (held & EL_INP_UP) dy -= step;
	chart_cursor_move(dx, dy, local);
}

static void fly(uint8_t held) {
	const bool b = held & EL_INP_B;
	float roll_in = 0.0f, pitch_in = 0.0f;
	if (_view == VIEW_FRONT) {
		if (held & EL_INP_RIGHT) roll_in += 1.0f;
		if (held & EL_INP_LEFT) roll_in -= 1.0f;
		if (!b) {
			// As a joystick: pulling back, down, climbs
			if (held & EL_INP_DOWN) pitch_in += 1.0f;
			if (held & EL_INP_UP) pitch_in -= 1.0f;
		}
	}
	else {
		move_cursor(held);
	}
	if (b && _view == VIEW_FRONT) {
		if (held & EL_INP_UP) _speed = fminf(_speed + SPEED_STEP, MAX_SPEED);
		if (held & EL_INP_DOWN) _speed = fmaxf(_speed - SPEED_STEP, 0.0f);
	}
	_roll = towards(_roll, roll_in * MAX_ROLL, ROLL_STEP);
	_pitch = towards(_pitch, pitch_in * MAX_PITCH, PITCH_STEP);

	frame_roll(&_me, _roll);
	frame_pitch(&_me, _pitch);
	frame_tidy(&_me);
	_me.pos = vec_add(_me.pos, vec_scale(_me.f, _speed));
	dust_move(_roll, _pitch, _speed);

	touch_station();
	if (vec_length(vec_sub(_me.pos, _planet.pos)) < _planet_radius) set_state(STATE_DEAD);
}

// The pulse laser while fire is held, and a missile at the ship nearest
// ahead when fire is pressed with B held
static void fire(uint8_t held, uint8_t pressed, ShipsEvents_t* ev) {
	if (_laser_wait) --_laser_wait;
	if (_laser_beam) --_laser_beam;
	_laser_temp = fmaxf(0.0f, _laser_temp - LASER_COOL);
	if (_overheated && _laser_temp < LASER_COOLED) _overheated = false;
	if (_view != VIEW_FRONT) return;
	if (held & EL_INP_B) {
		if (!(pressed & EL_INP_A)) return;
		const int32_t target = ships_nearest_ahead(&_me);
		if (!cmdr.missiles) message("NO MISSILES");
		else if (target < 0) message("NO TARGET");
		else if (ships_fire_missile(&_me, (uint32_t)target)) --cmdr.missiles;
		return;
	}
	if (!(held & EL_INP_A) || _laser_wait || _overheated) return;
	const Laser_t* l = &_lasers[cmdr.laser];
	_laser_wait = l->every;
	_laser_beam = LASER_SEEN;
	_laser_temp += l->heat;
	if (_laser_temp >= 1.0f) {
		_laser_temp = 1.0f;
		_overheated = true;
		message("LASER TEMPERATURE");
	}
	const int32_t i = ships_in_sights(&_me, LASER_RANGE);
	if (i >= 0) ships_hit((uint32_t)i, l->damage, ev);
}

// Hits on our shields, then on our energy once a shield is down, and what we destroyed
static void take(const ShipsEvents_t* ev) {
	_fore -= ev->fore;
	_aft -= ev->aft;
	if (_fore < 0.0f) {
		_energy += _fore;
		_fore = 0.0f;
	}
	if (_aft < 0.0f) {
		_energy += _aft;
		_aft = 0.0f;
	}
	if (ev->missile) message("INCOMING MISSILE");
	_kills += ev->kills;
	cmdr.cash += ev->bounty;
	if (ev->innocent && !_offender) {
		_offender = true;
		message("OFFENDER");
	}
	if (_energy <= 0.0f) {
		_energy = 0.0f;
		if (_state != STATE_DEAD) set_state(STATE_DEAD);
		return;
	}
	_energy = fminf(1.0f, _energy + RECHARGE);
	_fore = fminf(1.0f, _fore + RECHARGE);
	_aft = fminf(1.0f, _aft + RECHARGE);
}

static void move_world(ShipsEvents_t* ev) {
	frame_roll(&_station, STATION_SPIN);
	frame_tidy(&_station);
	const ShipsWorld_t w = world();
	ships_move(&w, ev);
}

// For trying a fight on the PC: puts a ship of the named type ahead of us, coming at us
void game_debug_pirate(const char* name) {
	const ShipsWorld_t w = world();
	ships_debug_place(0, name, vec_add(_me.pos, vec_scale(_me.f, 2500.0f)), vec_scale(_me.f, -1.0f), _me.u, &w);
}

// ----------------------------------------------------------------------------
// Drawing

typedef enum {
	THING_PLANET,
	THING_SUN,
	THING_STATION,
	THING_SHIP
} Thing_e;

typedef struct {
	Thing_e kind;
	uint32_t i;
	float near; // how far its nearest point is
} Thing_t;

static void draw_dashboard(void) {
	const Vec3_t planet = frame_view(&_me, _planet.pos);
	const Vec3_t station = frame_view(&_me, _station.pos);
	const float altitude = (vec_length(planet) - _planet_radius) / _planet_radius;
	Hud_t hud = {
		.fore_shield = _fore,
		.aft_shield = _aft,
		.fuel = (float)cmdr.fuel / GALAXY_FUEL_MAX,
		.cabin_temp = 0.1f,
		.laser_temp = _laser_temp,
		.altitude = fminf(fmaxf(altitude, 0.0f), 1.0f),
		.speed = _speed / MAX_SPEED,
		.energy = _energy,
		.roll = _roll / MAX_ROLL,
		.pitch = _pitch / MAX_PITCH,
		.missiles = cmdr.missiles,
		.compass = vec_length(station) < SAFE_ZONE * _planet_radius ? station : planet,
		.scanner_range = SCANNER_RANGE,
		.sights = _view == VIEW_FRONT && _state == STATE_FLYING && !_options,
	};
	Blip_t blips[SHIPS + 1];
	uint32_t n = 0;
	blips[n++] = (Blip_t){ station, model_coriolis.rgb, true };
	for (uint32_t i = 0; i < SHIPS; ++i) {
		if (ships[i].alive) blips[n++] = (Blip_t){ frame_view(&_me, ships[i].fr.pos), ships[i].model->rgb, false };
	}
	hud_draw(&hud, blips, n, draw_style);
}

static void draw_body(const WireModel_t* model, const Frame_t* fr) {
	float m[3][3];
	frame_matrix(&_me, fr, m);
	draw_object(model, frame_view(&_me, fr->pos), m, model->rgb, draw_style);
}

// Whether we are lined up to dock: pointing at the station with the slot level
static bool lined_up(void) {
	const Vec3_t d = vec_sub(_station.pos, _me.pos);
	const float dist = vec_length(d);
	return dist > 0.0f && vec_dot(d, _me.f) / dist > DOCK_CONE && fabsf(vec_dot(_station.r, _me.u)) < DOCK_LEVEL;
}

// Squares in space in front of the slot, turning with the station, green
// when we are lined up. Always wireframes, whatever the style, as they are
// only there to guide us.
static void draw_guides(void) {
	const Vec3_t d = vec_sub(_me.pos, _station.pos);
	if (vec_length(d) > GUIDE_RANGE || vec_dot(d, _station.f) < STATION_FACE) return;
	const hstx_dvi_pixel_t c = lined_up() ? hstx_dvi_pixel_rgb(0, 255, 0) : hstx_dvi_pixel_rgb(255, 182, 0);
	for (uint32_t i = 0; i < GUIDES; ++i) {
		const float z = GUIDE_NEAR + GUIDE_GAP * (float)i;
		const float h = GUIDE_SIZE + (z - GUIDE_NEAR) * GUIDE_GROWTH;
		const Vec3_t mid = vec_add(_station.pos, vec_scale(_station.f, z));
		Vec3_t corner[4];
		for (uint32_t k = 0; k < 4; ++k) {
			const float x = (k == 0 || k == 3) ? -h : h;
			const float y = k < 2 ? h : -h;
			corner[k] = frame_view(&_me, vec_add(mid, vec_add(vec_scale(_station.r, x), vec_scale(_station.u, y))));
		}
		if (!object_begin()) return;
		for (uint32_t k = 0; k < 4; ++k) add_edge(corner[k], corner[(k + 1) % 4], c);
		object_end();
	}
}

static void draw_front(void) {
	// The light comes from the sun
	const Vec3_t sun = frame_view(&_me, _sun_pos);
	draw_light = vec_unit(sun);
	// Our laser, from the bottom corners of the view to the sights
	if (_laser_beam && object_begin()) {
		const hstx_dvi_pixel_t c = hstx_dvi_pixel_rgb(255, 255, 146);
		const float w = 220.0f * draw_aspect;
		add_line((Vec2_t){ CENTRE_X - w, VIEW_BOTTOM - 1 }, (Vec2_t){ CENTRE_X - 3, CENTRE_Y + 2 }, c);
		add_line((Vec2_t){ CENTRE_X + w, VIEW_BOTTOM - 1 }, (Vec2_t){ CENTRE_X + 3, CENTRE_Y + 2 }, c);
		object_end();
	}
	ships_draw_effects(&_me, draw_style, _frame);
	// In front of the slot, so nearer than the station
	draw_guides();

	// Nearest first, so they cover the ones behind
	Thing_t things[SHIPS + 3];
	uint32_t n = 0;
	things[n++] = (Thing_t){ THING_PLANET, 0, vec_length(frame_view(&_me, _planet.pos)) - _planet_radius };
	things[n++] = (Thing_t){ THING_SUN, 0, vec_length(sun) - SUN_RADIUS };
	things[n++] = (Thing_t){ THING_STATION, 0, vec_length(frame_view(&_me, _station.pos)) - model_coriolis.radius };
	for (uint32_t i = 0; i < SHIPS; ++i) {
		if (!ships[i].alive) continue;
		const float d = vec_length(frame_view(&_me, ships[i].fr.pos));
		if (d < SHIP_SEEN) things[n++] = (Thing_t){ THING_SHIP, i, d - ships[i].model->radius };
	}
	for (uint32_t i = 1; i < n; ++i) {
		const Thing_t t = things[i];
		uint32_t j = i;
		for (; j > 0 && things[j - 1].near > t.near; --j) things[j] = things[j - 1];
		things[j] = t;
	}

	const GalaxySystem_t* sys = chart_system(chart_home());
	for (uint32_t i = 0; i < n; ++i) {
		const Thing_t* t = &things[i];
		switch (t->kind) {
			case THING_PLANET: {
				float m[3][3];
				frame_matrix(&_me, &_planet, m);
				draw_planet(frame_view(&_me, _planet.pos), _planet_radius, m, chart_economy_rgb(sys->economy),
					!(sys->seed[1] & 2), draw_style);
				break;
			}
			case THING_SUN:
				if (sun.z > SUN_RADIUS) {
					const Vec2_t m = project(&sun);
					const float rp = FOCAL * SUN_RADIUS / sun.z;
					if (draw_style == STYLE_WIRE || !hidden(m, rp * SUN_RAY_REACH + 1.0f)) draw_sun(m, rp, draw_style, _frame);
				}
				break;
			case THING_STATION:
				draw_body(&model_coriolis, &_station);
				break;
			case THING_SHIP:
				draw_body(ships[t->i].model, &ships[t->i].fr);
				break;
		}
	}
	// Behind it all
	dust_draw();
}

// ----------------------------------------------------------------------------
// The options, over the view while the game waits

static bool options(uint8_t pressed) {
	if (pressed & EL_INP_START) {
		_options = false;
		return false;
	}
	if (pressed & EL_INP_DOWN) _option = (_option + 1) % OPTIONS;
	if (pressed & EL_INP_UP) _option = (_option + OPTIONS - 1) % OPTIONS;
	const int32_t change = (pressed & EL_INP_RIGHT ? 1 : 0) - (pressed & EL_INP_LEFT ? 1 : 0);
	switch (_option) {
		case OPTION_STYLE:
			if (change) draw_style = (Style_e)((draw_style + STYLE_COUNT + change) % STYLE_COUNT);
			break;
		case OPTION_DITHER:
			if (change) draw_dither = !draw_dither;
			break;
		case OPTION_SCREEN:
			if (change) {
				draw_aspect = draw_aspect == ASPECT_16_9 ? ASPECT_4_3 : ASPECT_16_9;
				chart_refresh();
			}
			break;
		default:
			// The game ends
			if (pressed & EL_INP_A) return true;
			break;
	}
	return false;
}

static void draw_options(void) {
	GameText_t* t = _option_text[draw_set()];
	char s[GAME_TEXT_MAX + 1];
	const int32_t top = CENTRE_Y - 50;
	for (uint32_t i = 0; i < OPTIONS; ++i) {
		switch (i) {
			case OPTION_STYLE:
				snprintf(s, sizeof(s), "STYLE    < %s >", style_names[draw_style]);
				break;
			case OPTION_DITHER:
				snprintf(s, sizeof(s), "DITHERED < %s >", draw_dither ? "ON" : "OFF");
				break;
			case OPTION_SCREEN:
				snprintf(s, sizeof(s), "SCREEN   < %s >", draw_aspect == ASPECT_16_9 ? "16:9" : "4:3");
				break;
			default:
				snprintf(s, sizeof(s), "END GAME   (FIRE)");
				break;
		}
		game_text_set(&t[i], s, 1);
		game_spr_text_centre(&t[i], top + 24 * (int32_t)i, game_pallet(i == _option ? GAME_YELLOW : GAME_WHITE));
		draw_count_sprite();
	}
	game_text_set(&t[OPTIONS], "START TO GO ON", 1);
	game_spr_text_centre(&t[OPTIONS], top + 24 * OPTIONS + 16, game_pallet(GAME_CYAN));
	draw_count_sprite();
}

// ----------------------------------------------------------------------------
// Docked, launching and docking, and the end

#define STATUS_X 24
#define STATUS_VALUE_X (STATUS_X + 20 * 8)
#define STATUS_TOP (VIEW_TOP + 14)
#define STATUS_LINE 16
#define TUNNEL_RINGS 8
#define TUNNEL_SIDES 32
// The rings grow from this, in pixels, to this many times as big, past the corners of the view
#define TUNNEL_SMALL 6.0f
#define TUNNEL_GROWTH 110.0f

// Whether any of the other ships is on the scanner
static bool ships_near(void) {
	for (uint32_t i = 0; i < SHIPS; ++i) {
		if (ships[i].alive && vec_length(vec_sub(ships[i].fr.pos, _me.pos)) < SCANNER_RANGE) return true;
	}
	return false;
}

// Our rating from how many ships we have destroyed, as in the original
static const char* rating(uint32_t kills_made) {
	static const uint32_t kills[] = { 8, 16, 32, 64, 128, 512, 2560, 6400 };
	static const char* const names[] = {
		"Harmless", "Mostly Harmless", "Poor", "Average", "Above Average", "Competent", "Dangerous", "Deadly", "---- E L I T E ----"
	};
	uint32_t i = 0;
	while (i < sizeof(kills) / sizeof(kills[0]) && kills_made >= kills[i]) ++i;
	return names[i];
}

// Our status, as in the original game, with a Cobra like ours turning beside it
static void draw_status(void) {
	const char* const names[] = {
		"Present System", "Hyperspace System", "Condition", "Fuel", "Cash", "Legal Status", "Rating"
	};
	char fuel[24];
	snprintf(fuel, sizeof(fuel), "%u.%u Light Years", (unsigned)(cmdr.fuel / 10), (unsigned)(cmdr.fuel % 10));
	char cash[24];
	snprintf(cash, sizeof(cash), "%u.%u Cr", (unsigned)(cmdr.cash / 10), (unsigned)(cmdr.cash % 10));
	const char* values[] = {
		chart_system(chart_home())->name, chart_system(chart_selected())->name,
		_state == STATE_DOCKED ? "Docked" : ships_hostile_near(&_me, SCANNER_RANGE) ? "Red" : ships_near() ? "Yellow" : "Green",
		fuel, cash, _offender ? "Offender" : "Clean", rating(_kills)
	};
	int32_t y = STATUS_TOP;
	for (uint32_t i = 0; i < sizeof(names) / sizeof(names[0]); ++i) {
		screen_text(STATUS_X, y, names[i], GAME_CYAN, false);
		screen_text(STATUS_VALUE_X, y, values[i], GAME_WHITE, false);
		y += STATUS_LINE;
	}
	y += STATUS_LINE;
	screen_text(STATUS_X, y, "EQUIPMENT:", GAME_CYAN, false);
	const char* fitted[4];
	const uint32_t n = trade_equipment_names(fitted, 4);
	for (uint32_t i = 0; i < n; ++i) screen_text(STATUS_X + 16, y + STATUS_LINE * (int32_t)(i + 1), fitted[i], GAME_WHITE, false);
	if (_state == STATE_DOCKED && (_frame / 30) & 1) screen_text_centre(VIEW_BOTTOM - 24, "FIRE TO LAUNCH", GAME_CYAN, true);

	const float t = (float)_frame / 60.0f;
	float m[3][3];
	make_matrix(m, t * 0.5f, 0.45f, 0.0f);
	const float z = 650.0f;
	const Vec3_t pos = { 150.0f * z / (FOCAL * draw_aspect), -30.0f * z / FOCAL, z };
	draw_object(&model_cobra, pos, m, model_cobra.rgb, draw_style);
}

// The colours of the rings in hyperspace
static const uint8_t _hyper_rgb[6][3] = {
	{ 255, 73, 0 }, { 255, 219, 0 }, { 0, 255, 0 }, { 0, 219, 255 }, { 73, 73, 255 }, { 255, 0, 255 }
};

// Rings flying out from the middle, going through the station's tunnel, or
// in colours through hyperspace
static void draw_tunnel(bool hyper) {
	const float t = (float)_state_frames / (float)TUNNEL_FRAMES;
	for (uint32_t i = 0; i < TUNNEL_RINGS; ++i) {
		const float phase = fmodf(t * 3.0f + (float)i / TUNNEL_RINGS, 1.0f);
		const float r = TUNNEL_SMALL * powf(TUNNEL_GROWTH, phase);
		if (!object_begin()) break;
		Vec2_t p[TUNNEL_SIDES];
		circle_points(p, TUNNEL_SIDES, CENTRE_X, CENTRE_Y, r);
		const float level = 0.3f + 0.7f * phase;
		static const uint8_t white[3] = { 255, 255, 255 };
		const hstx_dvi_pixel_t c = shade(hyper ? _hyper_rgb[(i + _state_frames / 8) % 6] : white, level);
		for (uint32_t j = 0; j < TUNNEL_SIDES; ++j) add_line(p[j], p[(j + 1) % TUNNEL_SIDES], c);
		object_end();
	}
}

// Moves on from launching or docking once through the tunnel, and returns
// true when the end of the game has been shown
static bool next_state(void) {
	++_state_frames;
	switch (_state) {
		case STATE_LAUNCHING:
			if (_state_frames >= TUNNEL_FRAMES) set_state(STATE_FLYING);
			break;
		case STATE_DOCKING:
			if (_state_frames >= TUNNEL_FRAMES) {
				_speed = 0.0f;
				_roll = 0.0f;
				_pitch = 0.0f;
				_view = VIEW_FRONT;
				refit();
				set_state(STATE_DOCKED);
				save();
			}
			break;
		case STATE_HYPERSPACE:
			if (_state_frames >= HYPER_FRAMES) arrive();
			break;
		case STATE_DEAD:
			return _state_frames >= DEAD_FRAMES;
		default:
			break;
	}
	return false;
}

// ----------------------------------------------------------------------------

// Docked where a commander was saved
static void load(const Save_t* s) {
	cmdr = s->cmdr;
	_kills = s->kills;
	_offender = s->offender != 0;
	chart_set_galaxy(s->galaxy);
	enter_system(s->home);
	trade_arrive(chart_system(s->home));
	chart_set_fuel(cmdr.fuel);
	frame_set(&_me, _station.pos, _station.f, _station.u);
}

// The list of commanders, centred: the slot, where, cash and rating
#define SLOTS_X ((GAME_SCREEN_W - 49 * 8) / 2)

// When there is somewhere to save, picks which commander to play: fire plays
// the one in a slot, or a new one if it is empty, and select starts a new one
static void choose_commander(void) {
	_slot = -1;
	if (!save_ready()) return;
	static Save_t saves[SAVE_SLOTS];
	bool used[SAVE_SLOTS];
	for (uint32_t i = 0; i < SAVE_SLOTS; ++i) used[i] = save_read(i, &saves[i]);
	uint32_t pick = 0;
	for (;;) {
		const uint8_t pressed = game_input_pressed();
		if (pressed & EL_INP_DOWN) pick = (pick + 1) % SAVE_SLOTS;
		if (pressed & EL_INP_UP) pick = (pick + SAVE_SLOTS - 1) % SAVE_SLOTS;
		if (pressed & (EL_INP_A | EL_INP_SELECT)) {
			_slot = (int32_t)pick;
			if ((pressed & EL_INP_A) && used[pick]) load(&saves[pick]);
			return;
		}
		screen_begin();
		draw_dashboard();
		char line[GAME_TEXT_MAX + 1];
		const int32_t top = VIEW_TOP + 40;
		for (uint32_t i = 0; i < SAVE_SLOTS; ++i) {
			const uint32_t c = i == pick ? GAME_YELLOW : GAME_WHITE;
			const int32_t y = top + 24 * (int32_t)i;
			if (used[i]) {
				const Save_t* sv = &saves[i];
				// Where it was saved, if in this galaxy
				const char* at = sv->galaxy == chart_galaxy() ? chart_system(sv->home)->name : "";
				snprintf(line, sizeof(line), "%u  %-9s %6u.%u Cr", (unsigned)(i + 1), at,
					(unsigned)(sv->cmdr.cash / 10), (unsigned)(sv->cmdr.cash % 10));
				screen_text(SLOTS_X, y, line, c, false);
				screen_text(SLOTS_X + 30 * 8, y, rating(sv->kills), c, false);
			}
			else {
				snprintf(line, sizeof(line), "%u  EMPTY", (unsigned)(i + 1));
				screen_text(SLOTS_X, y, line, c, false);
			}
		}
		screen_text_centre(VIEW_BOTTOM - 40, "FIRE PLAYS", GAME_CYAN, false);
		screen_text_centre(VIEW_BOTTOM - 24, "SELECT FOR A NEW COMMANDER", GAME_CYAN, false);
		screen_end("COMMANDERS", "COMMANDERS");
		++_frame;
	}
}

void game_play(void) {
	start();
	choose_commander();
	for (;;) {
		const uint8_t pressed = game_input_pressed();
		const uint8_t held = game_input_get();
		if (_options) {
			if (options(pressed)) return;
		}
		else {
			ShipsEvents_t ev = { 0 };
			if (pressed & EL_INP_START) _options = true;
			const bool can_look = _state == STATE_DOCKED || _state == STATE_FLYING;
			if (pressed & EL_INP_SELECT) {
				// With B held, the ECM blows up the missiles flying
				if (held & EL_INP_B) {
					if (_state == STATE_FLYING) {
						if (cmdr.ecm) {
							ships_ecm();
							message("E.C.M.");
						}
						else {
							message("NO E.C.M.");
						}
					}
				}
				else if (can_look) {
					_view = (View_e)((_view + 1) % VIEWS);
					// The market and equipment only when docked
					if (_state != STATE_DOCKED && (_view == VIEW_MARKET || _view == VIEW_EQUIPMENT)) _view = VIEW_LOCAL;
				}
			}
			switch (_state) {
				case STATE_DOCKED: {
					const char* m = NULL;
					if (_view == VIEW_MARKET) m = trade_market_input(pressed, held);
					else if (_view == VIEW_EQUIPMENT) m = trade_equipment_input(pressed, chart_system(chart_home()));
					else move_cursor(held);
					if (m) message(m);
					chart_set_fuel(cmdr.fuel);
					// Only from the status, so not by mistake on the charts
					if ((pressed & EL_INP_A) && _view == VIEW_FRONT) launch();
					break;
				}
				case STATE_FLYING:
					// Fire on the charts or data jumps to the system the cursor is on
					if ((pressed & EL_INP_A) && _view != VIEW_FRONT) start_hyperspace();
					if (_hyper_frames && --_hyper_frames == 0) {
						// Still clear of everything at the end of the countdown
						if (mass_locked()) message("MASS LOCKED");
						else set_state(STATE_HYPERSPACE);
					}
					fire(held, pressed, &ev);
					fly(held);
					break;
				case STATE_DEAD:
					_speed = 0.0f;
					dust_move(0.0f, 0.0f, 0.0f);
					break;
				default:
					dust_move(0.0f, 0.0f, 20.0f);
					break;
			}
			move_world(&ev);
			if (_state == STATE_FLYING) take(&ev);
			if (next_state()) return;
		}

		screen_begin();
		draw_dashboard();
		// The charts and data are always wireframes, to look like data
		const Style_e chart_style = draw_style == STYLE_WIRE ? STYLE_WIRE : STYLE_SOLID;
		char title[GAME_TEXT_MAX + 1];
		const char* label = _view_labels[_view];
		switch (_view) {
			case VIEW_FRONT:
				if (_state == STATE_DOCKED) {
					snprintf(title, sizeof(title), "COMMANDER JAMESON");
					label = "STATUS";
					draw_status();
				}
				else if (_state == STATE_LAUNCHING || _state == STATE_DOCKING || _state == STATE_HYPERSPACE) {
					snprintf(title, sizeof(title), _state == STATE_LAUNCHING ? "LAUNCHING" :
						_state == STATE_DOCKING ? "DOCKING" : "HYPERSPACE");
					label = "TUNNEL";
					draw_tunnel(_state == STATE_HYPERSPACE);
				}
				else {
					snprintf(title, sizeof(title), "FRONT VIEW");
					draw_front();
					if (_state == STATE_DEAD) {
						label = "GAME OVER";
						screen_text_centre(CENTRE_Y - 8, "GAME OVER", GAME_RED, true);
					}
				}
				break;
			case VIEW_MARKET:
				snprintf(title, sizeof(title), "%s MARKET PRICES", chart_system(chart_home())->name);
				trade_draw_market();
				break;
			case VIEW_EQUIPMENT:
				snprintf(title, sizeof(title), "EQUIP SHIP");
				trade_draw_equipment(chart_system(chart_home()));
				break;
			case VIEW_LOCAL:
				snprintf(title, sizeof(title), "SHORT RANGE CHART");
				chart_draw_local(chart_style);
				break;
			case VIEW_GALAXY:
				snprintf(title, sizeof(title), "GALACTIC CHART %u", (unsigned)(chart_galaxy() + 1));
				chart_draw_galaxy();
				break;
			default:
				snprintf(title, sizeof(title), "DATA ON %s", chart_draw_data(chart_selected(), chart_style, _frame));
				break;
		}
		// The countdown to hyperspace, and any message
		if (_hyper_frames) {
			char count[12];
			snprintf(count, sizeof(count), "%u", (unsigned)((_hyper_frames + 59) / 60));
			screen_text(16, VIEW_TOP + 6, count, GAME_YELLOW, true);
		}
		if (_message_frames) {
			--_message_frames;
			screen_text_centre(VIEW_BOTTOM - 32, _message, GAME_YELLOW, true);
		}
		// Text draws straight into the row, so the options go last, over everything else
		if (_options) {
			label = "OPTIONS";
			draw_options();
		}
		screen_end(title, label);
		++_frame;
	}
}
