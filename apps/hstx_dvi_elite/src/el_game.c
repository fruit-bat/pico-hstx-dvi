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
#include "el_space.h"
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

// The other ships, flying about near the station
#define SHIPS 4
#define SHIP_RANGE 30000.0f
#define SCANNER_RANGE 12000.0f

typedef struct {
	const WireModel_t* model;
	Frame_t fr;
	float speed, roll, pitch;
} Ship_t;

// The front view is our status when docked
typedef enum {
	VIEW_FRONT = 0,
	VIEW_LOCAL,
	VIEW_GALAXY,
	VIEW_DATA,
	VIEWS
} View_e;

static const char* const _view_labels[VIEWS] = { "FRONT VIEW", "SHORT RANGE CHART", "GALACTIC CHART", "DATA ON SYSTEM" };

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
// How far we can jump, in tenths of a light year
static uint32_t _fuel;
// Frames left counting down to hyperspace, and where to
static uint32_t _hyper_frames;
static uint32_t _hyper_to;
static const char* _message;
static uint32_t _message_frames;
static Ship_t _ships[SHIPS];
static View_e _view;
static State_e _state;
// Frames into the state, for the tunnel and the end
static uint32_t _state_frames;
static bool _options;
static uint32_t _option;
static uint32_t _frame;
static GameText_t _option_text[2][OPTIONS + 1];

static float rand_range(float lo, float hi) {
	return lo + (hi - lo) * (float)(rand() & 0xffff) / 65535.0f;
}

static Vec3_t rand_unit(void) {
	return vec_unit((Vec3_t){ rand_range(-1.0f, 1.0f), rand_range(-1.0f, 1.0f), rand_range(-1.0f, 1.0f) });
}

// Moves a value towards a target by at most step
static float towards(float v, float target, float step) {
	if (v < target) return fminf(v + step, target);
	return fmaxf(v - step, target);
}

// A ship somewhere round the station, going its own way
static void new_ship(Ship_t* s) {
	s->model = model_ships[rand() % MODEL_SHIP_COUNT];
	const Vec3_t pos = vec_add(_station.pos, vec_scale(rand_unit(), rand_range(3000.0f, 9000.0f)));
	frame_set(&s->fr, pos, rand_unit(), rand_unit());
	s->speed = rand_range(3.0f, 10.0f);
	s->roll = rand_range(-0.01f, 0.01f);
	s->pitch = rand_range(-0.004f, 0.004f);
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
	for (uint32_t s = 0; s < SHIPS; ++s) new_ship(&_ships[s]);
}

static void start(void) {
	chart_set_galaxy(0);
	enter_system(HOME);
	frame_set(&_me, _station.pos, _station.f, _station.u);
	_speed = 0.0f;
	_roll = 0.0f;
	_pitch = 0.0f;
	_fuel = GALAXY_FUEL_MAX;
	chart_set_fuel(_fuel);
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

// Starts the countdown to the system the cursor is on, if we can reach it
static void start_hyperspace(void) {
	if (_hyper_frames) return;
	const uint32_t to = chart_selected();
	if (to == chart_home()) {
		message("WE ARE HERE");
		return;
	}
	if (galaxy_distance(chart_system(chart_home()), chart_system(to)) > _fuel) {
		message("OUT OF RANGE");
		return;
	}
	_hyper_to = to;
	_hyper_frames = HYPER_COUNT * 60;
}

// Out of hyperspace in the new system, facing its planet
static void arrive(void) {
	_fuel -= galaxy_distance(chart_system(chart_home()), chart_system(_hyper_to));
	chart_set_fuel(_fuel);
	enter_system(_hyper_to);
	frame_set(&_me, (Vec3_t){ 0.0f, 0.0f, -ARRIVAL * _planet_radius }, (Vec3_t){ 0.0f, 0.0f, 1.0f }, (Vec3_t){ 0.0f, 1.0f, 0.0f });
	_speed = MAX_SPEED * 0.25f;
	_roll = 0.0f;
	_pitch = 0.0f;
	_view = VIEW_FRONT;
	dust_init();
	set_state(STATE_FLYING);
}

// Out through the slot, facing the way it does, which is towards the planet
static void launch(void) {
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
	dust_move(_roll, _pitch, _speed * 0.5f);

	touch_station();
	if (vec_length(vec_sub(_me.pos, _planet.pos)) < _planet_radius) set_state(STATE_DEAD);
}

static void move_world(void) {
	frame_roll(&_station, STATION_SPIN);
	frame_tidy(&_station);
	for (uint32_t i = 0; i < SHIPS; ++i) {
		Ship_t* s = &_ships[i];
		frame_roll(&s->fr, s->roll);
		frame_pitch(&s->fr, s->pitch);
		frame_tidy(&s->fr);
		s->fr.pos = vec_add(s->fr.pos, vec_scale(s->fr.f, s->speed));
		if (vec_length(vec_sub(s->fr.pos, _me.pos)) > SHIP_RANGE) new_ship(s);
	}
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
		.fore_shield = 1.0f,
		.aft_shield = 1.0f,
		.fuel = (float)_fuel / GALAXY_FUEL_MAX,
		.cabin_temp = 0.1f,
		.laser_temp = 0.0f,
		.altitude = fminf(fmaxf(altitude, 0.0f), 1.0f),
		.speed = _speed / MAX_SPEED,
		.energy = 1.0f,
		.roll = _roll / MAX_ROLL,
		.pitch = _pitch / MAX_PITCH,
		.missiles = 4,
		.compass = vec_length(station) < SAFE_ZONE * _planet_radius ? station : planet,
		.scanner_range = SCANNER_RANGE,
		.sights = _view == VIEW_FRONT && _state == STATE_FLYING && !_options,
	};
	Blip_t blips[SHIPS + 1];
	uint32_t n = 0;
	blips[n++] = (Blip_t){ station, model_coriolis.rgb, true };
	for (uint32_t i = 0; i < SHIPS; ++i) {
		blips[n++] = (Blip_t){ frame_view(&_me, _ships[i].fr.pos), _ships[i].model->rgb, false };
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
	dust_draw();
	// In front of the slot, so nearer than the station
	draw_guides();

	// Nearest first, so they cover the ones behind
	Thing_t things[SHIPS + 3];
	uint32_t n = 0;
	things[n++] = (Thing_t){ THING_PLANET, 0, vec_length(frame_view(&_me, _planet.pos)) - _planet_radius };
	things[n++] = (Thing_t){ THING_SUN, 0, vec_length(sun) - SUN_RADIUS };
	things[n++] = (Thing_t){ THING_STATION, 0, vec_length(frame_view(&_me, _station.pos)) - model_coriolis.radius };
	for (uint32_t i = 0; i < SHIPS; ++i) {
		things[n++] = (Thing_t){ THING_SHIP, i, vec_length(frame_view(&_me, _ships[i].fr.pos)) - _ships[i].model->radius };
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
				draw_body(_ships[t->i].model, &_ships[t->i].fr);
				break;
		}
	}
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
		if (vec_length(vec_sub(_ships[i].fr.pos, _me.pos)) < SCANNER_RANGE) return true;
	}
	return false;
}

