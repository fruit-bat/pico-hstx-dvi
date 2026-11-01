// Copyright (c) 2024 fruit-bat.

// See el_chart.h.

#include "el_chart.h"
#include "el_space.h"
#include "game_pallet.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// The galactic chart, twice as wide as it is tall as in the original
// From the left of the chart to the middle of the screen, as it is drawn narrower for 16:9
#define GALAXY_LEFT ((float)CENTRE_X - 255.0f * draw_aspect)
#define GALAXY_TOP ((float)VIEW_TOP + 22.0f)
// The stars are added in bands of this many rows, so each sprite looks at a few
#define GALAXY_BAND_ROWS 8
// The short range chart: pixels for each step of x across the galaxy, and
// how far from us it shows each way, further across when it is drawn narrower
#define LOCAL_SCALE 7.2f
#define LOCAL_DX ((int32_t)(40.0f / draw_aspect))
#define LOCAL_DY 38
#define CHART_LABELS 32
#define CHART_INFO_Y (VIEW_BOTTOM - 10)
// In the attract, the cursor moves to another system this often, taking this long to get there
#define CHART_STEP_FRAMES 90
#define CHART_MOVE_FRAMES 30

static GameText_t _chart_labels[2][CHART_LABELS];
static GameText_t _chart_info[2];

// The galaxy shown, and the system we are at in it
static GalaxySystem_t _galaxy[GALAXY_SYSTEMS];
static uint32_t _galaxy_shown = GALAXIES;
static uint32_t _home = 0;
// Where we are when in witchspace, between two systems, and whether we are
static GalaxySystem_t _away;
static bool _is_away = false;
// The systems from the top of the chart down
static uint8_t _by_y[GALAXY_SYSTEMS];
// The systems on the short range chart, nearest first
static uint8_t _near[GALAXY_SYSTEMS];
static uint32_t _near_count;
// The cursor, in steps across the galaxy
static float _cursor_x, _cursor_y;
// How far we can jump, in tenths of a light year
static uint32_t _fuel = GALAXY_FUEL_MAX;

// The colour of a system on the short range chart, from rich industrial to poor agricultural
static const uint8_t _economy_rgb[8][3] = {
	{ 255, 73, 0 }, { 255, 146, 0 }, { 255, 219, 0 }, { 219, 255, 0 },
	{ 146, 255, 0 }, { 0, 255, 0 }, { 0, 219, 146 }, { 0, 182, 219 },
};

const uint8_t* chart_economy_rgb(uint32_t economy) {
	return _economy_rgb[economy & 7];
}

// Where we are: at the system, or between two in witchspace
static const GalaxySystem_t* here(void) {
	return _is_away ? &_away : &_galaxy[_home];
}

// The systems near us, for the short range chart, nearest first
static void find_near(void) {
	const GalaxySystem_t* h = here();
	_near_count = 0;
	for (uint32_t i = 0; i < GALAXY_SYSTEMS; ++i) {
		if (abs((int32_t)_galaxy[i].x - h->x) > LOCAL_DX || abs((int32_t)_galaxy[i].y - h->y) > LOCAL_DY) continue;
		const uint32_t d = galaxy_distance(h, &_galaxy[i]);
		uint32_t j = _near_count++;
		for (; j > 0 && galaxy_distance(h, &_galaxy[_near[j - 1]]) > d; --j) _near[j] = _near[j - 1];
		_near[j] = (uint8_t)i;
	}
}

void chart_set_galaxy(uint32_t g) {
	if (g == _galaxy_shown) return;
	_galaxy_shown = g;
	galaxy_make(g, _galaxy);
	for (uint32_t i = 0; i < GALAXY_SYSTEMS; ++i) {
		uint32_t j = i;
		for (; j > 0 && _galaxy[_by_y[j - 1]].y > _galaxy[i].y; --j) _by_y[j] = _by_y[j - 1];
		_by_y[j] = (uint8_t)i;
	}
	find_near();
}

