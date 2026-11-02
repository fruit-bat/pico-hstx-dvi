// Copyright (c) 2024 fruit-bat.

// See el_attract.h. Each screen is shown for a while, and any of them goes
// straight into a game when fire is pressed, or shows the options over it
// when start is.

#include "el_attract.h"
#include "el_chart.h"
#include "el_draw.h"
#include "el_hud.h"
#include "el_input.h"
#include "el_options.h"
#include "el_save.h"
#include "el_screen.h"
#include "el_space.h"
#include "game_pallet.h"
#include <math.h>
#include <stdio.h>

#define TITLE_FRAMES (10 * 60)
#define CHART_FRAMES (10 * 60)
#define DATA_SYSTEMS 4
#define DATA_FRAMES (150)
#define PARADE_FRAMES (20 * 60)
// Where we are in each galaxy
#define HOME 7

// ----------------------------------------------------------------------------
// The parade: every type of ship flying round a spinning space station, with
// missiles chasing two of them, past a sun, as in the wireframe test

typedef struct {
	const WireModel_t* model;
	Vec3_t pos;
	float yaw, pitch, roll;
} Parader_t;

enum {
	OBJ_STATION = 0,
	OBJ_SHIP,
	OBJ_MISSILE = OBJ_SHIP + MODEL_SHIP_COUNT,
	OBJECTS = OBJ_MISSILE + 2
};

static Parader_t _objects[OBJECTS];

// The station spins slowly with its slot towards us
#define STATION_Z 1700.0f
// The ships fly round it in a loop that comes close to us
#define LOOP_RADIUS 1300.0f
#define LOOP_SPEED 0.12f
// How long a missile is behind the ship it is chasing, in seconds
#define MISSILE_LAG 1.2f
// The sun, far away
static const Vec3_t _sun = { 5200.0f, 1500.0f, 9000.0f };
#define SUN_RADIUS 1600.0f
// How far the scanner sees
#define PARADE_SCANNER 3200.0f

static GameText_t _press[2];

// Where ship i is at time t, spaced out round the loop at its own height
static Vec3_t ship_path(uint32_t i, float t) {
	const float a = t * LOOP_SPEED + 2.0f * PI_F * (float)i / MODEL_SHIP_COUNT;
	const float r = LOOP_RADIUS + 120.0f * sinf((float)i * 1.3f);
	return (Vec3_t){
		r * sinf(a),
		150.0f * sinf((float)i * 2.4f) + 30.0f * sinf(t * 0.5f + (float)i),
		STATION_Z - r * cosf(a)
	};
}

// Points an object along the way it is going, banked by roll
static void face_along(Parader_t* o, const Vec3_t* from, const Vec3_t* to, float roll) {
	const float vx = to->x - from->x, vy = to->y - from->y, vz = to->z - from->z;
	o->yaw = atan2f(vx, vz);
	o->pitch = -atan2f(vy, sqrtf(vx * vx + vz * vz));
	o->roll = roll;
}

static void move_objects(uint32_t frame) {
	const float t = (float)frame / 60.0f;
	Parader_t* o = &_objects[OBJ_STATION];
	// Every other time round, the dodecahedron
	o->model = (frame / 100000u) & 1 ? &model_dodo : &model_coriolis;
	o->pos = (Vec3_t){ 0.0f, 0.0f, STATION_Z };
	o->yaw = PI_F;
	o->pitch = 0.15f;
	o->roll = t * 0.3f;

	for (uint32_t i = 0; i < MODEL_SHIP_COUNT; ++i) {
		o = &_objects[OBJ_SHIP + i];
		o->model = model_ships[i];
		o->pos = ship_path(i, t);
		const Vec3_t next = ship_path(i, t + 0.1f);
		// Banked into the turn, and rocking a little
		face_along(o, &o->pos, &next, 0.35f + 0.25f * sinf(t * 0.8f + (float)i));
	}

	// Each missile chases a ship, a little behind it and weaving
	for (uint32_t i = 0; i < 2; ++i) {
		o = &_objects[OBJ_MISSILE + i];
		o->model = &model_missile;
		const uint32_t target = i ? 7 : 19;
		const float lt = t - MISSILE_LAG;
		Vec3_t p = ship_path(target, lt);
		Vec3_t next = ship_path(target, lt + 0.1f);
		p.y += 40.0f * sinf(t * 3.0f + (float)i);
		next.y += 40.0f * sinf((t + 0.1f) * 3.0f + (float)i);
		o->pos = p;
		face_along(o, &p, &next, t * 4.0f);
	}
}