// Our status, as in the original game, with a Cobra like ours turning beside it
static void draw_status(void) {
	const char* const names[] = {
		"Present System", "Hyperspace System", "Condition", "Fuel", "Cash", "Legal Status", "Rating"
	};
	char fuel[24];
	snprintf(fuel, sizeof(fuel), "%u.%u Light Years", (unsigned)(_fuel / 10), (unsigned)(_fuel % 10));
	const char* values[] = {
		chart_system(chart_home())->name, chart_system(chart_selected())->name,
		_state == STATE_DOCKED ? "Docked" : ships_near() ? "Yellow" : "Green",
		fuel, "100.0 Cr", "Clean", "Harmless"
	};
	int32_t y = STATUS_TOP;
	for (uint32_t i = 0; i < sizeof(names) / sizeof(names[0]); ++i) {
		screen_text(STATUS_X, y, names[i], GAME_CYAN, false);
		screen_text(STATUS_VALUE_X, y, values[i], GAME_WHITE, false);
		y += STATUS_LINE;
	}
	y += STATUS_LINE;
	screen_text(STATUS_X, y, "EQUIPMENT:", GAME_CYAN, false);
	screen_text(STATUS_X + 16, y + STATUS_LINE, "Front Pulse Laser", GAME_WHITE, false);
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
				// Filled up, for now
				_fuel = GALAXY_FUEL_MAX;
				chart_set_fuel(_fuel);
				set_state(STATE_DOCKED);
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

void game_play(void) {
	start();
	for (;;) {
		const uint8_t pressed = game_input_pressed();
		const uint8_t held = game_input_get();
		if (_options) {
			if (options(pressed)) return;
		}
		else {
			if (pressed & EL_INP_START) _options = true;
			const bool can_look = _state == STATE_DOCKED || _state == STATE_FLYING;
			if (can_look && (pressed & EL_INP_SELECT)) _view = (View_e)((_view + 1) % VIEWS);
			switch (_state) {
				case STATE_DOCKED:
					move_cursor(held);
					// Only from the status, so not by mistake on the charts
					if ((pressed & EL_INP_A) && _view == VIEW_FRONT) launch();
					break;
				case STATE_FLYING:
					// Fire on the charts or data jumps to the system the cursor is on
					if ((pressed & EL_INP_A) && _view != VIEW_FRONT) start_hyperspace();
					if (_hyper_frames && --_hyper_frames == 0) set_state(STATE_HYPERSPACE);
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
			move_world();
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
			screen_text_centre(VIEW_BOTTOM - 44, _message, GAME_YELLOW, true);
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
