// Copyright (c) 2024 fruit-bat.

// A space station, ships and missiles in the style of Elite, drawn with
// sprites. Every type of ship flies round the station, past a sun.
//
// Each object is a sprite, with a list of the lines and convex shapes that
// make it up, drawn by sprite_renderer_parts. On each row it works out the
// run of each line and shape that falls on the row, from the row alone. The
// shapes are cut into bands a few rows deep, and fill from their left side
// to their right side.
//
// A pixel can only be drawn once, so the parts earlier in the list keep
// priority over the later ones, as with the sprites themselves.
//
// The faces at the back of a ship are left out, as the original game did,
// and an edge is only drawn if one of the faces it joins is facing us. The
// models are convex, so the faces left never cover each other, and a ship
// seen from anywhere has a convex outline. Filling that outline in black
// after the edges makes it solid, with a single shape.
//
// The ships are added nearest first. Lower sprite ids take priority, so the
// nearer ones cover those behind them and no depth buffer is needed.
//
// This example requires an external digital video connector connected to
// GPIOs 12 through 19 (the HSTX-capable GPIOs) with appropriate
// current-limiting resistors, e.g. 270 ohms. The pinout used in this example
// matches the Pico DVI Sock board, which can be soldered onto a Pico 2:
// https://github.com/Wren6991/Pico-DVI-Sock

#include "hstx_dvi_core.h"
#include "hstx_dvi_sprite.h"
#include "hstx_dvi_row_fifo.h"
#include "app_led.h"
#include "pico/stdlib.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "game_pallet.h"
#include "game_render.h"
#include "game_util.h"
#include "ship_model.h"
#include "galaxy.h"
#if defined(__ARM_ARCH)
#include "hardware/structs/systick.h"
#endif

// Enough for the scene
#define MAX_OBJECTS 128
#define MAX_PRIMS 1200
#define MAX_ALL_BANDS 2400
#define MAX_VERTS 48
#define MAX_FACES 32
// A shape can gain a corner for each side of the view it is cut by, and one at the near plane
#define MAX_POLY 24
#define STARS 40
// The distance to the screen, in pixels, which sets the field of view
#define FOCAL 320.0f
// Nothing closer than this is drawn
#define NEAR_Z 16.0f
// Smaller than this, in pixels across, and a ship is just a dot
#define DOT_SIZE 6.0f
// Smaller than this and a ship has no dark outlines when outlined
#define OUTLINE_SIZE 40.0f
// Smaller than this and a wireframe is just its outline
#define DETAIL_SIZE 32.0f
// The view, a little shorter than the screen to leave room for the text
// The view, between the text at the top and the dashboard
#define VIEW_TOP 36
#define VIEW_BOTTOM 332
#define CENTRE_X (GAME_SCREEN_W / 2)
#define CENTRE_Y ((VIEW_TOP + VIEW_BOTTOM) / 2)
// The fixed point the renderer uses for the x of the edges and shapes
#define FX SPRITE_PARTS_FX
// How long each way of drawing is shown for
#define STYLE_FRAMES (10 * 60)
// The system clock runs at the HSTX clock, 10 times the pixel clock, for the meter
#define LINE_CYCLES ((MODE_H_FRONT_PORCH + MODE_H_SYNC_WIDTH + MODE_H_BACK_PORCH + MODE_H_ACTIVE_PIXELS) * 10)
#define FRAME_LINES (MODE_V_FRONT_PORCH + MODE_V_SYNC_WIDTH + MODE_V_BACK_PORCH + MODE_V_ACTIVE_LINES)

typedef enum {
	STYLE_SOLID = 0,
	STYLE_WIRE,
	STYLE_OUTLINED,
	STYLE_FILLED,
	STYLE_COUNT
} Style_e;

typedef enum {
	SCREEN_SPACE = 0,
	SCREEN_GALAXY,
	SCREEN_LOCAL,
	SCREEN_DATA
} Screen_e;

// What is shown in turn: the view of space or a chart, and the way of
// drawing it, the faces shaded with or without dithering
typedef struct {
	const char* name;
	Screen_e screen;
	Style_e style;
	bool dither;
} Mode_t;

static const Mode_t _modes[] = {
	{ "GALACTIC CHART", SCREEN_GALAXY, STYLE_SOLID, false },
	{ "SHORT RANGE CHART", SCREEN_LOCAL, STYLE_SOLID, false },
	{ "SHORT RANGE FILLED", SCREEN_LOCAL, STYLE_FILLED, false },
	{ "DATA ON SYSTEM", SCREEN_DATA, STYLE_SOLID, false },
	{ "SOLID WIREFRAME", SCREEN_SPACE, STYLE_SOLID, false },
	{ "WIREFRAME", SCREEN_SPACE, STYLE_WIRE, false },
	{ "OUTLINED", SCREEN_SPACE, STYLE_OUTLINED, false },
	{ "OUTLINED DITHERED", SCREEN_SPACE, STYLE_OUTLINED, true },
	{ "FILLED", SCREEN_SPACE, STYLE_FILLED, false },
	{ "FILLED DITHERED", SCREEN_SPACE, STYLE_FILLED, true },
};
#define MODE_COUNT (sizeof(_modes) / sizeof(_modes[0]))

// Whether the faces are dithered this frame
static bool _dither;

// The colours on a model, as in art/ship_model.py
enum {
	PART_HULL = 0,
	PART_ENGINE,
	PART_VENT,
	PART_TRIM,
	PART_COUNT
};

static const uint8_t _part_rgb[PART_COUNT][3] = {
	{ 0, 0, 0 },       // The hull is the object's own colour
	{ 255, 146, 0 },   // Engines and fins
	{ 73, 73, 109 },   // Vents
	{ 0, 219, 255 },   // The gun and docking slot
};

typedef struct {
	float x, y, z;
} Vec3_t;

typedef struct {
	float x, y;
} Vec2_t;

typedef struct {
	const WireModel_t* model;
	Vec3_t pos;
	float yaw, pitch, roll;
	uint8_t rgb[3];
} Object_t;

typedef struct {
	float x, y, z;
} Star_t;

// The renderer on core 1 reads the objects of the frame it is drawing while
// this core works out the next, so there are two sets
static SpriteParts_t _wire_objects[2][MAX_OBJECTS];
static SpritePart_t _prims[2][MAX_PRIMS];
static SpriteBand_t _bands[2][MAX_ALL_BANDS];
static uint32_t _set = 0;
static uint32_t _object_count = 0;
static uint32_t _prim_count = 0;
static uint32_t _band_count = 0;
static uint32_t _sprite_count = 0;
// The object being made, and its box on the screen
static SpriteParts_t* _object = NULL;
static int32_t _box_top, _box_bottom, _box_left, _box_right;

// Not const so they live in RAM; the renderer reads them every row
static hstx_dvi_pixel_t _star_colours[3];

// The sun, far away, as a circle with rays round it
#define SUN_X 5200.0f
#define SUN_Y 1500.0f
#define SUN_Z 9000.0f
#define SUN_RADIUS 1600.0f
#define SUN_SIDES 24
#define SUN_RAYS 16
#define SUN_RAY_GAP 1.2f
#define SUN_RAY_LONG 1.65f
#define SUN_RAY_SHORT 1.4f

enum {
	OBJ_STATION = 0,
	OBJ_SHIP,
	OBJ_MISSILE = OBJ_SHIP + MODEL_SHIP_COUNT,
	OBJECTS = OBJ_MISSILE + 2
};
static Object_t _objects[OBJECTS];
static Star_t _stars[STARS];

static GameText_t _title[2];
static GameText_t _status[2];

// Towards the light, up and to the left behind us
static Vec3_t _light;

// Starts an object, returning false if there is no room
static bool object_begin(void) {
	if (_object_count >= MAX_OBJECTS) return false;
	_object = &_wire_objects[_set][_object_count];
	_object->part = &_prims[_set][_prim_count];
	_object->count = 0;
	_box_top = GAME_SCREEN_H;
	_box_bottom = 0;
	_box_left = GAME_SCREEN_W;
	_box_right = 0;
	return true;
}

// Adds a sprite for the object, if any of it is on the screen
static void object_end(void) {
	if (!_object) return;
	if (_object->count) {
		_object->top = _box_top;
		if (game_spr_add(_box_left, _box_top, _box_right - _box_left + 1, _box_bottom - _box_top, _object, NULL, sprite_renderer_parts)) {
			++_object_count;
			++_sprite_count;
		}
	}
	_object = NULL;
}

// Adds a line or a shape to the object, returning NULL if there is no room
static SpritePart_t* object_add(int32_t top, int32_t rows, int32_t left, int32_t right) {
	if (!_object || _prim_count >= MAX_PRIMS) return NULL;
	SpritePart_t* p = &_prims[_set][_prim_count++];
	++_object->count;
	p->top = (int16_t)top;
	p->rows = (int16_t)rows;
	if (top < _box_top) _box_top = top;
	if (top + rows > _box_bottom) _box_bottom = top + rows;
	if (left < _box_left) _box_left = left;
	if (right > _box_right) _box_right = right;
	return p;
}