uint32_t chart_galaxy(void) {
	return _galaxy_shown;
}

void chart_set_home(uint32_t i) {
	_home = i % GALAXY_SYSTEMS;
	_is_away = false;
	find_near();
	chart_cursor_set((float)_galaxy[_home].x, (float)_galaxy[_home].y);
}

void chart_set_away(uint32_t from, uint32_t to) {
	const GalaxySystem_t* a = &_galaxy[from % GALAXY_SYSTEMS];
	const GalaxySystem_t* b = &_galaxy[to % GALAXY_SYSTEMS];
	_away = *a;
	_away.x = (uint8_t)(((uint32_t)a->x + b->x) / 2);
	_away.y = (uint8_t)(((uint32_t)a->y + b->y) / 2);
	_is_away = true;
	find_near();
	chart_cursor_set((float)_away.x, (float)_away.y);
}

bool chart_away(void) {
	return _is_away;
}

uint32_t chart_distance(uint32_t to) {
	return galaxy_distance(here(), &_galaxy[to % GALAXY_SYSTEMS]);
}

void chart_set_fuel(uint32_t tenths) {
	_fuel = tenths;
}

void chart_refresh(void) {
	find_near();
}

uint32_t chart_home(void) {
	return _home;
}

const GalaxySystem_t* chart_system(uint32_t i) {
	return &_galaxy[i % GALAXY_SYSTEMS];
}

uint32_t chart_nearest(uint32_t k) {
	return _near_count ? _near[k % _near_count] : _home;
}

void chart_cursor_set(float x, float y) {
	_cursor_x = fminf(fmaxf(x, 0.0f), 255.0f);
	_cursor_y = fminf(fmaxf(y, 0.0f), 255.0f);
}

void chart_cursor_move(float dx, float dy, bool local) {
	float x = _cursor_x + dx, y = _cursor_y + dy;
	if (local) {
		// Kept on the short range chart
		const GalaxySystem_t* h = here();
		x = fminf(fmaxf(x, (float)h->x - LOCAL_DX), (float)h->x + LOCAL_DX);
		y = fminf(fmaxf(y, (float)h->y - LOCAL_DY), (float)h->y + LOCAL_DY);
	}
	chart_cursor_set(x, y);
}

// The system the cursor goes to at a step
static uint32_t glide_pick(bool local, uint32_t step) {
	const uint32_t h = (step * 2654435761u) >> 16;
	return local ? _near[h % _near_count] : h % GALAXY_SYSTEMS;
}

void chart_cursor_glide(bool local, uint32_t frame) {
	const uint32_t step = frame / CHART_STEP_FRAMES;
	const GalaxySystem_t* a = &_galaxy[glide_pick(local, step - 1)];
	const GalaxySystem_t* b = &_galaxy[glide_pick(local, step)];
	float t = (float)(frame % CHART_STEP_FRAMES) / (float)CHART_MOVE_FRAMES;
	if (t > 1.0f) t = 1.0f;
	t = t * t * (3.0f - 2.0f * t);
	chart_cursor_set((float)a->x + ((float)b->x - (float)a->x) * t, (float)a->y + ((float)b->y - (float)a->y) * t);
}

uint32_t chart_selected(void) {
	// Nearest the cursor, counting y as half, as the distances do
	uint32_t best = _home;
	float best_d = 1e9f;
	for (uint32_t i = 0; i < GALAXY_SYSTEMS; ++i) {
		const float dx = (float)_galaxy[i].x - _cursor_x;
		const float dy = ((float)_galaxy[i].y - _cursor_y) * 0.5f;
		const float d = dx * dx + dy * dy;
		if (d < best_d) {
			best_d = d;
			best = i;
		}
	}
	return best;
}

static Vec2_t galaxy_point(float x, float y) {
	return (Vec2_t){ GALAXY_LEFT + 2.0f * draw_aspect * x, GALAXY_TOP + y };
}

