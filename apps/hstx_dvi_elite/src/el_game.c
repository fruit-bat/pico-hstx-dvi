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
#include "el_options.h"
#include "el_screen.h"
#include "el_save.h"
#include "el_ships.h"
#include "el_space.h"
#include "el_trade.h"
#include "game_pallet.h"
#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// The system: a planet as big as its data says, in units for each km, the
// station going round it with its slot facing it, this many planet radii
// from its middle, and the sun far beyond. Where the sun and the station are
// comes from the system's numbers, so it is the same each time we visit.
#define PLANET_SCALE 3.0f
#define STATION_ORBIT 2.2f
#define STATION_SPIN 0.005f
#define SUN_RADIUS 30000.0f
#define SUN_DISTANCE 330000.0f
// The sun is drawn while its middle is at least this far ahead, for its
// distance, as further round its middle does not project well
#define SUN_NEAR_Z 0.2f
// Near the sun the cabin heats up: from this many sun radii from its middle,
// hotter the closer, from cool to as hot as it gets at the surface. It moves
// towards that by this much a frame. Too hot, it drains our energy.
#define HEAT_RANGE 4.0f
#define CABIN_COOL 0.1f
#define CABIN_CHANGE 0.004f
#define CABIN_TOO_HOT 0.85f
#define HEAT_DAMAGE 0.004f
// Hot enough for the fuel scoops, which take a tenth of a light year this often
#define SCOOP_HEAT 0.35f
#define SCOOP_FRAMES 15
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
// With an escape pod, when our energy runs out we leave the ship, looking
// back at it drifting and tumbling as we go, until it blows up, and then
// come in to the station, with a new ship, the same but for the cargo and the
// pod. How fast the pod goes, how much of our speed the ship keeps, how it
// tumbles, when it blows up, and when we are in the tunnel.
#define POD_SPEED 22.0f
#define HULK_DRIFT 0.3f
#define HULK_TUMBLE 0.02f
#define HULK_BLOWS 120
#define POD_FRAMES 210
// One jump in this many goes wrong, leaving us in witchspace, halfway, with
// the bodies of the system we left this far away, out of sight
#define WITCH_CHANCE 40
#define WITCH_FAR 5.0e7f
// The energy bomb goes off with three presses of its button within this
// many frames, destroying every ship within this range, with a flash that
// fades over this many frames
#define BOMB_PRESSES 3
#define BOMB_WINDOW 30
#define BOMB_RANGE SCANNER_RANGE
#define BOMB_FLASH 20
// The keys of keyboard, which do nothing without one
#ifdef GAME_KEYBOARD
#define KEY_HELD(k) game_key_held(k)
#define KEY_PRESSED(k) game_key_pressed(k)
#else
#define KEY_HELD(k) false
#define KEY_PRESSED(k) false
#endif
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
// The overdrive: holding B and up at full speed for this many frames, we go
// up to this fast, getting there by this much a frame, and burn a tenth of a
// light year of fuel this often, until slowed, out of fuel, or mass locked
#define OVERDRIVE_WAIT 30
#define OVERDRIVE_SPEED 360.0f
#define OVERDRIVE_RAMP (1.0f / 60.0f)
#define OVERDRIVE_FUEL_FRAMES 60
// It is mass locked as hyperspace is, but nearer the planet, in its radii
// above the surface, as the station is not far beyond hyperspace's limit;
// still some frames away at full overdrive
#define OVERDRIVE_LOCK_PLANET 0.3f
// The dust streams by no faster than this, or every speck would be gone in a frame
#define DUST_SPEED_MAX (3.0f * MAX_SPEED)
// The docking computer flies to a point this far out in front of the slot,
// round the side if we are behind the station, then in along the line to the
// slot, within this of it, as how far off it for how far out, at this speed.
// It turns at this rate, slows down when not facing the way it is going, and
// rolls to keep the slot level all the way.
#define DOCK_APPROACH 2500.0f
#define DOCK_ARRIVED 300.0f
#define DOCK_LINE 0.3f
#define DOCK_SPEED 10.0f
#define DOCK_TURN 0.02f
#define DOCK_SLOW 0.15f
// Slowing on the way to the point in front, to be at this speed this many frames from it
#define DOCK_BRAKE 50.0f
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
// How far off the middle of the view a targeting computer follows a ship, as
// across for each ahead, about 7 degrees
#define TRACK_CONE 0.12f
// How long the name of the ship it follows stays up once it lets go, so it can be read
#define TRACK_NAME_FRAMES 120
// How far below the middle of the sights the name goes, clear of them, or
// above if it would go off the bottom of the view, and how tall it is
#define TRACK_NAME_GAP 24
#define TRACK_NAME_H 16
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

// The views out, the way each laser points, as Mount_e; the front view is
// our status when docked, and the others are not seen then. The market and
// equipment are only seen docked.
typedef enum {
	VIEW_FRONT = 0,
	VIEW_REAR,
	VIEW_LEFT,
	VIEW_RIGHT,
	VIEW_MARKET,
	VIEW_EQUIPMENT,
	VIEW_INVENTORY,
	VIEW_LOCAL,
	VIEW_GALAXY,
	VIEW_DATA,
	VIEWS
} View_e;

static const char* const _view_labels[VIEWS] = {
	"FRONT VIEW", "REAR VIEW", "LEFT VIEW", "RIGHT VIEW", "MARKET PRICES", "EQUIP SHIP", "INVENTORY", "SHORT RANGE CHART", "GALACTIC CHART", "DATA ON SYSTEM"
};

typedef enum {
	STATE_DOCKED = 0,
	STATE_LAUNCHING,
	STATE_FLYING,
	STATE_DOCKING,
	STATE_HYPERSPACE,
	STATE_ESCAPE,
	STATE_DEAD
} State_e;

static Frame_t _me;
// Where we look from, the way the view is
static Frame_t _eye;
static float _speed, _roll, _pitch;
// Whether the overdrive is on, how far into it we are, from 0 to 1, how long
// we have held on at full speed, and frames towards burning more fuel
static bool _overdrive;
static float _over;
static uint32_t _over_hold, _over_fuel;
// Whether the overdrive was asked for at once, from the keyboard
static bool _over_ask;
// Whether the docking computer is flying us, and whether it is on the run in
static bool _docking, _dock_final;
// Our ship, left behind when we escape in the pod, and how it is going
static Frame_t _hulk;
static Vec3_t _hulk_drift;
static bool _hulk_on;
// Whether we are in witchspace, and the system we were going to
static bool _witch;
static uint32_t _witch_to;
// The presses of the bomb's button so far, how long is left for the rest, and the flash
static uint32_t _bomb_presses, _bomb_frames, _bomb_flash;
static Frame_t _planet, _station;
static float _planet_radius;
// The station's model, a Coriolis or a dodecahedron, and how far its front
// face, with the slot in the middle, is from its middle
static const WireModel_t* _station_model = &model_coriolis;
static float _station_face;
static Vec3_t _sun_pos;
// Frames left counting down to hyperspace, and where to
static uint32_t _hyper_frames;
static uint32_t _hyper_to;
// Whether the jump counting down is to the next galaxy
static bool _hyper_galactic;
static const char* _message;
static uint32_t _message_frames;
// Our ship and how we have done
static float _fore, _aft, _energy;
static float _laser_temp;
static float _cabin_temp;
static uint32_t _scoop_frames;
static bool _overheated;
static uint32_t _laser_wait, _laser_beam;
// The ship the targeting computer follows, or -1, and how far off the
// middle of the view it is, where the sights and the laser go
static int32_t _tracked = -1;
static Vec2_t _sights_off;
// The name of the ship it follows, or last followed, its bounty, and how long
// they show for
static const char* _tracked_name;
static uint32_t _tracked_bounty;
static uint32_t _tracked_name_frames;
static uint32_t _kills;
// The slot we save to, or -1 if there is nowhere to save
static int32_t _slot = -1;
static char _name[SAVE_NAME_MAX] = "JAMESON";
static bool _offender;
static View_e _view;
static State_e _state;
// Frames into the state, for the tunnel and the end
static uint32_t _state_frames;
static uint32_t _frame;