static hstx_dvi_pixel_t shade(const uint8_t rgb[3], float k) {
	if (k > 1.0f) k = 1.0f;
	return hstx_dvi_pixel_rgb((uint8_t)(rgb[0] * k), (uint8_t)(rgb[1] * k), (uint8_t)(rgb[2] * k));
}

// The colour of a lit face, dithered for the shades between the screen's colours if asked
static SpriteColour_t face_colour(const uint8_t rgb[3], float k) {
	if (!_dither) return sprite_colour(shade(rgb, k));
	if (k > 1.0f) k = 1.0f;
	return sprite_colour_dither((uint8_t)(rgb[0] * k + 0.5f), (uint8_t)(rgb[1] * k + 0.5f), (uint8_t)(rgb[2] * k + 0.5f));
}

static Vec2_t project(const Vec3_t* v) {
	return (Vec2_t){ CENTRE_X + FOCAL * v->x / v->z, CENTRE_Y - FOCAL * v->y / v->z };
}

// The rows lines and shapes are cut to: the view, or the whole screen for the HUD
static int32_t _clip_top = VIEW_TOP;
static int32_t _clip_bottom = VIEW_BOTTOM;

// Cuts a line down to the part inside the view, returning false if none of it is
static bool clip_line(Vec2_t* a, Vec2_t* b) {
	const float dx = b->x - a->x;
	const float dy = b->y - a->y;
	const float p[4] = { -dx, dx, -dy, dy };
	const float q[4] = {
		a->x,
		(float)(GAME_SCREEN_W - 1) - a->x,
		a->y - (float)_clip_top,
		(float)(_clip_bottom - 1) - a->y
	};
	float t0 = 0.0f, t1 = 1.0f;
	for (uint32_t i = 0; i < 4; ++i) {
		if (p[i] == 0.0f) {
			if (q[i] < 0.0f) return false;
			continue;
		}
		const float t = q[i] / p[i];
		if (p[i] < 0.0f) {
			if (t > t1) return false;
			if (t > t0) t0 = t;
		}
		else {
			if (t < t0) return false;
			if (t < t1) t1 = t;
		}
	}
	const Vec2_t o = *a;
	*a = (Vec2_t){ o.x + dx * t0, o.y + dy * t0 };
	*b = (Vec2_t){ o.x + dx * t1, o.y + dy * t1 };
	return true;
}

// Adds a line on the screen to the object
static void add_line(Vec2_t p0, Vec2_t p1, hstx_dvi_pixel_t colour) {
	if (!clip_line(&p0, &p1)) return;
	// Top to bottom
	if (p1.y < p0.y) {
		const Vec2_t t = p0;
		p0 = p1;
		p1 = t;
	}
	const int32_t top = (int32_t)lroundf(p0.y);
	const int32_t rows = (int32_t)lroundf(p1.y) - top + 1;
	SpritePart_t* p = object_add(top, rows, (int32_t)fminf(p0.x, p1.x), (int32_t)fmaxf(p0.x, p1.x) + 1);
	if (!p) return;
	p->is_shape = 0;
	p->colour = sprite_colour(colour);
	p->line.x0 = (int32_t)(p0.x * (1 << FX));
	p->line.x1 = (int32_t)(p1.x * (1 << FX));
	p->line.dxdy = rows > 1 ? (p->line.x1 - p->line.x0) / (rows - 1) : 0;
	p->line.half = p->line.dxdy / 2;
}

// Adds an edge between two corners to the object, cutting off the part behind us
static void add_edge(Vec3_t a, Vec3_t b, hstx_dvi_pixel_t colour) {
	if (a.z < NEAR_Z && b.z < NEAR_Z) return;
	if (a.z < NEAR_Z || b.z < NEAR_Z) {
		const float t = (NEAR_Z - a.z) / (b.z - a.z);
		const Vec3_t c = { a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, NEAR_Z };
		if (a.z < NEAR_Z) a = c; else b = c;
	}
	add_line(project(&a), project(&b), colour);
}

// Keeps the part of a shape on the side of a line across the screen where
// the axis (0 for x, 1 for y) is at least bound, or at most if below is set
static uint32_t clip_side(const Vec2_t* in, uint32_t n, Vec2_t* out, uint32_t axis, float bound, bool below) {
	uint32_t m = 0;
	for (uint32_t i = 0; i < n; ++i) {
		const Vec2_t* a = &in[i];
		const Vec2_t* b = &in[(i + 1) % n];
		const float da = (axis ? a->y : a->x) - bound;
		const float db = (axis ? b->y : b->x) - bound;
		const bool ia = below ? da <= 0.0f : da >= 0.0f;
		const bool ib = below ? db <= 0.0f : db >= 0.0f;
		if (ia) out[m++] = *a;
		if (ia != ib) {
			const float t = da / (da - db);
			out[m++] = (Vec2_t){ a->x + (b->x - a->x) * t, a->y + (b->y - a->y) * t };
		}
	}
	return m;
}

// Sets up the band of a shape between rows r0 and r1, with sides a0 to a1 and b0 to b1
static void set_band(SpriteBand_t* k, int32_t r0, int32_t r1, const Vec2_t* a0, const Vec2_t* a1, const Vec2_t* b0, const Vec2_t* b1) {
	const float sa = (a1->x - a0->x) / (a1->y - a0->y);
	const float sb = (b1->x - b0->x) / (b1->y - b0->y);
	const float yc = (float)r0 + 0.5f;
	float xa = a0->x + (yc - a0->y) * sa;
	float xb = b0->x + (yc - b0->y) * sb;
	// Whichever side is to the left half way down
	const float h = (float)(r1 - r0 - 1) * 0.5f;
	float sl = sa, sr = sb;
	if (xa + sa * h > xb + sb * h) {
		float t = xa; xa = xb; xb = t;
		t = sl; sl = sr; sr = t;
	}
	k->xl = (int32_t)((xa - 0.5f) * (1 << FX));
	k->dxl = (int32_t)(sl * (1 << FX));
	k->xr = (int32_t)((xb - 0.5f) * (1 << FX));
	k->dxr = (int32_t)(sr * (1 << FX));
}

// Adds a convex shape on the screen to the object, its corners in order round it
static void add_fill(const Vec2_t* corners, uint32_t n, SpriteColour_t colour) {
	if (n < 3 || n > MAX_POLY) return;
	// Cut to the view
	Vec2_t p[MAX_POLY + 4], q[MAX_POLY + 4];
	uint32_t m = clip_side(corners, n, q, 0, 0.0f, false);
	m = clip_side(q, m, p, 0, (float)GAME_SCREEN_W, true);
	m = clip_side(p, m, q, 1, (float)_clip_top, false);
	m = clip_side(q, m, p, 1, (float)_clip_bottom, true);
	if (m < 3 || _band_count + m > MAX_ALL_BANDS) return;

	// Walk down both sides from the top corner, a band at a time
	uint32_t top = 0, bottom = 0;
	for (uint32_t i = 1; i < m; ++i) {
		if (p[i].y < p[top].y) top = i;
		if (p[i].y > p[bottom].y) bottom = i;
	}
	SpriteBand_t* bands = &_bands[_set][_band_count];
	uint32_t count = 0;
	int32_t first = 0, last = 0;
	uint32_t a = top, na = (top + 1) % m;
	uint32_t b = top, nb = (top + m - 1) % m;
	float y = p[top].y;
	for (uint32_t guard = 0; guard < 2 * m && y < p[bottom].y; ++guard) {
		const float ny = fminf(p[na].y, p[nb].y);
		// The rows whose middles are in the band
		const int32_t r0 = (int32_t)ceilf(y - 0.5f);
		const int32_t r1 = (int32_t)ceilf(ny - 0.5f);
		if (r1 > r0) {
			if (!count) first = r0;
			last = r1;
			SpriteBand_t* k = &bands[count++];
			set_band(k, r0, r1, &p[a], &p[na], &p[b], &p[nb]);
			k->start = r0 - first;
		}
		if (p[na].y <= ny) {
			a = na;
			na = (na + 1) % m;
		}
		if (p[nb].y <= ny) {
			b = nb;
			nb = (nb + m - 1) % m;
		}
		y = ny;
	}
	if (!count) return;
	float left = (float)GAME_SCREEN_W, right = 0.0f;
	for (uint32_t i = 0; i < m; ++i) {
		left = fminf(left, p[i].x);
		right = fmaxf(right, p[i].x);
	}
	SpritePart_t* f = object_add(first, last - first, (int32_t)floorf(left), (int32_t)ceilf(right));
	if (!f) return;
	_band_count += count;
	f->is_shape = 1;
	f->colour = colour;
	f->shape.band = bands;
	f->shape.count = (uint16_t)count;
	f->shape.cur = 0;
}