static Vec2_t local_point(float x, float y) {
	const GalaxySystem_t* h = here();
	return (Vec2_t){ CENTRE_X + (x - (float)h->x) * LOCAL_SCALE * draw_aspect, CENTRE_Y + (y - (float)h->y) * LOCAL_SCALE * 0.5f };
}

// A cross with a gap in the middle
static void chart_cross(Vec2_t c, float gap, float size, hstx_dvi_pixel_t colour) {
	add_line((Vec2_t){ c.x - size, c.y }, (Vec2_t){ c.x - gap, c.y }, colour);
	add_line((Vec2_t){ c.x + gap, c.y }, (Vec2_t){ c.x + size, c.y }, colour);
	add_line((Vec2_t){ c.x, c.y - size }, (Vec2_t){ c.x, c.y - gap }, colour);
	add_line((Vec2_t){ c.x, c.y + gap }, (Vec2_t){ c.x, c.y + size }, colour);
}

static void chart_ring(Vec2_t c, float r, uint32_t sides, hstx_dvi_pixel_t colour) {
	Vec2_t p[32];
	circle_points(p, sides, c.x, c.y, r);
	for (uint32_t i = 0; i < sides; ++i) add_line(p[i], p[(i + 1) % sides], colour);
}

// The name of the system the cursor is on, and how far it is from us
static void chart_info(GameText_t* info, uint32_t to) {
	const uint32_t d = galaxy_distance(here(), &_galaxy[to]);
	char s[GAME_TEXT_MAX];
	snprintf(s, sizeof(s), "%s %u.%u LIGHT YEARS", _galaxy[to].name, (unsigned)(d / 10), (unsigned)(d % 10));
	game_text_set(info, s, 1);
	game_spr_text_centre(info, CHART_INFO_Y, game_pallet(GAME_WHITE));
	draw_count_sprite();
}

void chart_draw_galaxy(void) {
	const hstx_dvi_pixel_t white = hstx_dvi_pixel_rgb(255, 255, 255);
	const float cx = _cursor_x, cy = _cursor_y;
	const uint32_t to = chart_selected();

	// The cursor, and how far we can jump from where we are
	const GalaxySystem_t* h = here();
	if (object_begin()) {
		chart_cross(galaxy_point(cx, cy), 3.0f, 10.0f, hstx_dvi_pixel_rgb(255, 255, 0));
		object_end();
	}
	if (object_begin()) {
		const Vec2_t c = galaxy_point((float)h->x, (float)h->y);
		chart_cross(c, 0.0f, 6.0f, white);
		// A jump is 4 tenths of a light year for each step across
		chart_ring(c, 2.0f * (float)_fuel / 4.0f, 24, hstx_dvi_pixel_rgb(0, 255, 0));
		object_end();
	}

	// The systems, in bands of rows
	int32_t band = -1;
	for (uint32_t i = 0; i < GALAXY_SYSTEMS; ++i) {
		const GalaxySystem_t* s = &_galaxy[_by_y[i]];
		if (s->y / GALAXY_BAND_ROWS != band) {
			object_end();
			if (!object_begin()) break;
			band = s->y / GALAXY_BAND_ROWS;
		}
		const Vec2_t p = galaxy_point((float)s->x, (float)s->y);
		add_rect(p.x, p.y, p.x + 2.0f, p.y + 2.0f, white);
	}
	object_end();

	if (object_begin()) {
		add_box(GALAXY_LEFT - 6.0f, GALAXY_TOP - 6.0f, GALAXY_LEFT + 2.0f * draw_aspect * 255.0f + 8.0f, GALAXY_TOP + 255.0f + 6.0f,
			hstx_dvi_pixel_rgb(36, 109, 219));
		object_end();
	}
	chart_info(&_chart_info[draw_set()], to);
}

// A box taken on the chart, by a system or a label
typedef struct {
	int32_t x0, y0, x1, y1;
} ChartBox_t;