// Moves a value towards a target by at most step
static float towards(float v, float target, float step) {
	if (v < target) return fminf(v + step, target);
	return fmaxf(v - step, target);
}

// What the other ships need to know
static ShipsWorld_t world(void) {
	const GalaxySystem_t* sys = chart_system(chart_home());
	return (ShipsWorld_t){ &_me, _station.pos, sys->government, sys->economy, sys->population, _offender, cmdr.scoops, _witch };
}

// Repaired, as when docked
static void refit(void) {
	_fore = _aft = _energy = 1.0f;
	_laser_temp = 0.0f;
	_overheated = false;
	_cabin_temp = CABIN_COOL;
	_scoop_frames = 0;
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
// How far the face of a model pointing along n is from its middle
static float model_face(const WireModel_t* m, Vec3_t n) {
	float d = 0.0f;
	for (uint32_t i = 0; i < m->nf; ++i) {
		const WireFace_t* f = &m->f[i];
		if (f->nx * n.x + f->ny * n.y + f->nz * n.z < 0.99f) continue;
		const WireVertex_t* v = &m->v[f->v[0]];
		d = f->nx * v->x + f->ny * v->y + f->nz * v->z;
	}
	return d;
}

static void enter_system(uint32_t i) {
	chart_set_home(i);
	_witch = false;
	dust_strange = false;
	const GalaxySystem_t* sys = chart_system(i);
	_planet_radius = PLANET_SCALE * (float)sys->radius;
	frame_set(&_planet, (Vec3_t){ 0.0f, 0.0f, 0.0f }, (Vec3_t){ 0.0f, 0.0f, 1.0f },
		(Vec3_t){ 0.3f * system_number(sys, 0), 1.0f, 0.0f });
	float round = 1.2f * system_number(sys, 1);
	if (fabsf(round) < 0.3f) round = round < 0.0f ? -0.3f : 0.3f;
	_sun_pos = vec_scale(direction(round, 0.4f * system_number(sys, 2)), SUN_DISTANCE);
	const Vec3_t sp = vec_scale(direction(PI_F + 0.8f * system_number(sys, 3), 0.5f * system_number(sys, 4)),
		STATION_ORBIT * _planet_radius);
	_station_model = chart_station_model(sys);
	_station_face = model_face(_station_model, (Vec3_t){ 0.0f, 0.0f, 1.0f });
	// Its slot faces the planet
	frame_set(&_station, sp, vec_unit(vec_sub(_planet.pos, sp)), (Vec3_t){ 0.0f, 1.0f, 0.0f });
	const ShipsWorld_t w = world();
	ships_enter(&w);
}

static void start(void) {
	strcpy(_name, "JAMESON");
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
	_tracked = -1;
	_tracked_name_frames = 0;
	_hyper_frames = 0;
	_message_frames = 0;
	_view = VIEW_FRONT;
	_state = STATE_DOCKED;
	_state_frames = 0;
	options_close();
	_frame = 0;
	dust_init();
}

// Back to full speed at once
static void overdrive_off(void) {
	_overdrive = false;
	_over_ask = false;
	_over = 0.0f;
	_over_hold = 0;
}

static void set_state(State_e state) {
	_state = state;
	_state_frames = 0;
	overdrive_off();
	_docking = false;
	_bomb_presses = _bomb_frames = 0;
	// Docking or dying stops a countdown to hyperspace
	if (state == STATE_DOCKING || state == STATE_DEAD) _hyper_frames = 0;
	// The tunnels are seen ahead
	if (state == STATE_LAUNCHING || state == STATE_DOCKING || state == STATE_HYPERSPACE) _view = VIEW_FRONT;
}

static void message(const char* m) {
	_message = m;
	_message_frames = MESSAGE_FRAMES;
}

// Whether we are too close to something to go into hyperspace, or the
// overdrive: within planet of its radii above the planet's surface, or near
// the sun, the station or another ship
static bool mass_locked(float planet) {
	if (vec_length(vec_sub(_me.pos, _planet.pos)) < (1.0f + planet) * _planet_radius) return true;
	if (vec_length(vec_sub(_me.pos, _sun_pos)) < LOCK_SUN * SUN_RADIUS) return true;
	if (vec_length(vec_sub(_me.pos, _station.pos)) < LOCK_STATION) return true;
	for (uint32_t i = 0; i < SHIPS; ++i) {
		if (ships[i].alive && vec_length(vec_sub(_me.pos, ships[i].fr.pos)) < LOCK_SHIP) return true;
	}
	return false;
}

// Starts the countdown to the system the cursor is on, if we can reach it
// To the system on the charts, or with the galactic hyperdrive, from the
// galactic chart, to the next galaxy
static void start_hyperspace(bool galactic) {
	if (_hyper_frames) return;
	if (galactic && !cmdr.galactic) {
		message("NO GALACTIC HYPERDRIVE");
		return;
	}
	if (mass_locked(LOCK_PLANET)) {
		message("MASS LOCKED");
		return;
	}
	_hyper_galactic = galactic;
	if (galactic) {
		_hyper_frames = HYPER_COUNT * 60;
		message("GALACTIC HYPERSPACE");
		return;
	}
	const uint32_t to = chart_selected();
	if (to == chart_home() && !_witch) {
		message("WE ARE HERE");
		return;
	}
	if (chart_distance(to) > cmdr.fuel) {
		message("OUT OF RANGE");
		return;
	}
	_hyper_to = to;
	_hyper_frames = HYPER_COUNT * 60;
}

// Out of hyperspace in the new system, facing its planet
// Witchspace: halfway, with the fuel spent, and Thargoids waiting
static void enter_witchspace(void) {
	_witch_to = _hyper_to;
	chart_set_away(chart_home(), _hyper_to);
	_witch = true;
	dust_strange = true;
	// The bodies of the system we left, far out of sight; the sun still lights us
	_planet.pos = (Vec3_t){ 0.0f, 0.0f, WITCH_FAR };
	_station.pos = (Vec3_t){ 0.0f, WITCH_FAR, 0.0f };
	_sun_pos = (Vec3_t){ WITCH_FAR, WITCH_FAR, -WITCH_FAR };
	frame_set(&_me, (Vec3_t){ 0.0f, 0.0f, 0.0f }, (Vec3_t){ 0.0f, 0.0f, 1.0f }, (Vec3_t){ 0.0f, 1.0f, 0.0f });
	const ShipsWorld_t w = world();
	ships_enter(&w);
	_speed = MAX_SPEED * 0.25f;
	_roll = 0.0f;
	_pitch = 0.0f;
	_view = VIEW_FRONT;
	dust_init();
	set_state(STATE_FLYING);
	message("WITCHSPACE");
}

// Into the next galaxy, after the last the first, at the system nearest
// where we were on the chart, with the hyperdrive used up
static void galactic_jump(void) {
	const GalaxySystem_t* h = chart_system(chart_home());
	const float x = h->x, y = h->y;
	chart_set_galaxy((chart_galaxy() + 1) % GALAXIES);
	chart_cursor_set(x, y);
	_hyper_to = chart_selected();
	cmdr.galactic = false;
}

static void arrive(void) {
	if (_hyper_galactic) {
		galactic_jump();
	}
	else {
		cmdr.fuel -= chart_distance(_hyper_to);
		chart_set_fuel(cmdr.fuel);
	}
	if (!_witch && !_hyper_galactic && (options_witch() || rand() % WITCH_CHANCE == 0)) {
		enter_witchspace();
		return;
	}
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
	memcpy(s.name, _name, sizeof(_name));
	if (!save_write((uint32_t)_slot, &s)) message("SAVE FAILED");
}

// Out through the slot, facing the way it does, which is towards the planet,
// saving first what we bought
static void launch(void) {
	save();
	frame_set(&_me, vec_add(_station.pos, vec_scale(_station.f, _station_model->radius * LAUNCH_DISTANCE)), _station.f, _station.u);
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

// For trying the docking computer on the PC: fits one, and puts us behind
// the station, facing away from it, rolled
void game_debug_behind(void) {
	cmdr.docking = true;
	const Vec3_t at = vec_add(_station.pos, vec_add(vec_scale(_station.f, -4000.0f), vec_scale(_station.u, 600.0f)));
	frame_set(&_me, at, vec_scale(_station.f, -1.0f), _station.u);
	frame_roll(&_me, 1.0f);
	frame_tidy(&_me);
	_speed = MAX_SPEED * 0.5f;
	_view = VIEW_FRONT;
	set_state(STATE_FLYING);
}

// For trying the escape pod on the PC: fits one, with some food in the hold,
// and runs our energy out
void game_debug_pod(void) {
	cmdr.pod = true;
	cmdr.cargo[0] = 5;
	_fore = _aft = 0.0f;
	_energy = 0.0f;
}

// For trying the dodecahedron on the PC: moves us to the nearest system
// with one, and behind its station with a docking computer
void game_debug_dodo(void) {
	for (uint32_t k = 1; k < GALAXY_SYSTEMS; ++k) {
		const uint32_t i = chart_nearest(k);
		if (chart_system(i)->tech < DODO_TECH) continue;
		enter_system(i);
		trade_arrive(chart_system(i));
		break;
	}
	game_debug_behind();
}

// Whether a point is inside the station, or close enough to touch it,
// whichever shape it is: behind every face, with the margin
static bool in_station(Vec3_t local) {
	const WireModel_t* m = _station_model;
	for (uint32_t i = 0; i < m->nf; ++i) {
		const WireFace_t* f = &m->f[i];
		const WireVertex_t* v = &m->v[f->v[0]];
		const float d = f->nx * v->x + f->ny * v->y + f->nz * v->z;
		if (f->nx * local.x + f->ny * local.y + f->nz * local.z > d + STATION_MARGIN) return false;
	}
	return true;
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

// Whether we look out of the ship, rather than at a chart or the market
static bool looking_out(void) {
	return _view <= VIEW_RIGHT;
}

// The view after this one, as select goes round them
static View_e next_view(void) {
	View_e v = _view;
	for (;;) {
		v = (View_e)((v + 1) % VIEWS);
		if (_state == STATE_DOCKED && v >= VIEW_REAR && v <= VIEW_RIGHT) continue;
		if (_state != STATE_DOCKED && (v == VIEW_MARKET || v == VIEW_EQUIPMENT)) continue;
		return v;
	}
}

// The view turned round the ship, clockwise for step 1 and the other way
// for -1, as the shoulders turn it; from a chart, back to the front
static View_e look_round(int32_t step) {
	static const View_e round[4] = { VIEW_FRONT, VIEW_RIGHT, VIEW_REAR, VIEW_LEFT };
	if (!looking_out()) return VIEW_FRONT;
	uint32_t i = 0;
	while (round[i] != _view) ++i;
	return round[(uint32_t)((int32_t)i + 4 + step) % 4];
}

// Where we look from: our frame turned the way the view is
static void look(void) {
	_eye = _me;
	switch (_view) {
		case VIEW_REAR:
			_eye.r = vec_scale(_me.r, -1.0f);
			_eye.f = vec_scale(_me.f, -1.0f);
			break;
		case VIEW_LEFT:
			_eye.r = _me.f;
			_eye.f = vec_scale(_me.r, -1.0f);
			break;
		case VIEW_RIGHT:
			_eye.r = vec_scale(_me.f, -1.0f);
			_eye.f = _me.r;
			break;
		default:
			break;
	}
}

// How the dust moves in the view: our roll and pitch, and going forward,
// turned the way the view is
static void move_dust(float roll, float pitch, float speed) {
	Vec3_t turn = { pitch, 0.0f, roll };
	Vec3_t move = { 0.0f, 0.0f, speed };
	switch (_view) {
		case VIEW_REAR:
			turn = (Vec3_t){ -pitch, 0.0f, -roll };
			move = (Vec3_t){ 0.0f, 0.0f, -speed };
			break;
		case VIEW_LEFT:
			turn = (Vec3_t){ roll, 0.0f, -pitch };
			move = (Vec3_t){ speed, 0.0f, 0.0f };
			break;
		case VIEW_RIGHT:
			turn = (Vec3_t){ -roll, 0.0f, pitch };
			move = (Vec3_t){ -speed, 0.0f, 0.0f };
			break;
		default:
			break;
	}
	dust_move(turn, move);
}

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

// Into the overdrive, having held on at full speed, if we can, and out of it
// when near something or out of fuel, burning fuel while in it
static void overdrive(void) {
	if (!_overdrive && (_over_hold >= OVERDRIVE_WAIT || _over_ask)) {
		_over_hold = 0;
		_over_ask = false;
		if (mass_locked(OVERDRIVE_LOCK_PLANET)) message("MASS LOCKED");
		else if (!cmdr.fuel) message("NO FUEL");
		else {
			_overdrive = true;
			_over_fuel = 0;
		}
	}
	if (!_overdrive) return;
	if (mass_locked(OVERDRIVE_LOCK_PLANET)) {
		overdrive_off();
		message("MASS LOCKED");
		return;
	}
	if (++_over_fuel >= OVERDRIVE_FUEL_FRAMES) {
		_over_fuel = 0;
		if (!cmdr.fuel) {
			overdrive_off();
			message("NO FUEL");
			return;
		}
		--cmdr.fuel;
		chart_set_fuel(cmdr.fuel);
	}
	_over = fminf(_over + OVERDRIVE_RAMP, 1.0f);
}

// The docking computer, on or off, if we have one
static void docking_computer(bool on) {
	if (on && !cmdr.docking) {
		message("NO DOCKING COMPUTER");
		return;
	}
	if (on && _witch) {
		message("NO STATION");
		return;
	}
	if (on == _docking || _state != STATE_FLYING) return;
	_docking = on;
	_dock_final = false;
	if (on) overdrive_off();
	message(on ? "DOCKING COMPUTER ON" : "DOCKING COMPUTER OFF");
}

// The docking computer flying us: turning towards where it is going, rolling
// so the slot is level, and with the speed for it
static void auto_dock(void) {
	const Vec3_t d = vec_sub(_me.pos, _station.pos);
	const float out = vec_dot(d, _station.f);
	const Vec3_t across = vec_sub(d, vec_scale(_station.f, out));
	const float off = vec_length(across);
	const Vec3_t approach = vec_add(_station.pos, vec_scale(_station.f, DOCK_APPROACH));
	// On the line in front of the slot it goes in; it leaves the line if blown off it
	if (out > _station_face && off < out * DOCK_LINE) {
		if (vec_length(vec_sub(_me.pos, approach)) < DOCK_ARRIVED || out < DOCK_APPROACH) _dock_final = true;
	}
	else {
		_dock_final = false;
	}
	Vec3_t target = approach;
	float speed = MAX_SPEED;
	if (_dock_final) {
		target = _station.pos;
		speed = DOCK_SPEED;
	}
	else if (out < 2.0f * _station_face) {
		// Round the side first, clear of the station
		const Vec3_t side = off > 1.0f ? vec_scale(across, 1.0f / off) : _station.u;
		target = vec_add(approach, vec_scale(side, DOCK_APPROACH));
	}
	else {
		// Slowing to arrive
		speed = fminf(speed, fmaxf(DOCK_SPEED, vec_length(vec_sub(approach, _me.pos)) / DOCK_BRAKE));
	}

	// Turning: straight towards it, a little up when it is right behind, to start the turn
	Vec3_t dir = vec_unit(vec_sub(target, _me.pos));
	const float facing = vec_dot(dir, _me.f);
	if (facing < -0.99f) dir = vec_unit(vec_add(dir, vec_scale(_me.u, 0.2f)));
	const Vec3_t was = _me.f;
	const Vec3_t turn = vec_sub(dir, _me.f);
	const float l = vec_length(turn);
	_me.f = l > DOCK_TURN ? vec_add(_me.f, vec_scale(turn, DOCK_TURN / l)) : dir;
	frame_tidy(&_me);
	_pitch = vec_dot(vec_sub(_me.f, was), _me.u);

	// Rolling, the shorter way, until the station's right is across us
	const float p = vec_dot(_station.r, _me.r), q = vec_dot(_station.r, _me.u);
	float a = atan2f(-q, p);
	if (a > 0.5f * PI_F) a -= PI_F;
	if (a < -0.5f * PI_F) a += PI_F;
	_roll = fminf(fmaxf(a, -MAX_ROLL), MAX_ROLL);
	frame_roll(&_me, _roll);
	frame_tidy(&_me);

	speed *= fminf(fmaxf((facing - 0.5f) * 2.0f, DOCK_SLOW), 1.0f);
	_speed = towards(_speed, speed, SPEED_STEP);
	if (!_message_frames) message("DOCKING COMPUTER ON");
}

static void fly(uint8_t held) {
	if (_docking) {
		auto_dock();
		const float speed = _speed;
		_me.pos = vec_add(_me.pos, vec_scale(_me.f, speed));
		move_dust(_roll, _pitch, speed);
		touch_station();
		if (vec_length(vec_sub(_me.pos, _planet.pos)) < _planet_radius) set_state(STATE_DEAD);
		return;
	}
	const bool b = held & EL_INP_B;
	float roll_in = 0.0f, pitch_in = 0.0f;
	if (looking_out()) {
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
	// Space and ? as in the original, which leave the pitch alone
	const bool faster = (b && looking_out() && (held & EL_INP_UP)) || KEY_HELD(HID_KEY_SPACE);
	const bool slower = (b && looking_out() && (held & EL_INP_DOWN)) || KEY_HELD(HID_KEY_SLASH);
	if (faster) {
		_speed = fminf(_speed + SPEED_STEP, MAX_SPEED);
		_over_hold = _speed >= MAX_SPEED ? _over_hold + 1 : 0;
	}
	else {
		_over_hold = 0;
	}
	if (slower) {
		// Slowing drops out of the overdrive first
		if (_overdrive) overdrive_off();
		else _speed = fmaxf(_speed - SPEED_STEP, 0.0f);
	}
	overdrive();
	const float speed = _speed + (OVERDRIVE_SPEED - MAX_SPEED) * _over;
	_roll = towards(_roll, roll_in * MAX_ROLL, ROLL_STEP);
	_pitch = towards(_pitch, pitch_in * MAX_PITCH, PITCH_STEP);

	frame_roll(&_me, _roll);
	frame_pitch(&_me, _pitch);
	frame_tidy(&_me);
	_me.pos = vec_add(_me.pos, vec_scale(_me.f, speed));
	move_dust(_roll, _pitch, fminf(speed, DUST_SPEED_MAX));

	touch_station();
	if (vec_length(vec_sub(_me.pos, _planet.pos)) < _planet_radius) set_state(STATE_DEAD);
}

// A missile at the ship nearest the middle of the view, or ahead from a chart
static void fire_missile(void) {
	look();
	const int32_t target = ships_nearest_ahead(looking_out() ? &_eye : &_me);
	if (!cmdr.missiles) message("NO MISSILES");
	else if (target < 0) message("NO TARGET");
	else if (ships_fire_missile(&_me, (uint32_t)target)) --cmdr.missiles;
}

// The ECM, which blows up the missiles flying
static void ecm(void) {
	if (cmdr.ecm) {
		ships_ecm();
		message("E.C.M.");
	}
	else {
		message("NO E.C.M.");
	}
}

// The laser the way we look while fire is held, and a missile at the ship
// nearest the middle of the view when fire is pressed with B held
static void fire(uint8_t held, uint8_t pressed, ShipsEvents_t* ev) {
	if (_laser_wait) --_laser_wait;
	if (_laser_beam) --_laser_beam;
	_laser_temp = fmaxf(0.0f, _laser_temp - LASER_COOL);
	if (_overheated && _laser_temp < LASER_COOLED) _overheated = false;
	// Following the same ship while it can
	const int32_t keep = _tracked;
	_tracked = -1;
	if (!looking_out()) return;
	look();
	if (cmdr.targeting && cmdr.laser[_view] != LASER_NONE) _tracked = ships_tracked(&_eye, LASER_RANGE, TRACK_CONE, keep);
	if (_tracked >= 0) {
		_tracked_name = ships[_tracked].cls->name;
		_tracked_bounty = ships_bounty((uint32_t)_tracked);
		_tracked_name_frames = TRACK_NAME_FRAMES;
	}
	if (held & EL_INP_B) {
		if (pressed & EL_INP_A) fire_missile();
		return;
	}
	const Laser_e laser = cmdr.laser[_view];
	if (!(held & EL_INP_A) || _laser_wait || _overheated || laser == LASER_NONE) return;
	const Laser_t* l = &_lasers[laser];
	_laser_wait = l->every;
	_laser_beam = LASER_SEEN;
	_laser_temp += l->heat;
	if (_laser_temp >= 1.0f) {
		_laser_temp = 1.0f;
		_overheated = true;
		message("LASER TEMPERATURE");
	}
	// The ship the targeting computer follows, else the one in the middle
	const int32_t i = _tracked >= 0 ? _tracked : ships_in_sights(&_eye, LASER_RANGE);
	// Doing less the further it goes
	if (i >= 0) ships_hit((uint32_t)i, l->damage * ships_laser_falloff(vec_length(vec_sub(ships[i].fr.pos, _me.pos)), LASER_RANGE), ev);
}

// The energy bomb's button, which sets it off when pressed three times quickly
static void bomb(uint16_t pressed, ShipsEvents_t* ev) {
	if (_bomb_flash) --_bomb_flash;
	if (_bomb_frames && --_bomb_frames == 0) _bomb_presses = 0;
	// Tab on a keyboard, as in the original, at once, as a key is not pressed by mistake
	const bool now = KEY_PRESSED(HID_KEY_TAB);
	if (!(pressed & EL_INP_BOMB) && !now) return;
	if (!cmdr.bomb) {
		message("NO ENERGY BOMB");
		return;
	}
	if (!_bomb_frames) _bomb_frames = BOMB_WINDOW;
	if (++_bomb_presses < BOMB_PRESSES && !now) {
		message("ENERGY BOMB");
		return;
	}
	_bomb_presses = _bomb_frames = 0;
	cmdr.bomb = false;
	ships_bomb(_me.pos, BOMB_RANGE, ev);
	_bomb_flash = BOMB_FLASH;
	message("ENERGY BOMB USED");
}

// Out in the pod, looking back at the ship
static void escape(void) {
	set_state(STATE_ESCAPE);
	_hulk = _me;
	_hulk_drift = vec_scale(_me.f, _speed * HULK_DRIFT);
	_hulk_on = true;
	_speed = POD_SPEED;
	_roll = _pitch = 0.0f;
	_view = VIEW_REAR;
	_offender = false;
	trade_escape();
	chart_set_fuel(cmdr.fuel);
}

static void move_pod(void) {
	_me.pos = vec_add(_me.pos, vec_scale(_me.f, _speed));
	move_dust(0.0f, 0.0f, _speed);
	frame_roll(&_hulk, HULK_TUMBLE);
	frame_pitch(&_hulk, HULK_TUMBLE * 0.7f);
	frame_tidy(&_hulk);
	_hulk.pos = vec_add(_hulk.pos, _hulk_drift);
	if (_hulk_on && _state_frames == HULK_BLOWS) {
		_hulk_on = false;
		ships_explode(_hulk.pos, model_cobra.radius);
	}
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
	for (uint32_t i = 0; i < ev->scooped; ++i) message(trade_scoop(i < ev->aliens));
	if (ev->smashed) message(cmdr.scoops ? "CARGO LOST" : "NO FUEL SCOOPS");
	_kills += ev->kills;
	cmdr.cash += ev->bounty;
	if (ev->bounty) {
		static char bounty[24];
		snprintf(bounty, sizeof(bounty), "BOUNTY %u.%u CR", (unsigned)(ev->bounty / 10), (unsigned)(ev->bounty % 10));
		message(bounty);
	}
	if (ev->innocent && !_offender) {
		_offender = true;
		message("OFFENDER");
	}
	if (_energy <= 0.0f) {
		_energy = 0.0f;
		if (cmdr.pod) escape();
		else if (_state != STATE_DEAD) set_state(STATE_DEAD);
		return;
	}
	_energy = fminf(1.0f, _energy + RECHARGE);
	_fore = fminf(1.0f, _fore + RECHARGE);
	_aft = fminf(1.0f, _aft + RECHARGE);
}

// The sun's heat on the cabin, and the fuel scoops taking fuel from it as
// we fly. Into the sun is the end of us.
static void heat(void) {
	const float d = vec_length(vec_sub(_me.pos, _sun_pos)) / SUN_RADIUS;
	if (d < 1.0f) {
		set_state(STATE_DEAD);
		return;
	}
	const float h = fminf(fmaxf((HEAT_RANGE - d) / (HEAT_RANGE - 1.0f), 0.0f), 1.0f);
	_cabin_temp = towards(_cabin_temp, CABIN_COOL + (1.0f - CABIN_COOL) * h, CABIN_CHANGE);
	if (_cabin_temp > CABIN_TOO_HOT) {
		_energy -= HEAT_DAMAGE;
		if (!_message_frames) message("CABIN TEMPERATURE");
	}
	if (cmdr.scoops && h > SCOOP_HEAT && _speed > 0.0f && cmdr.fuel < GALAXY_FUEL_MAX) {
		if (++_scoop_frames >= SCOOP_FRAMES) {
			_scoop_frames = 0;
			++cmdr.fuel;
			chart_set_fuel(cmdr.fuel);
			if (!_message_frames) message("FUEL SCOOPS ON");
		}
	}
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

// For trying the fuel scoops on the PC: fits them, and puts us near the sun,
// heading past it, with a canister ahead and a little above the nose
void game_debug_scoop(void) {
	cmdr.scoops = true;
	cmdr.fuel = 0;
	chart_set_fuel(cmdr.fuel);
	const Vec3_t out = vec_unit(_sun_pos);
	const Vec3_t side = vec_unit(vec_sub((Vec3_t){ 0.0f, 1.0f, 0.0f }, vec_scale(out, out.y)));
	frame_set(&_me, vec_add(_sun_pos, vec_scale(side, SUN_RADIUS * 2.0f)), out, side);
	ships_debug_canister(vec_add(_me.pos, vec_add(vec_scale(_me.f, 1500.0f), vec_scale(_me.u, 12.0f))));
}

// For trying the overdrive on the PC: turns us to face the sun
void game_debug_face_sun(void) {
	frame_set(&_me, _me.pos, vec_unit(vec_sub(_sun_pos, _me.pos)), _me.u);
}

// For trying the rear view on the PC: fits a rear laser, and puts a ship of
// the named type behind us, coming after us
// For trying the energy bomb on the PC: fits one, and puts a pirate ahead,
// another behind and a third to the side
void game_debug_bomb(void) {
	const ShipsWorld_t w = world();
	cmdr.bomb = true;
	ships_debug_place(0, "SIDEWINDER", vec_add(_me.pos, vec_scale(_me.f, 2500.0f)), vec_scale(_me.f, -1.0f), _me.u, &w);
	ships_debug_place(1, "MAMBA", vec_add(_me.pos, vec_scale(_me.f, 4000.0f)), vec_scale(_me.f, -1.0f), _me.u, &w);
	ships_debug_place(2, "KRAIT", vec_sub(_me.pos, vec_scale(_me.r, 3000.0f)), _me.f, _me.u, &w);
}

// For trying the targeting computer on the PC: fits everything, it among it,
// and puts a pirate ahead, a little off to the right and above, coming at us
void game_debug_target(void) {
	const ShipsWorld_t w = world();
	trade_max_equipment();
	const Vec3_t off = vec_add(vec_scale(_me.r, 220.0f), vec_scale(_me.u, 120.0f));
	ships_debug_place(0, "SIDEWINDER", vec_add(_me.pos, vec_add(vec_scale(_me.f, 3000.0f), off)), vec_scale(_me.f, -1.0f), _me.u, &w);
}

// For trying the galactic hyperdrive on the PC: fits one
void game_debug_galactic(void) {
	cmdr.galactic = true;
}

// For trying witchspace on the PC: a jump to the nearest system goes wrong,
// with the first Thargoid ahead of us, coming at us
void game_debug_witch(void) {
	_hyper_to = chart_nearest(1);
	cmdr.fuel -= chart_distance(_hyper_to);
	chart_set_fuel(cmdr.fuel);
	enter_witchspace();
	frame_set(&ships[0].fr, vec_add(_me.pos, vec_scale(_me.f, 3000.0f)), vec_scale(_me.f, -1.0f), _me.u);
}

// Buttons pressed from the PC, which has only the GPIO ones
static uint16_t _debug_pressed;

void game_debug_press(uint16_t buttons) {
	_debug_pressed |= buttons;
}

void game_debug_chaser(const char* name) {
	const ShipsWorld_t w = world();
	cmdr.laser[MOUNT_REAR] = LASER_PULSE;
	ships_debug_place(0, name, vec_sub(_me.pos, vec_scale(_me.f, 2500.0f)), _me.f, _me.u, &w);
}

// ----------------------------------------------------------------------------
// Drawing

typedef enum {
	THING_PLANET,
	THING_SUN,
	THING_STATION,
	THING_HULK,
	THING_SHIP,
	THING_CANISTER
} Thing_e;

typedef struct {
	Thing_e kind;
	uint32_t i;
	float near; // how far its nearest point is
} Thing_t;

// Where the ship the targeting computer follows is now, off the middle of the
// view, or the middle if none
static Vec2_t sights_off(void) {
	if (_tracked < 0 || !ships[_tracked].alive || !looking_out()) return (Vec2_t){ 0.0f, 0.0f };
	look();
	const Vec3_t v = frame_view(&_eye, ships[_tracked].fr.pos);
	if (v.z <= 0.0f) return (Vec2_t){ 0.0f, 0.0f };
	const Vec2_t p = project(&v);
	return (Vec2_t){ p.x - CENTRE_X, p.y - CENTRE_Y };
}

static void draw_dashboard(void) {
	_sights_off = sights_off();
	const Vec3_t planet = frame_view(&_me, _planet.pos);
	const Vec3_t station = frame_view(&_me, _station.pos);
	const float altitude = (vec_length(planet) - _planet_radius) / _planet_radius;
	Hud_t hud = {
		.fore_shield = _fore,
		.aft_shield = _aft,
		.fuel = (float)cmdr.fuel / GALAXY_FUEL_MAX,
		.cabin_temp = _cabin_temp,
		.laser_temp = _laser_temp,
		.altitude = fminf(fmaxf(altitude, 0.0f), 1.0f),
		.speed = _speed / MAX_SPEED,
		.overdrive = _over,
		.energy = _energy,
		.roll = _roll / MAX_ROLL,
		.pitch = _pitch / MAX_PITCH,
		.missiles = cmdr.missiles,
		.compass = _witch ? (Vec3_t){ 0.0f, 0.0f, 0.0f } : vec_length(station) < SAFE_ZONE * _planet_radius ? station : planet,
		.scanner_range = SCANNER_RANGE,
		.sights = looking_out() && cmdr.laser[_view] != LASER_NONE && _state == STATE_FLYING && !options_shown(),
		.sights_off = _sights_off,
		.locked = _tracked >= 0,
	};
	Blip_t blips[SHIPS + CANISTERS + 1];
	uint32_t n = 0;
	blips[n++] = (Blip_t){ station, _station_model->rgb, true };
	for (uint32_t i = 0; i < SHIPS; ++i) {
		if (ships[i].alive) blips[n++] = (Blip_t){ frame_view(&_me, ships[i].fr.pos), ships[i].model->rgb, false };
	}
	for (uint32_t i = 0; i < CANISTERS; ++i) {
		if (canisters[i].on) blips[n++] = (Blip_t){ frame_view(&_me, canisters[i].fr.pos), model_canister.rgb, false };
	}
	hud_draw(&hud, blips, n, draw_hud_filled);
}

static void draw_body(const WireModel_t* model, const Frame_t* fr) {
	float m[3][3];
	frame_matrix(&_eye, fr, m);
	draw_object(model, frame_view(&_eye, fr->pos), m, model->rgb, draw_style);
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
	if (vec_length(d) > GUIDE_RANGE || vec_dot(d, _station.f) < _station_face) return;
	const hstx_dvi_pixel_t c = lined_up() ? hstx_dvi_pixel_rgb(0, 255, 0) : hstx_dvi_pixel_rgb(255, 182, 0);
	for (uint32_t i = 0; i < GUIDES; ++i) {
		const float z = GUIDE_NEAR + GUIDE_GAP * (float)i;
		const float h = GUIDE_SIZE + (z - GUIDE_NEAR) * GUIDE_GROWTH;
		const Vec3_t mid = vec_add(_station.pos, vec_scale(_station.f, z));
		Vec3_t corner[4];
		for (uint32_t k = 0; k < 4; ++k) {
			const float x = (k == 0 || k == 3) ? -h : h;
			const float y = k < 2 ? h : -h;
			corner[k] = frame_view(&_eye, vec_add(mid, vec_add(vec_scale(_station.r, x), vec_scale(_station.u, y))));
		}
		if (!object_begin()) return;
		for (uint32_t k = 0; k < 4; ++k) add_edge(corner[k], corner[(k + 1) % 4], c);
		object_end();
	}
}

// The view out of the ship, the way we look
static void draw_view(void) {
	look();
	// The light comes from the sun
	const Vec3_t sun = frame_view(&_eye, _sun_pos);
	draw_light = vec_unit(sun);
	// Our laser, from the bottom corners of the view to the sights
	if (_laser_beam && object_begin()) {
		const hstx_dvi_pixel_t c = hstx_dvi_pixel_rgb(255, 255, 146);
		const float w = 220.0f * draw_aspect;
		const float x = CENTRE_X + _sights_off.x, y = CENTRE_Y + _sights_off.y;
		add_line((Vec2_t){ CENTRE_X - w, VIEW_BOTTOM - 1 }, (Vec2_t){ x - 3, y + 2 }, c);
		add_line((Vec2_t){ CENTRE_X + w, VIEW_BOTTOM - 1 }, (Vec2_t){ x + 3, y + 2 }, c);
		object_end();
	}
	ships_draw_effects(&_eye, draw_style, _frame);
	// In front of the slot, so nearer than the station
	draw_guides();

	// Nearest first, so they cover the ones behind
	Thing_t things[SHIPS + CANISTERS + 4];
	uint32_t n = 0;
	// None of them in witchspace
	if (!_witch) {
		things[n++] = (Thing_t){ THING_PLANET, 0, vec_length(frame_view(&_eye, _planet.pos)) - _planet_radius };
		things[n++] = (Thing_t){ THING_SUN, 0, vec_length(sun) - SUN_RADIUS };
		things[n++] = (Thing_t){ THING_STATION, 0, vec_length(frame_view(&_eye, _station.pos)) - _station_model->radius };
	}
	for (uint32_t i = 0; i < SHIPS; ++i) {
		if (!ships[i].alive) continue;
		const float d = vec_length(frame_view(&_eye, ships[i].fr.pos));
		if (d < SHIP_SEEN) things[n++] = (Thing_t){ THING_SHIP, i, d - ships[i].model->radius };
	}
	if (_state == STATE_ESCAPE && _hulk_on) {
		things[n++] = (Thing_t){ THING_HULK, 0, vec_length(frame_view(&_eye, _hulk.pos)) - model_cobra.radius };
	}
	for (uint32_t i = 0; i < CANISTERS; ++i) {
		if (!canisters[i].on) continue;
		const float d = vec_length(frame_view(&_eye, canisters[i].fr.pos));
		if (d < SHIP_SEEN) things[n++] = (Thing_t){ THING_CANISTER, i, d - model_canister.radius };
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
				frame_matrix(&_eye, &_planet, m);
				draw_planet(frame_view(&_eye, _planet.pos), _planet_radius, m, chart_economy_rgb(sys->economy),
					!(sys->seed[1] & 2), draw_style);
				break;
			}
			case THING_SUN: {
				// As big as it looks from here, which close up fills the view
				const float d2 = vec_dot(sun, sun);
				if (sun.z > SUN_NEAR_Z * sqrtf(d2) && d2 > SUN_RADIUS * SUN_RADIUS) {
					const Vec2_t m = project(&sun);
					const float rp = FOCAL * SUN_RADIUS / sqrtf(d2 - SUN_RADIUS * SUN_RADIUS);
					if (draw_style == STYLE_WIRE || !hidden(m, rp * SUN_RAY_REACH + 1.0f)) draw_sun(m, rp, draw_style, _frame);
				}
				break;
			}
			case THING_STATION:
				draw_body(_station_model, &_station);
				break;
			case THING_SHIP:
				draw_body(ships[t->i].model, &ships[t->i].fr);
				break;
			case THING_CANISTER:
				draw_body(&model_canister, &canisters[t->i].fr);
				break;
			case THING_HULK:
				draw_body(&model_cobra, &_hulk);
				break;
		}
	}
	// Behind it all
	dust_draw();
	// The energy bomb's flash, fading, behind everything
	if (_bomb_flash && object_begin()) {
		const uint8_t v = (uint8_t)(255 * _bomb_flash / BOMB_FLASH);
		add_rect(0.0f, VIEW_TOP, GAME_SCREEN_W, VIEW_BOTTOM, hstx_dvi_pixel_rgb(v, v, v));
		object_end();
	}
}

// ----------------------------------------------------------------------------
// Docked, launching and docking, and the end

#define STATUS_X 24
#define STATUS_VALUE_X (STATUS_X + 20 * 8)
#define STATUS_TOP (VIEW_TOP + 14)
#define STATUS_LINE 16
// The equipment fitted, in two columns
#define STATUS_FITTED_ROWS 7
#define STATUS_FITTED_W (22 * 8)
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
	// In columns, clear of the Cobra
	const char* fitted[STATUS_FITTED_ROWS * 2];
	const uint32_t n = trade_equipment_names(fitted, STATUS_FITTED_ROWS * 2);
	for (uint32_t i = 0; i < n; ++i) {
		const int32_t col = (int32_t)(i / STATUS_FITTED_ROWS), row = (int32_t)(i % STATUS_FITTED_ROWS);
		screen_text(STATUS_X + 16 + col * STATUS_FITTED_W, y + STATUS_LINE * (row + 1), fitted[i], GAME_WHITE, false);
	}
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
				// Out of witchspace in the pod, to the system we were going to
				if (_witch) {
					enter_system(_witch_to);
					trade_arrive(chart_system(_witch_to));
				}
				refit();
				set_state(STATE_DOCKED);
				save();
			}
			break;
		case STATE_HYPERSPACE:
			if (_state_frames >= HYPER_FRAMES) arrive();
			break;
		case STATE_ESCAPE:
			if (_state_frames >= POD_FRAMES) set_state(STATE_DOCKING);
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
	memcpy(_name, s->name, sizeof(_name));
	_kills = s->kills;
	_offender = s->offender != 0;
	chart_set_galaxy(s->galaxy);
	enter_system(s->home);
	trade_arrive(chart_system(s->home));
	chart_set_fuel(cmdr.fuel);
	frame_set(&_me, _station.pos, _station.f, _station.u);
}

// With nothing pressed for this long, the commanders go back to the title
#define COMMANDERS_WAIT (30 * 60)

// The list of commanders: the slot, name, where and cash, and then the rating
#define SLOTS_RATING 35

// The letters a name is made of; a space ends it
static const char _letters[] = " ABCDEFGHIJKLMNOPQRSTUVWXYZ";
#define LETTERS (sizeof(_letters) - 1)
// The name, in big letters, centred
#define NAME_X ((GAME_SCREEN_W - (SAVE_NAME_MAX - 1) * 16) / 2)
// Held, up or down goes on changing the letter, after a wait, every few frames
#define NAME_REPEAT_WAIT 20
#define NAME_REPEAT_EVERY 4

// A name for a new commander: up and down change the letter, held to go on,
// left and right move along, and fire takes it. Spaces are left out, and none is Jameson.
static void enter_name(void) {
	char name[SAVE_NAME_MAX];
	memset(name, ' ', SAVE_NAME_MAX - 1);
	memcpy(name, _name, strlen(_name));
	name[SAVE_NAME_MAX - 1] = 0;
	uint32_t at = 0;
	uint32_t held_frames = 0;
	// Anything typed before
	while (game_key_char()) {}
	bool done = false;
	while (!done) {
		// On a keyboard the name is typed, so its keys are not the pad's
		uint8_t pressed = game_input_pressed() & ~game_input_keys();
		const uint8_t held = game_input_get() & ~game_input_keys() & (EL_INP_UP | EL_INP_DOWN);
		held_frames = held ? held_frames + 1 : 0;
		if (held_frames >= NAME_REPEAT_WAIT && (held_frames - NAME_REPEAT_WAIT) % NAME_REPEAT_EVERY == 0) pressed |= held;
		const uint32_t l = (uint32_t)(strchr(_letters, name[at]) - _letters);
		if (pressed & EL_INP_UP) name[at] = _letters[(l + 1) % LETTERS];
		if (pressed & EL_INP_DOWN) name[at] = _letters[(l + LETTERS - 1) % LETTERS];
		if ((pressed & EL_INP_RIGHT) && at < SAVE_NAME_MAX - 2) ++at;
		if ((pressed & EL_INP_LEFT) && at > 0) --at;
		if (pressed & EL_INP_A) break;
		// Typed: letters and spaces go in, Backspace goes back, and Enter takes it
		for (char c = game_key_char(); c && !done; c = game_key_char()) {
			if (c == '\r') {
				done = true;
			}
			else if (c == '\b') {
				if (at > 0) --at;
				name[at] = ' ';
			}
			else if (strchr(_letters, toupper((unsigned char)c))) {
				name[at] = (char)toupper((unsigned char)c);
				if (at < SAVE_NAME_MAX - 2) ++at;
			}
		}
		screen_begin();
		draw_dashboard();
		// The name with the letter we are on in yellow, and spaces as dots
		char shown[SAVE_NAME_MAX];
		for (uint32_t i = 0; i < SAVE_NAME_MAX; ++i) shown[i] = name[i] == ' ' ? '.' : name[i];
		const char letter[2] = { shown[at], 0 };
		shown[at] = ' ';
		screen_text(NAME_X, CENTRE_Y - 16, shown, GAME_WHITE, true);
		screen_text(NAME_X + 16 * (int32_t)at, CENTRE_Y - 16, letter, GAME_YELLOW, true);
		screen_text_centre(VIEW_BOTTOM - 40, game_keyboard() ? "TYPE IT, OR UP AND DOWN CHANGE THE LETTER" : "UP AND DOWN CHANGE THE LETTER", GAME_CYAN, false);
		screen_text_centre(VIEW_BOTTOM - 24, game_keyboard() ? "FIRE OR ENTER TAKES THE NAME" : "FIRE TAKES THE NAME", GAME_CYAN, false);
		screen_end("NEW COMMANDER", "NAME");
		++_frame;
	}
	uint32_t n = 0;
	for (uint32_t i = 0; name[i]; ++i) {
		if (name[i] != ' ') _name[n++] = name[i];
	}
	_name[n] = 0;
	if (!n) strcpy(_name, "JAMESON");
}

// When there is somewhere to save, picks which commander to play: fire plays
// the one in a slot, or a new one if it is empty, and select starts a new one.
// Start shows the options, as in the game. Returns false if the game is ended
// from the options, or nothing is pressed for a while.
static bool choose_commander(void) {
	_slot = -1;
	if (!save_ready()) return true;
	static Save_t saves[SAVE_SLOTS];
	bool used[SAVE_SLOTS];
	for (uint32_t i = 0; i < SAVE_SLOTS; ++i) used[i] = save_read(i, &saves[i]);
	uint32_t pick = 0;
	uint32_t idle = 0;
	for (;;) {
		const uint16_t pressed = game_input_pressed();
		idle = game_input_get() ? 0 : idle + 1;
		if (idle >= COMMANDERS_WAIT) {
			options_close();
			return false;
		}
		if (options_shown()) {
			if (options_input(pressed)) return false;
		}
		else {
			if (pressed & EL_INP_START) options_open(true);
			if (pressed & EL_INP_DOWN) pick = (pick + 1) % SAVE_SLOTS;
			if (pressed & EL_INP_UP) pick = (pick + SAVE_SLOTS - 1) % SAVE_SLOTS;
			if (pressed & (EL_INP_A | EL_INP_SELECT)) {
				_slot = (int32_t)pick;
				if ((pressed & EL_INP_A) && used[pick]) load(&saves[pick]);
				else enter_name();
				return true;
			}
		}
		screen_begin();
		draw_dashboard();
		// Text draws straight into the row, so the options go over nothing else
		if (options_shown()) {
			options_draw();
			screen_end("COMMANDERS", "OPTIONS");
			++_frame;
			continue;
		}
		char line[GAME_TEXT_MAX + 1];
		const int32_t top = VIEW_TOP + 40;
		// Centred on the widest
		uint32_t widest = 8;
		for (uint32_t i = 0; i < SAVE_SLOTS; ++i) {
			const uint32_t w = used[i] ? SLOTS_RATING + (uint32_t)strlen(rating(saves[i].kills)) : 8;
			if (w > widest) widest = w;
		}
		const int32_t x = (GAME_SCREEN_W - 8 * (int32_t)widest) / 2;
		for (uint32_t i = 0; i < SAVE_SLOTS; ++i) {
			const uint32_t c = i == pick ? GAME_YELLOW : GAME_WHITE;
			const int32_t y = top + 24 * (int32_t)i;
			if (i == pick) screen_select(x, y, 8 * (int32_t)widest);
			if (used[i]) {
				const Save_t* sv = &saves[i];
				// Where it was saved, if in this galaxy
				const char* at = sv->galaxy == chart_galaxy() ? chart_system(sv->home)->name : "";
				snprintf(line, sizeof(line), "%u  %-8s %-9s %6u.%u Cr", (unsigned)(i + 1), sv->name, at,
					(unsigned)(sv->cmdr.cash / 10), (unsigned)(sv->cmdr.cash % 10));
				screen_text(x, y, line, c, false);
				screen_text(x + SLOTS_RATING * 8, y, rating(sv->kills), c, false);
			}
			else {
				snprintf(line, sizeof(line), "%u  EMPTY", (unsigned)(i + 1));
				screen_text(x, y, line, c, false);
			}
		}
		screen_text_centre(VIEW_BOTTOM - 40, "FIRE PLAYS", GAME_CYAN, false);
		screen_text_centre(VIEW_BOTTOM - 24, "SELECT FOR A NEW COMMANDER", GAME_CYAN, false);
		screen_end("COMMANDERS", "COMMANDERS");
		++_frame;
	}
}

#ifdef GAME_KEYBOARD
// The keys of BBC Elite that are not the pad's: f0 to f9 as F10 and F1 to F9,
// the views and screens, M missile, E ECM, J overdrive, H hyperspace and
// Ctrl-H galactic hyperspace, C docking computer, D data on the system
static void keyboard(void) {
	const bool docked = _state == STATE_DOCKED;
	if (_state != STATE_FLYING && !docked) return;
	if (KEY_PRESSED(HID_KEY_F5)) _view = VIEW_GALAXY;
	if (KEY_PRESSED(HID_KEY_F6)) _view = VIEW_LOCAL;
	if (KEY_PRESSED(HID_KEY_F9)) _view = VIEW_INVENTORY;
	if (KEY_PRESSED(HID_KEY_D)) _view = VIEW_DATA;
	if (docked) {
		if (KEY_PRESSED(HID_KEY_F10)) launch();
		if (KEY_PRESSED(HID_KEY_F4)) _view = VIEW_EQUIPMENT;
		if (KEY_PRESSED(HID_KEY_F7)) _view = VIEW_MARKET;
		if (KEY_PRESSED(HID_KEY_F8)) _view = VIEW_FRONT;
		return;
	}
	if (KEY_PRESSED(HID_KEY_F10)) _view = VIEW_FRONT;
	if (KEY_PRESSED(HID_KEY_F1)) _view = VIEW_REAR;
	if (KEY_PRESSED(HID_KEY_F2)) _view = VIEW_LEFT;
	if (KEY_PRESSED(HID_KEY_F3)) _view = VIEW_RIGHT;
	if (KEY_PRESSED(HID_KEY_M)) fire_missile();
	if (KEY_PRESSED(HID_KEY_E)) ecm();
	if (KEY_PRESSED(HID_KEY_C)) docking_computer(!_docking);
	if (KEY_PRESSED(HID_KEY_H)) start_hyperspace(game_key_modifiers() & (KEYBOARD_MODIFIER_LEFTCTRL | KEYBOARD_MODIFIER_RIGHTCTRL));
	if (KEY_PRESSED(HID_KEY_J)) {
		if (_overdrive) {
			overdrive_off();
		}
		else {
			_speed = MAX_SPEED;
			_over_ask = true;
		}
	}
}
#endif

void game_play(void) {
	start();
	if (!choose_commander()) return;
	for (;;) {
		const uint16_t pressed = game_input_pressed() | _debug_pressed;
		_debug_pressed = 0;
		const uint16_t held = game_input_get();
		if (options_shown()) {
			if (options_input(pressed)) return;
		}
		else {
			ShipsEvents_t ev = { 0 };
			if (pressed & EL_INP_START) options_open(true);
			// The docking computer, from X or the options; any flying control takes over from it
			if (options_dock()) docking_computer(true);
			else if (pressed & EL_INP_X) docking_computer(!_docking);
			else if (pressed & (EL_INP_LEFT | EL_INP_RIGHT | EL_INP_UP | EL_INP_DOWN | EL_INP_A | EL_INP_B)) docking_computer(false);
			const bool can_look = _state == STATE_DOCKED || _state == STATE_FLYING;
			if (pressed & EL_INP_SELECT) {
				// With B held, the ECM blows up the missiles flying
				if (held & EL_INP_B) {
					if (_state == STATE_FLYING) ecm();
				}
				else if (can_look) {
					_view = next_view();
				}
			}
#ifdef GAME_KEYBOARD
			keyboard();
#endif
			// A SNES pad's shoulders look round the ship, in flight
			if (_state == STATE_FLYING && (pressed & EL_INP_R)) _view = look_round(1);
			if (_state == STATE_FLYING && (pressed & EL_INP_L)) _view = look_round(-1);
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
					// and with B held on the galactic chart, the galactic hyperdrive to the next galaxy
					if ((pressed & EL_INP_A) && !looking_out()) start_hyperspace((held & EL_INP_B) && _view == VIEW_GALAXY);
					if (_hyper_frames && --_hyper_frames == 0) {
						// Still clear of everything at the end of the countdown
						if (mass_locked(LOCK_PLANET)) message("MASS LOCKED");
						else set_state(STATE_HYPERSPACE);
					}
					fire(held, pressed, &ev);
					bomb(pressed, &ev);
					fly(held);
					heat();
					break;
				case STATE_DEAD:
					_speed = 0.0f;
					move_dust(0.0f, 0.0f, 0.0f);
					break;
				case STATE_ESCAPE:
					move_pod();
					break;
				default:
					move_dust(0.0f, 0.0f, 20.0f);
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
					snprintf(title, sizeof(title), "COMMANDER %s", _name);
					label = "STATUS";
					draw_status();
				}
				else if (_state == STATE_LAUNCHING || _state == STATE_DOCKING || _state == STATE_HYPERSPACE) {
					snprintf(title, sizeof(title), _state == STATE_LAUNCHING ? "LAUNCHING" :
						_state == STATE_DOCKING ? "DOCKING" : _hyper_galactic ? "GALACTIC HYPERSPACE" : "HYPERSPACE");
					label = "TUNNEL";
					draw_tunnel(_state == STATE_HYPERSPACE);
				}
				else {
					snprintf(title, sizeof(title), "FRONT VIEW");
					draw_view();
				}
				break;
			case VIEW_REAR:
			case VIEW_LEFT:
			case VIEW_RIGHT:
				snprintf(title, sizeof(title), "%s", _view_labels[_view]);
				draw_view();
				break;
			case VIEW_MARKET:
				snprintf(title, sizeof(title), "%s MARKET PRICES", chart_system(chart_home())->name);
				trade_draw_market();
				break;
			case VIEW_EQUIPMENT:
				snprintf(title, sizeof(title), "EQUIP SHIP");
				trade_draw_equipment(chart_system(chart_home()));
				break;
			case VIEW_INVENTORY:
				snprintf(title, sizeof(title), "INVENTORY");
				trade_draw_inventory();
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
		if (_state == STATE_ESCAPE) snprintf(title, sizeof(title), "ESCAPE POD");
		if (_state == STATE_DEAD && looking_out()) {
			label = "GAME OVER";
			screen_text_centre(CENTRE_Y - 8, "GAME OVER", GAME_RED, true);
		}
		// The countdown to hyperspace, and any message
		if (_hyper_frames) {
			char count[12];
			snprintf(count, sizeof(count), "%u", (unsigned)((_hyper_frames + 59) / 60));
			screen_text(16, VIEW_TOP + 6, count, GAME_YELLOW, true);
		}
		// The ship the targeting computer follows, under the sights, where the eye is
		if (_tracked_name_frames) {
			--_tracked_name_frames;
			if (_state == STATE_FLYING && looking_out() && !options_shown()) {
				const int32_t x = (int32_t)(CENTRE_X + _sights_off.x);
				const int32_t sy = (int32_t)(CENTRE_Y + _sights_off.y);
				int32_t y = sy + TRACK_NAME_GAP;
				if (y + TRACK_NAME_H * 2 > VIEW_BOTTOM) y = sy - TRACK_NAME_GAP - TRACK_NAME_H * 2;
				screen_text_centre_on(x, y, _tracked_name, _tracked >= 0 ? GAME_GREEN : GAME_GREY, true);
				// and its bounty under it, or that it is clean, and not to be shot
				char bounty[24];
				if (_tracked_bounty) snprintf(bounty, sizeof(bounty), "BOUNTY %u.%u CR", (unsigned)(_tracked_bounty / 10), (unsigned)(_tracked_bounty % 10));
				else snprintf(bounty, sizeof(bounty), "CLEAN");
				screen_text_centre_on(x, y + TRACK_NAME_H + 2, bounty, _tracked < 0 ? GAME_GREY : _tracked_bounty ? GAME_YELLOW : GAME_WHITE, true);
			}
		}
		if (_message_frames) {
			--_message_frames;
			screen_text_centre(VIEW_BOTTOM - 32, _message, GAME_YELLOW, true);
		}
		// Text draws straight into the row, so the options go last, over everything else
		if (options_shown()) {
			label = "OPTIONS";
			options_draw();
		}
		screen_end(title, label);
		++_frame;
	}
}