// Adds a flat convex shape to the object, cutting off the part behind us
static void add_shape(const Vec3_t* corners, uint32_t n, SpriteColour_t colour) {
	Vec2_t p[MAX_POLY];
	uint32_t m = 0;
	for (uint32_t i = 0; i < n && m + 2 <= MAX_POLY; ++i) {
		const Vec3_t* a = &corners[i];
		const Vec3_t* b = &corners[(i + 1) % n];
		if (a->z >= NEAR_Z) p[m++] = project(a);
		if ((a->z >= NEAR_Z) != (b->z >= NEAR_Z)) {
			const float t = (NEAR_Z - a->z) / (b->z - a->z);
			const Vec3_t c = { a->x + (b->x - a->x) * t, a->y + (b->y - a->y) * t, NEAR_Z };
			p[m++] = project(&c);
		}
	}
	add_fill(p, m, colour);
}

static float turn(const Vec2_t* o, const Vec2_t* a, const Vec2_t* b) {
	return (a->x - o->x) * (b->y - o->y) - (a->y - o->y) * (b->x - o->x);
}

// The outline round some points, in order, from Andrew's monotone chain. Returns how many corners it has.
static uint32_t outline(Vec2_t* p, uint32_t n, Vec2_t* out) {
	for (uint32_t i = 1; i < n; ++i) {
		const Vec2_t v = p[i];
		uint32_t j = i;
		for (; j > 0 && (p[j - 1].x > v.x || (p[j - 1].x == v.x && p[j - 1].y > v.y)); --j) p[j] = p[j - 1];
		p[j] = v;
	}
	uint32_t k = 0;
	for (uint32_t i = 0; i < n; ++i) {
		while (k >= 2 && turn(&out[k - 2], &out[k - 1], &p[i]) <= 0.0f) --k;
		out[k++] = p[i];
	}
	for (int32_t i = (int32_t)n - 2, t = k + 1; i >= 0; --i) {
		while (k >= (uint32_t)t && turn(&out[k - 2], &out[k - 1], &p[i]) <= 0.0f) --k;
		out[k++] = p[i];
	}
	return k - 1;
}

static Vec3_t rotate(const Vec3_t* v, const float m[3][3]) {
	return (Vec3_t){
		m[0][0] * v->x + m[0][1] * v->y + m[0][2] * v->z,
		m[1][0] * v->x + m[1][1] * v->y + m[1][2] * v->z,
		m[2][0] * v->x + m[2][1] * v->y + m[2][2] * v->z,
	};
}

// Roll about z, then pitch about x, then yaw about y
static void make_matrix(float m[3][3], float yaw, float pitch, float roll) {
	const float cy = cosf(yaw), sy = sinf(yaw);
	const float cp = cosf(pitch), sp = sinf(pitch);
	const float cr = cosf(roll), sr = sinf(roll);
	m[0][0] = cy * cr + sy * sp * sr;
	m[0][1] = -cy * sr + sy * sp * cr;
	m[0][2] = sy * cp;
	m[1][0] = cp * sr;
	m[1][1] = cp * cr;
	m[1][2] = -sp;
	m[2][0] = -sy * cr + cy * sp * sr;
	m[2][1] = sy * sr + cy * sp * cr;
	m[2][2] = cy * cp;
}

// The outlines of the solid objects drawn so far this frame, nearest first,
// for leaving out those behind them
#define MAX_OCCLUDERS 8
#define MAX_OCCLUDER_SIDES 24
// Smaller than this, in pixels across, and an object hides too little to look at
#define OCCLUDER_SIZE 24.0f

typedef struct {
	Vec2_t p[MAX_OCCLUDER_SIDES];
	uint32_t n;
	float turn; // 1 or -1, for the side of each edge that is inside
} Occluder_t;

static Occluder_t _occluders[MAX_OCCLUDERS];
static uint32_t _occluder_count = 0;
static uint32_t _hidden_count = 0;

static void add_occluder(const Vec2_t* p, uint32_t n) {
	if (_occluder_count >= MAX_OCCLUDERS || n < 3 || n > MAX_OCCLUDER_SIDES) return;
	Occluder_t* o = &_occluders[_occluder_count++];
	float area = 0.0f;
	for (uint32_t i = 0; i < n; ++i) {
		o->p[i] = p[i];
		const Vec2_t* b = &p[(i + 1) % n];
		area += p[i].x * b->y - b->x * p[i].y;
	}
	o->n = n;
	o->turn = area < 0.0f ? -1.0f : 1.0f;
}

// Whether a circle on the screen is wholly inside the outline of something nearer
static bool hidden(Vec2_t c, float r) {
	for (uint32_t k = 0; k < _occluder_count; ++k) {
		const Occluder_t* o = &_occluders[k];
		uint32_t i = 0;
		for (; i < o->n; ++i) {
			const Vec2_t* a = &o->p[i];
			const Vec2_t* b = &o->p[(i + 1) % o->n];
			const float ex = b->x - a->x, ey = b->y - a->y;
			// How far inside this edge the middle is, times the edge's length
			const float d = o->turn * (ex * (c.y - a->y) - ey * (c.x - a->x));
			if (d < r * sqrtf(ex * ex + ey * ey)) break;
		}
		if (i == o->n) return true;
	}
	return false;
}

static void draw_model(const Object_t* o, Style_e style) {
	const WireModel_t* model = o->model;
	if (o->pos.z < -model->radius) return;
	const Vec2_t centre = project(&o->pos);
	// Wholly behind something solid and nearer, it is left out
	if (style != STYLE_WIRE && o->pos.z - model->radius > NEAR_Z &&
		hidden(centre, FOCAL * model->radius / (o->pos.z - model->radius) + 1.0f)) {
		++_hidden_count;
		return;
	}
	// Far away it is just a dot
	const float size = o->pos.z > model->radius ? FOCAL * model->radius * 2.0f / o->pos.z : (float)GAME_SCREEN_W;
	if (size < DOT_SIZE) {
		add_line(centre, (Vec2_t){ centre.x + 1.0f, centre.y + 1.0f }, shade(o->rgb, 1.0f));
		return;
	}

	float m[3][3];
	make_matrix(m, o->yaw, o->pitch, o->roll);

	// Each corner relative to us
	Vec3_t cam[MAX_VERTS];
	bool behind = false;
	for (uint32_t i = 0; i < model->nv; ++i) {
		const Vec3_t v = { model->v[i].x, model->v[i].y, model->v[i].z };
		const Vec3_t r = rotate(&v, m);
		cam[i] = (Vec3_t){ r.x + o->pos.x, r.y + o->pos.y, r.z + o->pos.z };
		if (i < model->nh && cam[i].z < NEAR_Z) behind = true;
	}

	// The outline on the screen, which a solid object hides everything behind
	Vec2_t hull[2 * MAX_VERTS];
	uint32_t hull_n = 0;
	if (!behind) {
		Vec2_t p[MAX_VERTS];
		for (uint32_t i = 0; i < model->nh; ++i) p[i] = project(&cam[i]);
		hull_n = outline(p, model->nh, hull);
		if (style != STYLE_WIRE && size >= OCCLUDER_SIZE) add_occluder(hull, hull_n);
	}

	// Small and far away, a wireframe is just its outline. A convex outline
	// crosses each row twice, where all the edges can cross it many times,
	// and each costs about as much on a row however short it is.
	if (!behind && size < DETAIL_SIZE && (style == STYLE_SOLID || style == STYLE_WIRE)) {
		const hstx_dvi_pixel_t c = shade(o->rgb, 1.0f);
		for (uint32_t i = 0; i < hull_n; ++i) add_line(hull[i], hull[(i + 1) % hull_n], c);
		if (style == STYLE_SOLID) add_fill(hull, hull_n, sprite_colour(hstx_dvi_pixel_rgb(0, 0, 0)));
		return;
	}

	// A face is facing us if we are in front of it, and lit by how much it faces the light
	bool facing[MAX_FACES];
	float light[MAX_FACES];
	for (uint32_t i = 0; i < model->nf; ++i) {
		const WireFace_t* f = &model->f[i];
		const Vec3_t n = rotate(&(Vec3_t){ f->nx, f->ny, f->nz }, m);
		const Vec3_t* c = &cam[f->v[0]];
		facing[i] = n.x * c->x + n.y * c->y + n.z * c->z < 0.0f;
		light[i] = fmaxf(0.0f, n.x * _light.x + n.y * _light.y + n.z * _light.z);
	}

	// The edges go first so they are drawn over the faces
	for (uint32_t i = 0; i < model->ne; ++i) {
		const WireEdge_t* e = &model->e[i];
		if (!facing[e->f0] && !facing[e->f1]) continue;
		const uint8_t* rgb = e->colour == PART_HULL ? o->rgb : _part_rgb[e->colour];
		hstx_dvi_pixel_t colour;
		if (style == STYLE_SOLID || style == STYLE_WIRE || e->colour == PART_TRIM) {
			colour = shade(rgb, 1.0f);
		}
		else if (style == STYLE_OUTLINED && e->colour == PART_HULL && size >= OUTLINE_SIZE && facing[e->f0] && facing[e->f1]) {
			// Only between faces we can see, as dark edges round the outside
			// hardly show against space. Each edge costs about as much on a
			// row however short it is, so small ships have none.
			colour = shade(rgb, 0.25f);
		}
		else {
			continue;
		}
		add_edge(cam[e->v0], cam[e->v1], colour);
	}

	Vec3_t corners[WIRE_MAX_LOOP];
	if (style == STYLE_SOLID) {
		const SpriteColour_t black = sprite_colour(hstx_dvi_pixel_rgb(0, 0, 0));
		if (!behind) {
			// The outline of the hull, as one shape
			add_fill(hull, hull_n, black);
		}
		else {
			// Close up the outline is hard to find, so each face
			for (uint32_t i = 0; i < model->nf; ++i) {
				const WireFace_t* f = &model->f[i];
				if (!facing[i]) continue;
				for (uint32_t j = 0; j < f->n; ++j) corners[j] = cam[f->v[j]];
				add_shape(corners, f->n, black);
			}
		}
		return;
	}
	if (style == STYLE_WIRE) return;

	// The shapes on the faces, so they are drawn over the faces they sit on
	for (uint32_t i = 0; i < model->np; ++i) {
		const WirePoly_t* p = &model->p[i];
		if (!facing[p->f]) continue;
		for (uint32_t j = 0; j < p->n; ++j) corners[j] = cam[p->v[j]];
		add_shape(corners, p->n, face_colour(_part_rgb[p->colour], 0.5f + 0.5f * light[p->f]));
	}
	for (uint32_t i = 0; i < model->nf; ++i) {
		const WireFace_t* f = &model->f[i];
		if (!facing[i]) continue;
		for (uint32_t j = 0; j < f->n; ++j) corners[j] = cam[f->v[j]];
		add_shape(corners, f->n, face_colour(o->rgb, 0.3f + 0.7f * light[i]));
	}
}