// Whether a label at x, y, w wide is on the chart and clear of the boxes taken
static bool label_free(const ChartBox_t* taken, uint32_t n, int32_t x, int32_t y, int32_t w) {
	if (x < 0 || x + w > GAME_SCREEN_W || y < VIEW_TOP || y + 8 > CHART_INFO_Y - 2) return false;
	for (uint32_t i = 0; i < n; ++i) {
		if (x < taken[i].x1 + 3 && taken[i].x0 < x + w + 3 && y < taken[i].y1 + 2 && taken[i].y0 < y + 10) return false;
	}
	return true;
}

// Drawn as a wireframe, solid unless style is STYLE_WIRE
void chart_draw_local(Style_e style) {
	GameText_t* labels = _chart_labels[draw_set()];
	const float cx = _cursor_x, cy = _cursor_y;
	const uint32_t to = chart_selected();
	const GalaxySystem_t* h = here();
	const Vec2_t home = local_point((float)h->x, (float)h->y);

	if (object_begin()) {
		chart_cross(local_point(cx, cy), 6.0f, 18.0f, hstx_dvi_pixel_rgb(255, 255, 0));
		object_end();
	}
	if (object_begin()) {
		chart_cross(home, 0.0f, 8.0f, hstx_dvi_pixel_rgb(255, 255, 255));
		object_end();
	}

	// The systems, as circles as big as their planets, coloured by what they make.
	// The labels must keep clear of them.
	// Static, as they are too big for the stack
	static float radius[GALAXY_SYSTEMS];
	static ChartBox_t taken[GALAXY_SYSTEMS + CHART_LABELS];
	for (uint32_t i = 0; i < _near_count; ++i) {
		const GalaxySystem_t* s = &_galaxy[_near[i]];
		const Vec2_t c = local_point((float)s->x, (float)s->y);
		const float r = 3.0f + (float)(s->radius - 2816) / 512.0f;
		radius[i] = r;
		taken[i] = (ChartBox_t){ (int32_t)(c.x - r), (int32_t)(c.y - r), (int32_t)(c.x + r), (int32_t)(c.y + r) };
		if (!object_begin()) break;
		Vec2_t p[16];
		circle_points(p, 16, c.x, c.y, r);
		const hstx_dvi_pixel_t colour = shade(_economy_rgb[s->economy], 1.0f);
		for (uint32_t j = 0; j < 16; ++j) add_line(p[j], p[(j + 1) % 16], colour);
		// Solid, so the ring behind does not show through
		if (style != STYLE_WIRE) add_fill(p, 16, sprite_colour(hstx_dvi_pixel_rgb(0, 0, 0)));
		object_end();
	}

	// How far we can jump, in two halves so each sprite looks at fewer lines on a row
	const float range = (float)_fuel / 4.0f * LOCAL_SCALE;
	Vec2_t ring[32];
	circle_points(ring, 32, home.x, home.y, range);
	for (uint32_t half = 0; half < 2; ++half) {
		if (!object_begin()) break;
		for (uint32_t j = half * 16; j < half * 16 + 16; ++j) add_line(ring[j], ring[(j + 1) % 32], hstx_dvi_pixel_rgb(0, 255, 0));
		object_end();
	}

	// The names, beside the systems where there is room. Text draws straight
	// into the row, so these go last.
	uint32_t n = 0;
	for (uint32_t i = 0; i < _near_count && n < CHART_LABELS; ++i) {
		const GalaxySystem_t* s = &_galaxy[_near[i]];
		const Vec2_t c = local_point((float)s->x, (float)s->y);
		GameText_t* t = &labels[n];
		game_text_set(t, s->name, 1);
		const int32_t w = game_text_width(t);
		const int32_t r = (int32_t)radius[i] + 4;
		const int32_t x = (int32_t)c.x, y = (int32_t)c.y - 4;
		const int32_t try[4][2] = { { x + r, y }, { x - r - w, y }, { x - w / 2, y - r - 8 }, { x - w / 2, y + r } };
		for (uint32_t k = 0; k < 4; ++k) {
			if (!label_free(taken, _near_count + n, try[k][0], try[k][1], w)) continue;
			game_spr_text(t, try[k][0], try[k][1], game_pallet(_near[i] == to ? GAME_YELLOW : GAME_WHITE));
			draw_count_sprite();
			taken[_near_count + n] = (ChartBox_t){ try[k][0], try[k][1], try[k][0] + w, try[k][1] + 8 };
			++n;
			break;
		}
	}
	chart_info(&_chart_info[draw_set()], to);
}