static void draw_parader(const Parader_t* o) {
	float m[3][3];
	make_matrix(m, o->yaw, o->pitch, o->roll);
	draw_object(o->model, o->pos, m, o->model->rgb, draw_style);
}

// Returns the name of the nearest ship
static const char* draw_parade(uint32_t frame) {
	dust_move((Vec3_t){ 0.0f, 0.0f, 0.0f }, (Vec3_t){ 0.0f, 0.0f, 6.0f });

	// Nearest first, so they cover the ones behind
	uint32_t order[OBJECTS];
	for (uint32_t i = 0; i < OBJECTS; ++i) {
		const float d = vec_dot(_objects[i].pos, _objects[i].pos);
		uint32_t j = i;
		for (; j > 0 && vec_dot(_objects[order[j - 1]].pos, _objects[order[j - 1]].pos) > d; --j) order[j] = order[j - 1];
		order[j] = i;
	}
	for (uint32_t i = 0; i < OBJECTS; ++i) draw_parader(&_objects[order[i]]);

	// The sun, after everything else so it is behind them all
	const Vec2_t m = project(&_sun);
	const float rp = FOCAL * SUN_RADIUS / _sun.z;
	if (draw_style == STYLE_WIRE || !hidden(m, rp * SUN_RAY_REACH + 1.0f)) draw_sun(m, rp, draw_style, frame);
	dust_draw();

	// The nearest ship, which comes first bar the station
	return _objects[order[0] == OBJ_STATION ? order[1] : order[0]].model->name;
}

// ----------------------------------------------------------------------------

typedef enum {
	SHOW_TITLE = 0,
	SHOW_GALAXY,
	SHOW_LOCAL,
	SHOW_DATA,
	SHOW_PARADE,
	SHOWS
} Show_e;

static const uint32_t _show_frames[SHOWS] = {
	TITLE_FRAMES, CHART_FRAMES, CHART_FRAMES, DATA_SYSTEMS * DATA_FRAMES, PARADE_FRAMES
};

static const char* const _show_labels[SHOWS] = {
	"TITLE", "GALACTIC CHART", "SHORT RANGE CHART", "DATA ON SYSTEM", "PARADE"
};

// The dashboard, with made up values that change over time
static void draw_dashboard(uint32_t frame, bool parade) {
	const float t = (float)frame / 60.0f;
	Hud_t hud = {
		.fore_shield = 0.75f + 0.25f * sinf(t * 0.7f),
		.aft_shield = 0.6f + 0.3f * sinf(t * 0.5f + 1.0f),
		.fuel = 0.85f,
		.cabin_temp = 0.3f + 0.1f * sinf(t * 0.3f),
		.laser_temp = 0.15f + 0.6f * fmaxf(0.0f, sinf(t * 1.3f)),
		.altitude = 0.0f,
		.speed = 0.55f + 0.3f * sinf(t * 0.3f),
		.energy = 0.8f + 0.2f * sinf(t * 0.2f),
		.roll = sinf(t * 0.4f),
		.pitch = 0.6f * sinf(t * 0.25f),
		.missiles = 2,
		.compass = _objects[OBJ_STATION].pos,
		.scanner_range = PARADE_SCANNER,
		.sights = parade,
	};
	Blip_t blips[OBJECTS];
	for (uint32_t i = 0; i < OBJECTS; ++i) {
		blips[i] = (Blip_t){ _objects[i].pos, _objects[i].model->rgb, i == OBJ_STATION };
	}
	hud_draw(&hud, blips, OBJECTS, draw_hud_filled);
}