// Adds the object's sprite
static void draw_object(const Object_t* o, Style_e style) {
	if (!object_begin()) return;
	draw_model(o, style);
	object_end();
}

static float rand_range(float lo, float hi) {
	return lo + (hi - lo) * (float)(rand() & 0xffff) / 65535.0f;
}

static void new_star(Star_t* s, float z) {
	s->x = rand_range(-1600.0f, 1600.0f);
	s->y = rand_range(-1200.0f, 1200.0f);
	s->z = z;
}

// Stars streaming past, brighter as they come closer. They draw straight
// into the row and are added before the ships, so the ships cover them.
static void draw_stars(void) {
	for (uint32_t i = 0; i < STARS; ++i) {
		Star_t* s = &_stars[i];
		s->z -= 6.0f;
		if (s->z < NEAR_Z) new_star(s, 2000.0f);
		const int32_t x = (int32_t)(CENTRE_X + FOCAL * s->x / s->z);
		const int32_t y = (int32_t)(CENTRE_Y - FOCAL * s->y / s->z);
		if (x < 0 || x >= GAME_SCREEN_W - 2 || y < VIEW_TOP || y >= VIEW_BOTTOM) {
			new_star(s, rand_range(1000.0f, 2000.0f));
			continue;
		}
		const uint32_t b = s->z < 400.0f ? 2 : s->z < 1000.0f ? 1 : 0;
		game_spr_hline(x, y, b == 2 ? 2 : 1, &_star_colours[b]);
		++_sprite_count;
	}
}

// The station spins slowly with its slot towards us
#define STATION_Z 1700.0f
// The ships fly round it in a loop that comes close to us
#define LOOP_RADIUS 1300.0f
#define LOOP_SPEED 0.12f
// How long a missile is behind the ship it is chasing, in seconds
#define MISSILE_LAG 1.2f

// Where ship i is at time t, spaced out round the loop at its own height
static Vec3_t ship_path(uint32_t i, float t) {
	const float a = t * LOOP_SPEED + 2.0f * 3.1416f * (float)i / MODEL_SHIP_COUNT;
	const float r = LOOP_RADIUS + 120.0f * sinf((float)i * 1.3f);
	return (Vec3_t){
		r * sinf(a),
		150.0f * sinf((float)i * 2.4f) + 30.0f * sinf(t * 0.5f + (float)i),
		STATION_Z - r * cosf(a)
	};
}

// Points an object along the way it is going, banked by roll
static void face_along(Object_t* o, const Vec3_t* from, const Vec3_t* to, float roll) {
	const float vx = to->x - from->x, vy = to->y - from->y, vz = to->z - from->z;
	o->yaw = atan2f(vx, vz);
	o->pitch = -atan2f(vy, sqrtf(vx * vx + vz * vz));
	o->roll = roll;
}