// a circle with a meridian and an equator, or a crater, on the side facing us.

// Where the data starts, which sets the border round it
#define DATA_X 24
#define DATA_TOP (VIEW_TOP + 10)
// The picture goes under the data, inside the same border
#define PICTURE_LEFT ((float)DATA_X)
#define PICTURE_RIGHT ((float)(GAME_SCREEN_W - DATA_X))
#define PICTURE_TOP ((float)(DATA_TOP + 160))
#define PICTURE_BOTTOM ((float)(VIEW_BOTTOM - 10))
// The sun at the left, rays and all
#define PICTURE_SUN 32.0f
// Where the planet is on the screen, and how far away, in station radii,
// which sets how big the station looks
#define SYSTEM_X ((PICTURE_LEFT + PICTURE_RIGHT) * 0.5f + 60.0f)
#define SYSTEM_Y ((PICTURE_TOP + PICTURE_BOTTOM) * 0.5f)
#define SYSTEM_DEPTH 40.0f
// How far the station is from the middle of the planet, in planet radii
#define ORBIT 1.9f
// The planet's axis leans towards us by this much, so we see the north pole
#define PLANET_TILT -0.4f


#define DATA_TEXTS 32
#define DATA_LINE 14
// The letters in a line of the description
#define DATA_WRAP 74

static GameText_t _data_text[2][DATA_TEXTS];

// Turns a point on the planet about its axis, then leans it
static void planet_matrix(float m[3][3], float spin) {
	const float cs = cosf(spin), ss = sinf(spin);
	const float ct = cosf(PLANET_TILT), st = sinf(PLANET_TILT);
	m[0][0] = cs; m[0][1] = 0.0f; m[0][2] = ss;
	m[1][0] = st * ss; m[1][1] = ct; m[1][2] = -st * cs;
	m[2][0] = -ct * ss; m[2][1] = st; m[2][2] = ct * cs;
}

static void draw_system(const GalaxySystem_t* sys, Style_e style, uint32_t frame) {
	const float t = (float)frame / 60.0f;

	// The planet, as big on the screen as it is in km
	const float z = model_coriolis.radius * SYSTEM_DEPTH;
	const float r_px = 18.0f + 26.0f * (float)(sys->radius - 2816) / 4096.0f;
	const float r = r_px * z / FOCAL;
	const Vec3_t centre = { (SYSTEM_X - CENTRE_X) * z / (FOCAL * draw_aspect), (CENTRE_Y - SYSTEM_Y) * z / FOCAL, z };

	// The station, going round in a ring leaning towards us
	const float a = t * 0.5f;
	const Vec3_t station = { centre.x + ORBIT * r * cosf(a), centre.y + 0.3f * ORBIT * r * sinf(a), centre.z + ORBIT * r * sinf(a) };
	float sm[3][3];
	make_matrix(sm, 0.3f * sinf(t * 0.2f), 0.2f, t * 0.6f);
	const bool in_front = station.z < centre.z;
	if (in_front) draw_object(&model_coriolis, station, sm, model_coriolis.rgb, style);
	float pm[3][3];
	planet_matrix(pm, t * 0.4f);
	draw_planet(centre, r, pm, _economy_rgb[sys->economy], !(sys->seed[1] & 2), style);
	if (!in_front) draw_object(&model_coriolis, station, sm, model_coriolis.rgb, style);

	// The sun, off to one side
	draw_sun((Vec2_t){ PICTURE_LEFT + PICTURE_SUN * SUN_RAY_REACH * draw_aspect, SYSTEM_Y }, PICTURE_SUN, style, frame);
}