// The title: a Cobra turning slowly in the middle of the view
static void draw_title(uint32_t frame) {
	const float t = (float)frame / 60.0f;
	float m[3][3];
	make_matrix(m, t * 0.5f, 0.45f + 0.15f * sinf(t * 0.3f), 0.15f * sinf(t * 0.4f));
	draw_object(&model_cobra, (Vec3_t){ 0.0f, 0.0f, 420.0f }, m, model_cobra.rgb, draw_style);
	GameText_t* p = &_press[draw_set()];
	game_text_set_smooth(p, "PRESS FIRE");
	// Flashing
	if ((frame / 30) & 1) {
		game_spr_text_centre(p, VIEW_BOTTOM - 30, game_pallet(GAME_CYAN));
		draw_count_sprite();
	}
	// Whether commanders can be saved
	if (save_built()) {
		char s[GAME_TEXT_MAX + 1];
		if (save_card()) snprintf(s, sizeof(s), "SD CARD READY");
		else snprintf(s, sizeof(s), "NO SD CARD (ERROR %d)", save_error());
		screen_text_centre(VIEW_BOTTOM - 10, s, save_card() ? GAME_GREEN : GAME_RED, false);
	}
	screen_text_centre(VIEW_BOTTOM - 46, "START FOR OPTIONS", GAME_GREY, false);
}

// The options, over the dashboard, until start is pressed again
static void show_options(uint32_t frame) {
	options_open(false);
	while (options_shown()) {
		options_input(game_input_pressed());
		screen_begin();
		draw_dashboard(frame, false);
		options_draw();
		screen_end("OPTIONS", "OPTIONS");
	}
}

void attract_run(void) {
	// A game may have ended in witchspace
	dust_strange = false;
	// Start with the first galaxy
	for (uint32_t round = 0; ; ++round) {
		const uint32_t g = round % GALAXIES;
		chart_set_galaxy(g);
		chart_set_home(HOME);
		for (uint32_t show = 0; show < SHOWS; ++show) {
			for (uint32_t f = 0; f < _show_frames[show]; ++f) {
				// The charts and data are always wireframes, to look like data
				const Style_e chart_style = draw_style == STYLE_WIRE ? STYLE_WIRE : STYLE_SOLID;
				const uint32_t frame = round * 100000u + show * 10000u + f;
				move_objects(frame);
				screen_begin();
				draw_dashboard(frame, show == SHOW_PARADE);
				char title[GAME_TEXT_MAX + 1];
				switch (show) {
					case SHOW_TITLE:
						snprintf(title, sizeof(title), "---- H A R M L E S S ----");
						draw_title(f);
						break;
					case SHOW_GALAXY:
						snprintf(title, sizeof(title), "GALACTIC CHART %u", (unsigned)(g + 1));
						chart_cursor_glide(false, f);
						chart_draw_galaxy();
						break;
					case SHOW_LOCAL:
						snprintf(title, sizeof(title), "SHORT RANGE CHART");
						chart_cursor_glide(true, f);
						chart_draw_local(chart_style);
						break;
					case SHOW_DATA:
						snprintf(title, sizeof(title), "DATA ON %s",
							chart_draw_data(chart_nearest(f / DATA_FRAMES), chart_style, frame));
						break;
					default:
						snprintf(title, sizeof(title), "%s", draw_parade(frame));
						break;
				}
				screen_end(title, _show_labels[show]);
				const uint16_t pressed = game_input_pressed();
				if (pressed & EL_INP_A) return;
				if (pressed & EL_INP_START) show_options(frame);
			}
		}
	}
}