static void move_objects(uint32_t frame) {
	const float t = (float)frame / 60.0f;
	Object_t* o = &_objects[OBJ_STATION];
	o->pos = (Vec3_t){ 0.0f, 0.0f, STATION_Z };
	o->yaw = 3.1416f;
	o->pitch = 0.15f;
	o->roll = t * 0.3f;

	for (uint32_t i = 0; i < MODEL_SHIP_COUNT; ++i) {
		o = &_objects[OBJ_SHIP + i];
		o->pos = ship_path(i, t);
		const Vec3_t next = ship_path(i, t + 0.1f);
		// Banked into the turn, and rocking a little
		face_along(o, &o->pos, &next, 0.35f + 0.25f * sinf(t * 0.8f + (float)i));
	}

	// Each missile chases a ship, a little behind it and weaving
	for (uint32_t i = 0; i < 2; ++i) {
		o = &_objects[OBJ_MISSILE + i];
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

// Adds the sun, after everything else so it is behind them all bar the stars.
// It is an outline with rays that turn slowly and flicker, made of parts like
// the ships, filled in black for the solid wireframes and yellow when filled.
static void draw_sun(Style_e style, uint32_t frame) {
	const Vec3_t c = { SUN_X, SUN_Y, SUN_Z };
	const Vec2_t m = project(&c);
	const float rp = FOCAL * SUN_RADIUS / SUN_Z;
	if (style != STYLE_WIRE && hidden(m, rp * (SUN_RAY_LONG + 0.15f) + 1.0f)) return;
	const hstx_dvi_pixel_t edge = hstx_dvi_pixel_rgb(255, 255, 0);
	const hstx_dvi_pixel_t ray = hstx_dvi_pixel_rgb(255, 146, 0);
	Vec2_t outline[SUN_SIDES];
	for (uint32_t i = 0; i < SUN_SIDES; ++i) {
		const float a = 2.0f * 3.1416f * (float)i / SUN_SIDES;
		outline[i] = (Vec2_t){ m.x + rp * cosf(a), m.y + rp * sinf(a) };
	}
	// It is in a few objects, as an object's sprite looks at all its parts on
	// every row it covers: each half of the outline, then what fills it
	for (uint32_t h = 0; h < 2; ++h) {
		if (!object_begin()) return;
		for (uint32_t i = 0; i < SUN_SIDES / 2; ++i) {
			const uint32_t k = i + h * SUN_SIDES / 2;
			add_line(outline[k], outline[(k + 1) % SUN_SIDES], edge);
		}
		object_end();
	}
	if (style != STYLE_WIRE && object_begin()) {
		add_fill(outline, SUN_SIDES, sprite_colour(style == STYLE_SOLID ? hstx_dvi_pixel_rgb(0, 0, 0) : edge));
		object_end();
	}
	// And each ray
	const float turn = (float)frame * 0.004f;
	for (uint32_t i = 0; i < SUN_RAYS; ++i) {
		if (!object_begin()) return;
		const float a = turn + 2.0f * 3.1416f * (float)i / SUN_RAYS;
		// Its own flicker, so the stars do not change when the sun is hidden
		const uint32_t h = (frame * 2654435761u + i * 40503u) >> 16;
		const float len = (i & 1 ? SUN_RAY_SHORT : SUN_RAY_LONG) + 0.06f * (float)(h % 3);
		const float ca = cosf(a), sa = sinf(a);
		add_line(
			(Vec2_t){ m.x + rp * SUN_RAY_GAP * ca, m.y + rp * SUN_RAY_GAP * sa },
			(Vec2_t){ m.x + rp * len * ca, m.y + rp * len * sa }, ray);
		object_end();
	}
}


// ----------------------------------------------------------------------------
// The dashboard under the view, after the original game: gauges either side
// of the scanner, which shows where the ships are round us, and a compass
// pointing to the station. It is drawn in outlines for the wireframe styles,
// and solid for the filled ones.
//
// Each part of it is an object of its own, as an object's sprite looks at all
// its parts on every row it covers.

#define DASH_TOP 336
#define DASH_BOTTOM 477
#define GAUGE_ROWS 7
#define GAUGE_X0 32
#define GAUGE_X1 168
#define GAUGE_RIGHT_X0 492
#define GAUGE_RIGHT_X1 628
#define GAUGE_H 9
#define SCANNER_X 320
#define SCANNER_Y 410
#define SCANNER_RX 112.0f
#define SCANNER_RY 46.0f
#define SCANNER_SIDES 24
// How far the scanner sees
#define SCANNER_RANGE 3200.0f
#define COMPASS_X 438
#define COMPASS_Y 356
#define COMPASS_R 12.0f
#define MISSILES 4

typedef enum {
	GAUGE_FS = 0, GAUGE_AS, GAUGE_FV, GAUGE_CT, GAUGE_LT, GAUGE_AL,
	GAUGE_SP, GAUGE_RL, GAUGE_DC, GAUGE_E1, GAUGE_E2, GAUGE_E3, GAUGE_E4,
	GAUGES
} Gauge_e;

static const char* const _gauge_names[GAUGES] = {
	"FS", "AS", "FV", "CT", "LT", "AL", "SP", "RL", "DC", "1", "2", "3", "4"
};
static GameText_t _gauge_labels[GAUGES];

#define PANELS 3
static const float _panels[PANELS][2] = { { 4, 178 }, { 184, 456 }, { 462, 636 } };

static hstx_dvi_pixel_t hud_rgb(uint8_t r, uint8_t g, uint8_t b) {
	return hstx_dvi_pixel_rgb(r, g, b);
}

static void hud_box(float x0, float y0, float x1, float y1, hstx_dvi_pixel_t c) {
	add_line((Vec2_t){ x0, y0 }, (Vec2_t){ x1, y0 }, c);
	add_line((Vec2_t){ x1, y0 }, (Vec2_t){ x1, y1 }, c);
	add_line((Vec2_t){ x1, y1 }, (Vec2_t){ x0, y1 }, c);
	add_line((Vec2_t){ x0, y1 }, (Vec2_t){ x0, y0 }, c);
}

// Fills the pixels from x0 up to x1 and y0 up to y1
static void hud_rect(float x0, float y0, float x1, float y1, hstx_dvi_pixel_t c) {
	const Vec2_t r[4] = { { x0, y0 }, { x1, y0 }, { x1, y1 }, { x0, y1 } };
	add_fill(r, 4, sprite_colour(c));
}

// A box that is filled or outlined, to suit the style
static void hud_block(bool filled, float x0, float y0, float x1, float y1, hstx_dvi_pixel_t c) {
	if (filled) hud_rect(x0, y0, x1 + 1.0f, y1 + 1.0f, c);
	else hud_box(x0, y0, x1, y1, c);
}

static void hud_ellipse(Vec2_t* p, uint32_t n, float cx, float cy, float rx, float ry) {
	for (uint32_t i = 0; i < n; ++i) {
		const float a = 2.0f * 3.1416f * (float)i / (float)n;
		p[i] = (Vec2_t){ cx + rx * cosf(a), cy + ry * sinf(a) };
	}
}

static float clamp01(float v) {
	return v < 0.0f ? 0.0f : v > 1.0f ? 1.0f : v;
}

// The y of the bar of a gauge
static float gauge_y(uint32_t row) {
	return (float)DASH_TOP + 6.0f + (float)row * ((float)(DASH_BOTTOM - DASH_TOP - 12) / GAUGE_ROWS);
}

// A bar, filled from the left to v, with a dim track for the whole length
static void hud_bar(bool filled, float x0, float x1, float y, float v, hstx_dvi_pixel_t c, hstx_dvi_pixel_t track) {
	if (!object_begin()) return;
	const float x = x0 + (x1 - x0) * clamp01(v);
	if (x > x0 + 1.0f) hud_block(filled, x0, y, x, y + GAUGE_H - 1, c);
	hud_box(x0, y, x1, y + GAUGE_H - 1, track);
	object_end();
}

// A marker on a track, from -1 at the left to 1 at the right
static void hud_marker(bool filled, float x0, float x1, float y, float v, hstx_dvi_pixel_t c, hstx_dvi_pixel_t track) {
	if (!object_begin()) return;
	const float m = x0 + (x1 - x0) * 0.5f * (clamp01(0.5f + 0.5f * v) * 2.0f);
	hud_block(filled, m - 3.0f, y, m + 3.0f, y + GAUGE_H - 1, c);
	add_line((Vec2_t){ (x0 + x1) * 0.5f, y }, (Vec2_t){ (x0 + x1) * 0.5f, y + GAUGE_H - 1 }, track);
	hud_box(x0, y, x1, y + GAUGE_H - 1, track);
	object_end();
}

// A ship on the scanner: a blob above or below where it is, on a stalk
static void hud_blip(const Object_t* o, uint32_t i) {
	const float x = o->pos.x / SCANNER_RANGE;
	const float z = o->pos.z / SCANNER_RANGE;
	if (x * x + z * z > 1.0f) return;
	const float sx = SCANNER_X + SCANNER_RX * x;
	const float sy = SCANNER_Y - SCANNER_RY * z;
	const float by = sy - o->pos.y / SCANNER_RANGE * SCANNER_RY * 2.0f;
	if (!object_begin()) return;
	const hstx_dvi_pixel_t c = shade(o->rgb, 1.0f);
	const float w = i == OBJ_STATION ? 3.0f : 2.0f;
	hud_rect(sx - w, by - 1.0f, sx + w, by + 1.0f, c);
	if (fabsf(by - sy) > 1.0f) add_line((Vec2_t){ sx, sy }, (Vec2_t){ sx, by }, c);
	object_end();
}

// The dashboard, with the sights in the middle of the view if asked
static void draw_hud(Style_e style, uint32_t frame, bool sights) {
	const bool filled = style == STYLE_OUTLINED || style == STYLE_FILLED;
	const float t = (float)frame / 60.0f;
	const hstx_dvi_pixel_t frame_c = hud_rgb(36, 109, 219);
	const hstx_dvi_pixel_t track = hud_rgb(36, 73, 109);
	const hstx_dvi_pixel_t scan = hud_rgb(0, 146, 182);
	_clip_top = 0;
	_clip_bottom = GAME_SCREEN_H;

	// The sights in the middle of the view
	if (sights && object_begin()) {
		const hstx_dvi_pixel_t c = hud_rgb(255, 255, 255);
		add_line((Vec2_t){ CENTRE_X - 26, CENTRE_Y }, (Vec2_t){ CENTRE_X - 8, CENTRE_Y }, c);
		add_line((Vec2_t){ CENTRE_X + 8, CENTRE_Y }, (Vec2_t){ CENTRE_X + 26, CENTRE_Y }, c);
		add_line((Vec2_t){ CENTRE_X, CENTRE_Y - 18 }, (Vec2_t){ CENTRE_X, CENTRE_Y - 6 }, c);
		add_line((Vec2_t){ CENTRE_X, CENTRE_Y + 6 }, (Vec2_t){ CENTRE_X, CENTRE_Y + 18 }, c);
		object_end();
	}

	// The gauges, with made up values that change over time
	const hstx_dvi_pixel_t yellow = hud_rgb(255, 255, 0);
	const hstx_dvi_pixel_t green = hud_rgb(0, 255, 0);
	const hstx_dvi_pixel_t red = hud_rgb(255, 73, 0);
	const float energy = 0.8f + 0.2f * sinf(t * 0.2f);
	const float left[6] = {
		0.75f + 0.25f * sinf(t * 0.7f),
		0.6f + 0.3f * sinf(t * 0.5f + 1.0f),
		0.85f,
		0.3f + 0.1f * sinf(t * 0.3f),
		0.15f + 0.6f * fmaxf(0.0f, sinf(t * 1.3f)),
		0.0f
	};
	const hstx_dvi_pixel_t left_c[6] = { yellow, yellow, green, red, red, green };
	for (uint32_t g = GAUGE_FS; g <= GAUGE_AL; ++g) {
		hud_bar(filled, GAUGE_X0, GAUGE_X1, gauge_y(g), left[g], left_c[g], track);
	}
	hud_bar(filled, GAUGE_RIGHT_X0, GAUGE_RIGHT_X1, gauge_y(0), 0.55f + 0.3f * sinf(t * 0.3f), yellow, track);
	hud_marker(filled, GAUGE_RIGHT_X0, GAUGE_RIGHT_X1, gauge_y(1), sinf(t * 0.4f), green, track);
	hud_marker(filled, GAUGE_RIGHT_X0, GAUGE_RIGHT_X1, gauge_y(2), 0.6f * sinf(t * 0.25f), green, track);
	for (uint32_t b = 0; b < 4; ++b) {
		// The banks empty from the top
		hud_bar(filled, GAUGE_RIGHT_X0, GAUGE_RIGHT_X1, gauge_y(3 + b), energy * 4.0f - (float)(3 - b), yellow, track);
	}

	// The ships on the scanner, over it
	for (uint32_t i = 0; i < OBJECTS; ++i) hud_blip(&_objects[i], i);

	// The compass, with a dot towards the station, red when it is behind us
	if (object_begin()) {
		const Vec3_t* s = &_objects[OBJ_STATION].pos;
		const float l = sqrtf(s->x * s->x + s->y * s->y + s->z * s->z);
		const float dx = COMPASS_X + COMPASS_R * s->x / l;
		const float dy = COMPASS_Y - COMPASS_R * s->y / l;
		hud_rect(dx - 2.0f, dy - 2.0f, dx + 2.0f, dy + 2.0f, s->z >= 0.0f ? green : red);
		Vec2_t c[16];
		hud_ellipse(c, 16, COMPASS_X, COMPASS_Y, COMPASS_R + 2.0f, COMPASS_R + 2.0f);
		for (uint32_t i = 0; i < 16; ++i) add_line(c[i], c[(i + 1) % 16], scan);
		object_end();
	}

	// The missiles, two of them away
	if (object_begin()) {
		for (uint32_t i = 0; i < MISSILES; ++i) {
			const float x = SCANNER_X - 112.0f + 14.0f * (float)i;
			if (i < 2) hud_box(x, DASH_BOTTOM - 14, x + 9, DASH_BOTTOM - 6, track);
			else hud_block(filled, x, DASH_BOTTOM - 14, x + 9, DASH_BOTTOM - 6, green);
		}
		object_end();
	}

	// The scanner: an ellipse, with lines showing ahead and to the sides
	if (object_begin()) {
		Vec2_t e[SCANNER_SIDES];
		hud_ellipse(e, SCANNER_SIDES, SCANNER_X, SCANNER_Y, SCANNER_RX, SCANNER_RY);
		for (uint32_t i = 0; i < SCANNER_SIDES; ++i) add_line(e[i], e[(i + 1) % SCANNER_SIDES], scan);
		add_line((Vec2_t){ SCANNER_X - SCANNER_RX, SCANNER_Y }, (Vec2_t){ SCANNER_X + SCANNER_RX, SCANNER_Y }, track);
		add_line((Vec2_t){ SCANNER_X, SCANNER_Y - SCANNER_RY }, (Vec2_t){ SCANNER_X, SCANNER_Y + SCANNER_RY }, track);
		add_line((Vec2_t){ SCANNER_X, SCANNER_Y }, (Vec2_t){ SCANNER_X - SCANNER_RX * 0.7f, SCANNER_Y - SCANNER_RY * 0.7f }, track);
		add_line((Vec2_t){ SCANNER_X, SCANNER_Y }, (Vec2_t){ SCANNER_X + SCANNER_RX * 0.7f, SCANNER_Y - SCANNER_RY * 0.7f }, track);
		object_end();
	}

	// The panels
	for (uint32_t i = 0; i < PANELS; ++i) {
		if (!object_begin()) break;
		hud_box(_panels[i][0], DASH_TOP, _panels[i][1], DASH_BOTTOM, frame_c);
		object_end();
	}

	// The labels. Text draws straight into the row, so it goes after
	// everything else to be drawn over it.
	for (uint32_t g = 0; g < GAUGES; ++g) {
		const bool right = g >= GAUGE_SP;
		const uint32_t row = right ? g - GAUGE_SP : g;
		game_spr_text(&_gauge_labels[g], right ? GAUGE_RIGHT_X0 - 22 : 8, (int32_t)gauge_y(row) + 1, game_pallet(GAME_CYAN));
		++_sprite_count;
	}

	_clip_top = VIEW_TOP;
	_clip_bottom = VIEW_BOTTOM;
}

// ----------------------------------------------------------------------------
// The charts of the galaxy, as in the original game

// The galaxy shown, and the system we are at in it, Lave in the first
#define HOME 7
static GalaxySystem_t _galaxy[GALAXY_SYSTEMS];
static uint32_t _galaxy_shown = GALAXIES;
// The systems from the top of the chart down
static uint8_t _by_y[GALAXY_SYSTEMS];
// The systems on the short range chart
static uint8_t _near[GALAXY_SYSTEMS];
static uint32_t _near_count;

// The galactic chart, twice as wide as it is tall as in the original
#define GALAXY_LEFT 64.0f
#define GALAXY_TOP ((float)VIEW_TOP + 22.0f)
// The stars are added in bands of this many rows, so each sprite looks at a few
#define GALAXY_BAND_ROWS 8
// The short range chart: pixels for each step of x across the galaxy, and
// how far from us it shows each way
#define LOCAL_SCALE 7.2f
#define LOCAL_DX 40
#define LOCAL_DY 38
// How far we can jump, in tenths of a light year
#define FUEL_RANGE 70
#define CHART_LABELS 32
#define CHART_INFO_Y (VIEW_BOTTOM - 10)
// The cursor moves to another system this often, taking this long to get there
#define CHART_STEP_FRAMES 90
#define CHART_MOVE_FRAMES 30

static GameText_t _chart_labels[2][CHART_LABELS];
static GameText_t _chart_info[2];

// The colour of a system on the short range chart, from rich industrial to poor agricultural
static const uint8_t _economy_rgb[8][3] = {
	{ 255, 73, 0 }, { 255, 146, 0 }, { 255, 219, 0 }, { 219, 255, 0 },
	{ 146, 255, 0 }, { 0, 255, 0 }, { 0, 219, 146 }, { 0, 182, 219 },
};

static void set_galaxy(uint32_t g) {
	if (g == _galaxy_shown) return;
	_galaxy_shown = g;
	galaxy_make(g, _galaxy);
	for (uint32_t i = 0; i < GALAXY_SYSTEMS; ++i) {
		uint32_t j = i;
		for (; j > 0 && _galaxy[_by_y[j - 1]].y > _galaxy[i].y; --j) _by_y[j] = _by_y[j - 1];
		_by_y[j] = (uint8_t)i;
	}
	const GalaxySystem_t* h = &_galaxy[HOME];
	_near_count = 0;
	for (uint32_t i = 0; i < GALAXY_SYSTEMS; ++i) {
		if (abs((int32_t)_galaxy[i].x - h->x) <= LOCAL_DX && abs((int32_t)_galaxy[i].y - h->y) <= LOCAL_DY) {
			_near[_near_count++] = (uint8_t)i;
		}
	}
}

// The system the cursor goes to at a step
static uint32_t chart_pick(bool local, uint32_t step) {
	const uint32_t h = (step * 2654435761u) >> 16;
	return local ? _near[h % _near_count] : h % GALAXY_SYSTEMS;
}

// Where the cursor is in the galaxy, moving from one system to the next.
// Returns the system it is going to.
static uint32_t chart_cursor(bool local, uint32_t frame, float* x, float* y) {
	const uint32_t step = frame / CHART_STEP_FRAMES;
	const GalaxySystem_t* a = &_galaxy[chart_pick(local, step - 1)];
	const uint32_t to = chart_pick(local, step);
	const GalaxySystem_t* b = &_galaxy[to];
	float t = (float)(frame % CHART_STEP_FRAMES) / (float)CHART_MOVE_FRAMES;
	if (t > 1.0f) t = 1.0f;
	t = t * t * (3.0f - 2.0f * t);
	*x = (float)a->x + ((float)b->x - (float)a->x) * t;
	*y = (float)a->y + ((float)b->y - (float)a->y) * t;
	return to;
}

static Vec2_t galaxy_point(float x, float y) {
	return (Vec2_t){ GALAXY_LEFT + 2.0f * x, GALAXY_TOP + y };
}

static Vec2_t local_point(float x, float y) {
	const GalaxySystem_t* h = &_galaxy[HOME];
	return (Vec2_t){ CENTRE_X + (x - (float)h->x) * LOCAL_SCALE, CENTRE_Y + (y - (float)h->y) * LOCAL_SCALE * 0.5f };
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
	hud_ellipse(p, sides, c.x, c.y, r, r);
	for (uint32_t i = 0; i < sides; ++i) add_line(p[i], p[(i + 1) % sides], colour);
}

// The name of the system the cursor is on, and how far it is from us
static void chart_info(GameText_t* info, uint32_t to) {
	const uint32_t d = galaxy_distance(&_galaxy[HOME], &_galaxy[to]);
	char s[GAME_TEXT_MAX];
	snprintf(s, sizeof(s), "%s %u.%u LIGHT YEARS", _galaxy[to].name, (unsigned)(d / 10), (unsigned)(d % 10));
	game_text_set(info, s, 1);
	game_spr_text_centre(info, CHART_INFO_Y, game_pallet(GAME_WHITE));
	++_sprite_count;
}

static void draw_galaxy_chart(uint32_t frame, GameText_t* info) {
	const hstx_dvi_pixel_t white = hstx_dvi_pixel_rgb(255, 255, 255);
	float cx, cy;
	const uint32_t to = chart_cursor(false, frame, &cx, &cy);

	// The cursor, and how far we can jump from where we are
	const GalaxySystem_t* h = &_galaxy[HOME];
	if (object_begin()) {
		chart_cross(galaxy_point(cx, cy), 3.0f, 10.0f, hstx_dvi_pixel_rgb(255, 255, 0));
		object_end();
	}
	if (object_begin()) {
		const Vec2_t c = galaxy_point((float)h->x, (float)h->y);
		chart_cross(c, 0.0f, 6.0f, white);
		// A jump is 4 tenths of a light year for each step across
		chart_ring(c, 2.0f * (float)FUEL_RANGE / 4.0f, 24, hstx_dvi_pixel_rgb(0, 255, 0));
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
		hud_rect(p.x, p.y, p.x + 2.0f, p.y + 2.0f, white);
	}
	object_end();

	if (object_begin()) {
		hud_box(GALAXY_LEFT - 6.0f, GALAXY_TOP - 6.0f, GALAXY_LEFT + 2.0f * 255.0f + 8.0f, GALAXY_TOP + 255.0f + 6.0f,
			hstx_dvi_pixel_rgb(36, 109, 219));
		object_end();
	}
	chart_info(info, to);
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

static void draw_local_chart(Style_e style, uint32_t frame, GameText_t* labels, GameText_t* info) {
	const bool filled = style == STYLE_OUTLINED || style == STYLE_FILLED;
	float cx, cy;
	const uint32_t to = chart_cursor(true, frame, &cx, &cy);
	const GalaxySystem_t* h = &_galaxy[HOME];
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
	float radius[GALAXY_SYSTEMS];
	ChartBox_t taken[GALAXY_SYSTEMS + CHART_LABELS];
	for (uint32_t i = 0; i < _near_count; ++i) {
		const GalaxySystem_t* s = &_galaxy[_near[i]];
		const Vec2_t c = local_point((float)s->x, (float)s->y);
		const float r = 3.0f + (float)(s->radius - 2816) / 512.0f;
		radius[i] = r;
		taken[i] = (ChartBox_t){ (int32_t)(c.x - r), (int32_t)(c.y - r), (int32_t)(c.x + r), (int32_t)(c.y + r) };
		if (!object_begin()) break;
		Vec2_t p[16];
		hud_ellipse(p, 16, c.x, c.y, r, r);
		const hstx_dvi_pixel_t colour = shade(_economy_rgb[s->economy], 1.0f);
		if (filled) {
			add_fill(p, 16, _dither ? sprite_colour_dither(_economy_rgb[s->economy][0], _economy_rgb[s->economy][1],
				_economy_rgb[s->economy][2]) : sprite_colour(colour));
		}
		else {
			for (uint32_t j = 0; j < 16; ++j) add_line(p[j], p[(j + 1) % 16], colour);
			// Solid, so the ring behind does not show through
			if (style == STYLE_SOLID) add_fill(p, 16, sprite_colour(hstx_dvi_pixel_rgb(0, 0, 0)));
		}
		object_end();
	}

	// How far we can jump, in two halves so each sprite looks at fewer lines on a row
	const float range = (float)FUEL_RANGE / 4.0f * LOCAL_SCALE;
	Vec2_t ring[32];
	hud_ellipse(ring, 32, home.x, home.y, range, range);
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
			++_sprite_count;
			taken[_near_count + n] = (ChartBox_t){ try[k][0], try[k][1], try[k][0] + w, try[k][1] + 8 };
			++n;
			break;
		}
	}
	chart_info(info, to);
}

// The data on a system: us first, then the nearest systems in turn
#define DATA_FRAMES (STYLE_FRAMES / 4)
#define DATA_X 24
#define DATA_TOP (VIEW_TOP + 10)
#define DATA_TEXTS 32

#define DATA_LINE 14
// The letters in a line of the description
#define DATA_WRAP 74

static GameText_t _data_text[2][DATA_TEXTS];

// The k-th nearest system to us, us being the 0th
static uint32_t nearest_system(uint32_t k) {
	uint8_t order[GALAXY_SYSTEMS];
	uint32_t n = 0;
	const GalaxySystem_t* h = &_galaxy[HOME];
	for (uint32_t i = 0; i < _near_count; ++i) {
		const uint32_t d = galaxy_distance(h, &_galaxy[_near[i]]);
		uint32_t j = n++;
		for (; j > 0 && galaxy_distance(h, &_galaxy[order[j - 1]]) > d; --j) order[j] = order[j - 1];
		order[j] = _near[i];
	}
	return n ? order[k % n] : HOME;
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
		++_sprite_count;
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
		++_sprite_count;
	}
	return t;
}