// A line of the data, its name in cyan and the rest in white, returning the
// next free text. A text holds GAME_TEXT_MAX letters, so a longer line takes more.
static uint32_t data_line(GameText_t* texts, uint32_t t, int32_t y, const char* name, const char* value) {
	int32_t x = DATA_X;
	if (name && t < DATA_TEXTS) {
		game_text_set(&texts[t], name, 1);
		game_spr_text(&texts[t], x, y, game_pallet(GAME_CYAN));
		x += game_text_width(&texts[t]) + 8;
		++t;
		draw_count_sprite();
	}
	for (size_t len = strlen(value); len && t < DATA_TEXTS; ++t) {
		char part[GAME_TEXT_MAX + 1];
		const size_t n = len < GAME_TEXT_MAX ? len : GAME_TEXT_MAX;
		memcpy(part, value, n);
		part[n] = 0;
		game_text_set(&texts[t], part, 1);
		game_spr_text(&texts[t], x, y, game_pallet(GAME_WHITE));
		x += game_text_width(&texts[t]);
		value += n;
		len -= n;
		draw_count_sprite();
	}
	return t;
}

const char* chart_draw_data(uint32_t i, Style_e style, uint32_t frame) {
	GameText_t* texts = _data_text[draw_set()];
	const GalaxySystem_t* h = here();
	const GalaxySystem_t* sys = &_galaxy[i % GALAXY_SYSTEMS];
	char v[160];
	uint32_t t = 0;
	int32_t y = DATA_TOP;
	const uint32_t d = galaxy_distance(h, sys);
	snprintf(v, sizeof(v), "%u.%u Light Years", (unsigned)(d / 10), (unsigned)(d % 10));
	t = data_line(texts, t, y, "Distance:", v);
	y += DATA_LINE;
	t = data_line(texts, t, y, "Economy:", galaxy_economy_names[sys->economy]);
	y += DATA_LINE;
	t = data_line(texts, t, y, "Government:", galaxy_government_names[sys->government]);
	y += DATA_LINE;
	snprintf(v, sizeof(v), "%u", (unsigned)sys->tech);
	t = data_line(texts, t, y, "Tech.Level:", v);
	y += DATA_LINE;
	snprintf(v, sizeof(v), "%u.%u Billion", (unsigned)(sys->population / 10), (unsigned)(sys->population % 10));
	t = data_line(texts, t, y, "Population:", v);
	y += DATA_LINE;
	char species[48];
	galaxy_species(sys, species, sizeof(species));
	snprintf(v, sizeof(v), "(%s)", species);
	t = data_line(texts, t, y, NULL, v);
	y += DATA_LINE;
	const uint32_t productivity = ((sys->economy ^ 7) + 3) * (sys->government + 4) * sys->population * 8;
	snprintf(v, sizeof(v), "%u M CR", (unsigned)productivity);
	t = data_line(texts, t, y, "Gross Productivity:", v);
	y += DATA_LINE;
	snprintf(v, sizeof(v), "%u km", (unsigned)sys->radius);
	t = data_line(texts, t, y, "Average Radius:", v);
	y += DATA_LINE * 2;

	// What it is known for, a word at a time to fit the width
	galaxy_description(sys, v, sizeof(v));
	const char* p = v;
	while (*p && y + DATA_LINE <= VIEW_BOTTOM) {
		uint32_t n = 0, cut = 0;
		while (p[n] && n < DATA_WRAP) {
			if (p[n] == ' ') cut = n;
			++n;
		}
		if (!p[n] || !cut) cut = n;
		char line[DATA_WRAP + 1];
		memcpy(line, p, cut);
		line[cut] = 0;
		t = data_line(texts, t, y, NULL, line);
		y += DATA_LINE;
		p += cut;
		while (*p == ' ') ++p;
	}
	draw_system(sys, style, frame);
	return sys->name;
}
