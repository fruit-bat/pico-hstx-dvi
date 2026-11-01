// Copyright (c) 2024 fruit-bat.

// See el_game.h.
//
// As in the original game, we are always at the middle of things: each frame
// everything is seen from our ship, along its axes.
//
// The controls:
//   left and right roll, and up and down dive and climb, as with a joystick
//   B held with up or down speeds up or slows down
//   select shows the next screen: the view, the charts and the data on the
//   system the cursor is on, which the pad moves on the charts
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

// The system: a planet, the station going round it with its slot facing it,
// and the sun far beyond
#define PLANET_RADIUS 12000.0f
#define STATION_DISTANCE 26000.0f
#define STATION_SPIN 0.005f
#define SUN_RADIUS 30000.0f
static const Vec3_t _sun_pos = { 150000.0f, 40000.0f, 300000.0f };
// Close to the station the compass points to it, and otherwise to the planet
#define SAFE_ZONE 40000.0f
// Where we start, in front of the slot, in station radii from its middle
#define LAUNCH_DISTANCE 2.0f
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

typedef enum {
	VIEW_FRONT = 0,
	VIEW_LOCAL,
	VIEW_GALAXY,
	VIEW_DATA,
	VIEWS
} View_e;

static const char* const _view_labels[VIEWS] = { "FRONT VIEW", "SHORT RANGE CHART", "GALACTIC CHART", "DATA ON SYSTEM" };

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
static Ship_t _ships[SHIPS];
static View_e _view;
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

static void start(void) {
	chart_set_galaxy(0);
	chart_set_home(HOME);
	frame_set(&_planet, (Vec3_t){ 0.0f, 0.0f, 0.0f }, (Vec3_t){ 0.0f, 0.0f, 1.0f }, (Vec3_t){ 0.2f, 1.0f, 0.0f });
	// The station's slot faces the planet
	const Vec3_t sp = { 0.0f, 0.0f, -STATION_DISTANCE };
	frame_set(&_station, sp, vec_unit(vec_sub(_planet.pos, sp)), (Vec3_t){ 0.0f, 1.0f, 0.0f });
	// Launched, just out of the slot facing the planet
	frame_set(&_me, vec_add(sp, vec_scale(_station.f, model_coriolis.radius * LAUNCH_DISTANCE)), _station.f, _station.u);
	_speed = MAX_SPEED * 0.25f;
	_roll = 0.0f;
	_pitch = 0.0f;
	for (uint32_t i = 0; i < SHIPS; ++i) new_ship(&_ships[i]);
	_view = VIEW_FRONT;
	_options = false;
	_option = 0;
	_frame = 0;
	dust_init();
}

// ----------------------------------------------------------------------------
// Moving

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
		// The pad moves the cursor on the charts
		const bool local = _view == VIEW_LOCAL;
		float step = local ? CURSOR_STEP_LOCAL : CURSOR_STEP_GALAXY;
		if (b) step *= CURSOR_FAST;
		float dx = 0.0f, dy = 0.0f;
		if (held & EL_INP_RIGHT) dx += step;
		if (held & EL_INP_LEFT) dx -= step;
		if (held & EL_INP_DOWN) dy += step;
		if (held & EL_INP_UP) dy -= step;
		if (_view != VIEW_DATA) chart_cursor_move(dx, dy, local);
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

	// Not into the planet, for now
	const Vec3_t out = vec_sub(_me.pos, _planet.pos);
	const float d = vec_length(out);
	if (d < PLANET_RADIUS + 100.0f) {
		_me.pos = vec_add(_planet.pos, vec_scale(out, (PLANET_RADIUS + 100.0f) / d));
		_speed = 0.0f;
	}
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
	const float altitude = (vec_length(planet) - PLANET_RADIUS) / PLANET_RADIUS;
	Hud_t hud = {
		.fore_shield = 1.0f,
		.aft_shield = 1.0f,
		.fuel = 1.0f,
		.cabin_temp = 0.1f,
		.laser_temp = 0.0f,
		.altitude = fminf(fmaxf(altitude, 0.0f), 1.0f),
		.speed = _speed / MAX_SPEED,
		.energy = 1.0f,
		.roll = _roll / MAX_ROLL,
		.pitch = _pitch / MAX_PITCH,
		.missiles = 4,
		.compass = vec_length(station) < SAFE_ZONE ? station : planet,
		.scanner_range = SCANNER_RANGE,
		.sights = _view == VIEW_FRONT && !_options,
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

static void draw_front(void) {
	// The light comes from the sun
	const Vec3_t sun = frame_view(&_me, _sun_pos);
	draw_light = vec_unit(sun);
	dust_draw();

	// Nearest first, so they cover the ones behind
	Thing_t things[SHIPS + 3];
	uint32_t n = 0;
	things[n++] = (Thing_t){ THING_PLANET, 0, vec_length(frame_view(&_me, _planet.pos)) - PLANET_RADIUS };
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
				draw_planet(frame_view(&_me, _planet.pos), PLANET_RADIUS, m, chart_economy_rgb(sys->economy),
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
			if (pressed & EL_INP_SELECT) _view = (View_e)((_view + 1) % VIEWS);
			fly(held);
			move_world();
		}

		screen_begin();
		draw_dashboard();
		// The charts and data are always wireframes, to look like data
		const Style_e chart_style = draw_style == STYLE_WIRE ? STYLE_WIRE : STYLE_SOLID;
		char title[GAME_TEXT_MAX + 1];
		switch (_view) {
			case VIEW_FRONT:
				snprintf(title, sizeof(title), "FRONT VIEW");
				draw_front();
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
		// Text draws straight into the row, so the options go last, over everything else
		if (_options) draw_options();
		screen_end(title, _options ? "OPTIONS" : _view_labels[_view]);
		++_frame;
	}
}