// Returns the name of the system
static const char* draw_data(uint32_t frame, GameText_t* texts) {
	const GalaxySystem_t* h = &_galaxy[HOME];
	const GalaxySystem_t* sys = &_galaxy[nearest_system((frame % STYLE_FRAMES) / DATA_FRAMES)];
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
	return sys->name;
}

static float distance2(const Vec3_t* v) {
	return v->x * v->x + v->y * v->y + v->z * v->z;
}

static uint32_t percent(uint32_t cycles, uint32_t lines) {
	return (cycles * 100u + lines * LINE_CYCLES / 2) / (lines * LINE_CYCLES);
}

// Cycles as rows of the display, to one place, such as "5.2" or "-0.3"
static void rows_text(char* s, size_t n, int32_t cycles) {
	const int32_t tenths = cycles * 10 / LINE_CYCLES;
	const int32_t a = tenths < 0 ? -tenths : tenths;
	snprintf(s, n, "%s%d.%d", tenths < 0 ? "-" : "", (int)(a / 10), (int)(a % 10));
}

// ----------------------------------------------------------------------------
// With WIREFRAME_BENCH defined, a benchmark run at the start, timing the parts
// of the renderer's work on their own, from the meter. The results are shown
// for a while, and printed over USB serial.
#if defined(WIREFRAME_BENCH)

#define BENCH_SETTLE 10
#define BENCH_FRAMES 30
#define BENCH_LINES 32
#define BENCH_STACK_ROWS 12
#define BENCH_RESULTS 8
#define BENCH_SHOW_FRAMES (20 * 60)

static GameText_t _bench_title;
static GameText_t _bench_lines[BENCH_RESULTS];

static void bench_rect(void) {
	const Vec2_t r[4] = {
		{ 0.0f, (float)VIEW_TOP }, { (float)GAME_SCREEN_W, (float)VIEW_TOP },
		{ (float)GAME_SCREEN_W, (float)VIEW_BOTTOM }, { 0.0f, (float)VIEW_BOTTOM }
	};
	add_fill(r, 4, sprite_colour(hstx_dvi_pixel_rgb(0, 0, 146)));
}

static void bench_line(uint32_t i) {
	const float x = 10.0f + 19.0f * (float)i;
	add_line((Vec2_t){ x, (float)VIEW_TOP }, (Vec2_t){ x, (float)(VIEW_BOTTOM - 1) }, hstx_dvi_pixel_rgb(255, 255, 255));
}

static void bench_empty(void) {
}

static void bench_fill(void) {
	if (!object_begin()) return;
	bench_rect();
	object_end();
}

// The second is under the first, so all its pixels are already taken
static void bench_fill_under(void) {
	if (!object_begin()) return;
	bench_rect();
	bench_rect();
	object_end();
}

static void bench_lines_one_sprite(void) {
	if (!object_begin()) return;
	for (uint32_t i = 0; i < BENCH_LINES; ++i) bench_line(i);
	object_end();
}

static void bench_lines_own_sprites(void) {
	for (uint32_t i = 0; i < BENCH_LINES; ++i) {
		if (!object_begin()) return;
		bench_line(i);
		object_end();
	}
}

// Only one of the lines is on each row, so the rest are skipped
static void bench_lines_stacked(void) {
	if (!object_begin()) return;
	for (uint32_t i = 0; i < BENCH_LINES; ++i) {
		const float y = (float)(VIEW_TOP + BENCH_STACK_ROWS * i);
		const float x = 100.0f + (float)i;
		add_line((Vec2_t){ x, y }, (Vec2_t){ x, y + (float)(BENCH_STACK_ROWS - 1) }, hstx_dvi_pixel_rgb(255, 255, 255));
	}
	object_end();
}

// The cycles the renderer spends on a frame of the scene
static uint32_t bench_run(const char* name, void (*scene)(void)) {
	game_text_set_smooth(&_bench_title, name);
	uint64_t sum = 0;
	for (uint32_t f = 0; f < BENCH_SETTLE + BENCH_FRAMES; ++f) {
		_set ^= 1;
		_object_count = 0;
		_prim_count = 0;
		_band_count = 0;
		game_spr_begin();
		game_spr_text_centre(&_bench_title, 12, game_pallet(GAME_YELLOW));
		scene();
		game_spr_end();
		hstx_dvi_sprite_wait_for_frame();
		if (f >= BENCH_SETTLE) sum += hstx_dvi_sprite_timing.frame;
	}
	const uint32_t cycles = (uint32_t)(sum / BENCH_FRAMES);
	printf("bench %-24s %8u cycles a frame\n", name, (unsigned)cycles);
	return cycles;
}

// How fast SysTick counts, in kHz, checked against the microsecond timer on this core
static uint32_t bench_systick_khz(void) {
#if defined(__ARM_ARCH)
	systick_hw->rvr = 0x00ffffff;
	systick_hw->cvr = 0;
	systick_hw->csr = 0x5;
	const uint32_t a = systick_hw->cvr;
	busy_wait_us(10000);
	const uint32_t b = systick_hw->cvr;
	return ((a - b) & 0x00ffffff) / 10;
#else
	return 0;
#endif
}

// Sets a result line, with a value in tenths
static void bench_result(uint32_t i, const char* name, int32_t tenths) {
	char s[GAME_TEXT_MAX];
	if (tenths < 0) tenths = 0;
	snprintf(s, sizeof(s), "%-14s %5u.%u", name, (unsigned)(tenths / 10), (unsigned)(tenths % 10));
	game_text_set_smooth(&_bench_lines[i], s);
	printf("%s\n", s);
}

static void bench(void) {
	const uint32_t khz = bench_systick_khz();
	const uint32_t empty = bench_run("EMPTY", bench_empty);
	const uint32_t fill = bench_run("FILL", bench_fill);
	const uint32_t under = bench_run("FILL UNDER", bench_fill_under);
	const uint32_t lines = bench_run("LINES ONE SPRITE", bench_lines_one_sprite);
	const uint32_t own = bench_run("LINES OWN SPRITES", bench_lines_own_sprites);
	const uint32_t stacked = bench_run("LINES STACKED", bench_lines_stacked);

	// In cycles, as tenths
	const int64_t rows = VIEW_BOTTOM - VIEW_TOP;
	const int64_t pixels = rows * GAME_SCREEN_W;
	const int64_t spans = rows * BENCH_LINES;
	const int64_t span = ((int64_t)lines - empty) * 10 / spans;
	const int64_t stack_rows = BENCH_STACK_ROWS * BENCH_LINES;
	printf("systick %u kHz, expected %u kHz\n", (unsigned)khz, (unsigned)(MODE_HSTX_FREQ_HZ / 1000));
	bench_result(0, "SYSTICK MHZ", (int32_t)(khz / 100));
	bench_result(1, "EMPTY ROW", (int32_t)((int64_t)empty * 10 / MODE_V_ACTIVE_LINES));
	bench_result(2, "FILL PIXEL", (int32_t)(((int64_t)fill - empty) * 10 / pixels));
	bench_result(3, "TAKEN PIXEL", (int32_t)(((int64_t)under - fill) * 10 / pixels));
	bench_result(4, "LINE SPAN", (int32_t)span);
	bench_result(5, "SPRITE CALL", (int32_t)(((int64_t)own - lines) * 10 / spans));
	bench_result(6, "PART SKIPPED", (int32_t)((((int64_t)stacked - empty) * 10 - stack_rows * span) / (stack_rows * (BENCH_LINES - 1))));
	bench_result(7, "LINE BUDGET", LINE_CYCLES * 10);

	// Show the results for a while
	game_text_set_smooth(&_bench_title, "CYCLES");
	for (uint32_t f = 0; f < BENCH_SHOW_FRAMES; ++f) {
		game_spr_begin();
		game_spr_text_centre(&_bench_title, 12, game_pallet(GAME_YELLOW));
		for (uint32_t i = 0; i < BENCH_RESULTS; ++i) {
			game_spr_text(&_bench_lines[i], 96, 80 + 36 * (int32_t)i, game_pallet(GAME_CYAN));
		}
		game_spr_end();
		hstx_dvi_sprite_wait_for_frame();
	}
}
#endif

void main_loop(void) {
	for (uint32_t frame = 0; ; ++frame) {
		const Mode_t* mode = &_modes[(frame / STYLE_FRAMES) % MODE_COUNT];
		const Style_e style = mode->style;
		_dither = mode->dither;
		move_objects(frame);
		_set ^= 1;
		_object_count = 0;
		_prim_count = 0;
		_band_count = 0;
		_sprite_count = 2;
		_occluder_count = 0;
		_hidden_count = 0;

		game_spr_begin();
		GameText_t* title = &_title[frame & 1];
		GameText_t* status = &_status[frame & 1];
		game_spr_text_centre(status, 4, game_pallet(GAME_CYAN));
		game_spr_text_centre(title, 16, game_pallet(GAME_YELLOW));
		draw_hud(style, frame, mode->screen == SCREEN_SPACE);

		if (mode->screen == SCREEN_SPACE) {
			draw_stars();

			// Nearest first, so they cover the ones behind
			uint32_t order[OBJECTS];
			for (uint32_t i = 0; i < OBJECTS; ++i) {
				uint32_t j = i;
				for (; j > 0 && distance2(&_objects[order[j - 1]].pos) > distance2(&_objects[i].pos); --j) order[j] = order[j - 1];
				order[j] = i;
			}
			for (uint32_t i = 0; i < OBJECTS; ++i) draw_object(&_objects[order[i]], style);
			draw_sun(style, frame);

			// The name of the nearest ship, which comes first bar the station
			const Object_t* nearest = &_objects[order[0]];
			if (order[0] == OBJ_STATION) nearest = &_objects[order[1]];
			game_text_set_smooth(title, nearest->model->name);
		}
		else {
			// The next galaxy each time round
			const uint32_t g = (frame / (STYLE_FRAMES * MODE_COUNT)) % GALAXIES;
			set_galaxy(g);
			char s[GAME_TEXT_MAX];
			if (mode->screen == SCREEN_GALAXY) {
				snprintf(s, sizeof(s), "GALACTIC CHART %u", (unsigned)(g + 1));
				draw_galaxy_chart(frame, &_chart_info[frame & 1]);
			}
			else if (mode->screen == SCREEN_LOCAL) {
				snprintf(s, sizeof(s), "SHORT RANGE CHART");
				draw_local_chart(style, frame, _chart_labels[frame & 1], &_chart_info[frame & 1]);
			}
			else {
				snprintf(s, sizeof(s), "DATA ON %s", draw_data(frame, _data_text[frame & 1]));
			}
			game_text_set_smooth(title, s);
		}
		game_spr_end();

		// How hard the renderer worked on the last frame it drew
		char s[GAME_TEXT_MAX];
		const HstxDviSpriteTiming_t timing = hstx_dvi_sprite_timing;
		char ahead[16];
		rows_text(ahead, sizeof(ahead), timing.ahead_min);
		if (timing.frame) {
			snprintf(s, sizeof(s), "%s AHEAD %s AVG %u%%", mode->name, ahead,
				(unsigned)percent(timing.frame, FRAME_LINES));
		}
		else {
			snprintf(s, sizeof(s), "%s PARTS %u", mode->name, (unsigned)_prim_count);
		}
		game_text_set(status, s, 1);
		if (timing.frame && frame % 60 == 0) {
			printf("%s parts %u sprites %u hidden %u calls %u ahead %s line %u%% load %u%% avg %u%%\n", mode->name,
				(unsigned)_prim_count, (unsigned)_sprite_count, (unsigned)_hidden_count, (unsigned)timing.calls_max, ahead,
				(unsigned)percent(timing.line_max, 1),
				(unsigned)percent(timing.window_max, HSTX_DVI_ROW_FIFO_SIZE),
				(unsigned)percent(timing.frame, FRAME_LINES));
		}
		hstx_dvi_sprite_wait_for_frame();
	}
}

int main(void)
{
	stdio_init_all();
	app_led_on();

	game_pallet_init();
	_objects[OBJ_STATION].model = &model_coriolis;
	for (uint32_t i = 0; i < MODEL_SHIP_COUNT; ++i) _objects[OBJ_SHIP + i].model = model_ships[i];
	for (uint32_t i = 0; i < 2; ++i) _objects[OBJ_MISSILE + i].model = &model_missile;
	for (uint32_t i = 0; i < OBJECTS; ++i) {
		for (uint32_t c = 0; c < 3; ++c) _objects[i].rgb[c] = _objects[i].model->rgb[c];
	}
	const float l = sqrtf(0.4f * 0.4f + 0.6f * 0.6f + 0.7f * 0.7f);
	_light = (Vec3_t){ -0.4f / l, 0.6f / l, -0.7f / l };
	_star_colours[0] = hstx_dvi_pixel_rgb(73, 73, 109);
	_star_colours[1] = hstx_dvi_pixel_rgb(146, 146, 182);
	_star_colours[2] = hstx_dvi_pixel_rgb(255, 255, 255);
	for (uint32_t i = 0; i < STARS; ++i) new_star(&_stars[i], rand_range(NEAR_Z, 2000.0f));

	for (uint32_t i = 0; i < 2; ++i) {
		game_text_set_smooth(&_title[i], "");
		game_text_set(&_status[i], "", 1);
	}
	for (uint32_t g = 0; g < GAUGES; ++g) game_text_set(&_gauge_labels[g], _gauge_names[g], 1);

	// Initialize the HSTX DVI sprite system
	hstx_dvi_sprite_init_all();

	sleep_ms(2000); // Allow time for initialization

	printf("HSTX DVI Wireframe Test\n");

#if defined(WIREFRAME_BENCH)
	bench();
#endif
	main_loop();
}
