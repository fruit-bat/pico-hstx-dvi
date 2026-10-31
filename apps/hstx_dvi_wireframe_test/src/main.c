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
#include "game_pallet.h"
#include "game_render.h"
#include "game_util.h"
#include "ship_model.h"
#if defined(__ARM_ARCH)
#include "hardware/structs/systick.h"
#endif

// Enough for the scene
#define MAX_OBJECTS 40
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
// The view, a little shorter than the screen to leave room for the text
#define VIEW_TOP 40
#define VIEW_BOTTOM (GAME_SCREEN_H - 40)
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

static const char* const _style_names[STYLE_COUNT] = { "SOLID WIREFRAME", "WIREFRAME", "OUTLINED", "FILLED" };

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

// The sun, far away, as a disc with a ragged edge that flickers
#define SUN_X 5200.0f
#define SUN_Y 2600.0f
#define SUN_Z 9000.0f
#define SUN_RADIUS 1600.0f
// How much of it is the bright middle
#define SUN_CORE 0.75f
#define VIEW_ROWS (VIEW_BOTTOM - VIEW_TOP)

// How far the sun reaches either side of its middle on each row
typedef struct {
	int32_t cx;
	int16_t outer[VIEW_ROWS];
	int16_t inner[VIEW_ROWS];
	hstx_dvi_pixel_t core, rim;
} Sun_t;

static Sun_t _suns[2];

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

// The middle of the sun first, so it keeps the pixels, then its rim round it
static void __not_in_flash_func(sun_renderer)(
	const void* d1,
	const void* d2,
	hstx_dvi_row_t* r,
	const int32_t x,
	const int32_t row,
	const SpriteId spriteId
) {
	const Sun_t* s = (const Sun_t*)d1;
	const int32_t i = s->inner[row];
	const int32_t o = s->outer[row];
	if (i >= 0) sprite_render_span(r, s->cx - i, s->cx + i + 1, s->core, spriteId);
	sprite_render_span(r, s->cx - o, s->cx + o + 1, s->rim, spriteId);
}

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

static Vec2_t project(const Vec3_t* v) {
	return (Vec2_t){ CENTRE_X + FOCAL * v->x / v->z, CENTRE_Y - FOCAL * v->y / v->z };
}

// Cuts a line down to the part inside the view, returning false if none of it is
static bool clip_line(Vec2_t* a, Vec2_t* b) {
	const float dx = b->x - a->x;
	const float dy = b->y - a->y;
	const float p[4] = { -dx, dx, -dy, dy };
	const float q[4] = {
		a->x,
		(float)(GAME_SCREEN_W - 1) - a->x,
		a->y - (float)VIEW_TOP,
		(float)(VIEW_BOTTOM - 1) - a->y
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
	p->colour = colour;
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
static void add_fill(const Vec2_t* corners, uint32_t n, hstx_dvi_pixel_t colour) {
	if (n < 3 || n > MAX_POLY) return;
	// Cut to the view
	Vec2_t p[MAX_POLY + 4], q[MAX_POLY + 4];
	uint32_t m = clip_side(corners, n, q, 0, 0.0f, false);
	m = clip_side(q, m, p, 0, (float)GAME_SCREEN_W, true);
	m = clip_side(p, m, q, 1, (float)VIEW_TOP, false);
	m = clip_side(q, m, p, 1, (float)VIEW_BOTTOM, true);
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
static void add_shape(const Vec3_t* corners, uint32_t n, hstx_dvi_pixel_t colour) {
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

static void draw_model(const Object_t* o, Style_e style) {
	const WireModel_t* model = o->model;
	if (o->pos.z < -model->radius) return;
	const Vec2_t centre = project(&o->pos);
	// Far away it is just a dot
	if (o->pos.z > model->radius && FOCAL * model->radius * 2.0f / o->pos.z < DOT_SIZE) {
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
		else if (style == STYLE_OUTLINED && e->colour == PART_HULL) {
			colour = shade(rgb, 0.25f);
		}
		else {
			continue;
		}
		add_edge(cam[e->v0], cam[e->v1], colour);
	}

	Vec3_t corners[WIRE_MAX_LOOP];
	if (style == STYLE_SOLID) {
		const hstx_dvi_pixel_t black = hstx_dvi_pixel_rgb(0, 0, 0);
		if (!behind) {
			// The outline of the hull, as one shape
			Vec2_t p[MAX_VERTS], q[2 * MAX_VERTS];
			for (uint32_t i = 0; i < model->nh; ++i) p[i] = project(&cam[i]);
			add_fill(q, outline(p, model->nh, q), black);
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
		add_shape(corners, p->n, shade(_part_rgb[p->colour], 0.5f + 0.5f * light[p->f]));
	}
	for (uint32_t i = 0; i < model->nf; ++i) {
		const WireFace_t* f = &model->f[i];
		if (!facing[i]) continue;
		for (uint32_t j = 0; j < f->n; ++j) corners[j] = cam[f->v[j]];
		add_shape(corners, f->n, shade(o->rgb, 0.3f + 0.7f * light[i]));
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
		220.0f * sinf((float)i * 2.4f) + 40.0f * sinf(t * 0.5f + (float)i),
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

// Adds the sun's sprite, with a ragged edge that changes every frame. It is
// added after everything else, so it is behind them all bar the stars.
static void draw_sun(void) {
	const Vec3_t c = { SUN_X, SUN_Y, SUN_Z };
	const Vec2_t m = project(&c);
	const float rp = FOCAL * SUN_RADIUS / SUN_Z;
	int32_t top = (int32_t)ceilf(m.y - rp);
	int32_t bottom = (int32_t)floorf(m.y + rp);
	if (top < VIEW_TOP) top = VIEW_TOP;
	if (bottom > VIEW_BOTTOM - 1) bottom = VIEW_BOTTOM - 1;
	if (bottom < top) return;
	Sun_t* sun = &_suns[_set];
	sun->cx = (int32_t)lroundf(m.x);
	for (int32_t y = top; y <= bottom; ++y) {
		const float dy = (float)y + 0.5f - m.y;
		const float w = rp * rp - dy * dy;
		const float hw = w > 0.0f ? sqrtf(w) : 0.0f;
		sun->outer[y - top] = (int16_t)(hw + (float)(rand() % 4));
		const float wi = rp * rp * SUN_CORE * SUN_CORE - dy * dy;
		sun->inner[y - top] = wi > 0.0f ? (int16_t)(sqrtf(wi) - (float)(rand() % 3)) : -1;
	}
	if (game_spr_add(sun->cx - (int32_t)rp - 4, top, (uint32_t)(2.0f * rp) + 9, bottom - top + 1, sun, NULL, sun_renderer)) {
		++_sprite_count;
	}
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
	add_fill(r, 4, hstx_dvi_pixel_rgb(0, 0, 146));
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
	game_text_set(&_bench_title, name, 2);
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
	game_text_set(&_bench_lines[i], s, 2);
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
	game_text_set(&_bench_title, "CYCLES", 2);
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
		const Style_e style = (Style_e)((frame / STYLE_FRAMES) % STYLE_COUNT);
		move_objects(frame);
		_set ^= 1;
		_object_count = 0;
		_prim_count = 0;
		_band_count = 0;
		_sprite_count = 2;

		game_spr_begin();
		GameText_t* title = &_title[frame & 1];
		GameText_t* status = &_status[frame & 1];
		game_spr_text_centre(title, 12, game_pallet(GAME_YELLOW));
		game_spr_text_centre(status, GAME_SCREEN_H - 24, game_pallet(GAME_CYAN));
		draw_stars();

		// Nearest first, so they cover the ones behind
		uint32_t order[OBJECTS];
		for (uint32_t i = 0; i < OBJECTS; ++i) {
			uint32_t j = i;
			for (; j > 0 && distance2(&_objects[order[j - 1]].pos) > distance2(&_objects[i].pos); --j) order[j] = order[j - 1];
			order[j] = i;
		}
		for (uint32_t i = 0; i < OBJECTS; ++i) draw_object(&_objects[order[i]], style);
		draw_sun();
		game_spr_end();

		// The name of the nearest ship, which comes first bar the station
		const Object_t* nearest = &_objects[order[0]];
		if (order[0] == OBJ_STATION) nearest = &_objects[order[1]];
		game_text_set(title, nearest->model->name, 2);

		// How hard the renderer worked on the last frame it drew
		char s[GAME_TEXT_MAX];
		const HstxDviSpriteTiming_t timing = hstx_dvi_sprite_timing;
		char ahead[16];
		rows_text(ahead, sizeof(ahead), timing.ahead_min);
		if (timing.frame) {
			snprintf(s, sizeof(s), "%s AHEAD %s AVG %u%%", _style_names[style], ahead,
				(unsigned)percent(timing.frame, FRAME_LINES));
		}
		else {
			snprintf(s, sizeof(s), "%s PARTS %u", _style_names[style], (unsigned)_prim_count);
		}
		game_text_set(status, s, 2);
		if (timing.frame && frame % 60 == 0) {
			printf("%s parts %u sprites %u calls %u ahead %s line %u%% load %u%% avg %u%%\n", _style_names[style],
				(unsigned)_prim_count, (unsigned)_sprite_count, (unsigned)timing.calls_max, ahead,
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
	for (uint32_t i = 0; i < 2; ++i) {
		_suns[i].core = hstx_dvi_pixel_rgb(255, 255, 182);
		_suns[i].rim = hstx_dvi_pixel_rgb(255, 146, 0);
	}
	for (uint32_t i = 0; i < STARS; ++i) new_star(&_stars[i], rand_range(NEAR_Z, 2000.0f));

	for (uint32_t i = 0; i < 2; ++i) {
		game_text_set(&_title[i], "", 2);
		game_text_set(&_status[i], "", 2);
	}

	// Initialize the HSTX DVI sprite system
	hstx_dvi_sprite_init_all();

	sleep_ms(2000); // Allow time for initialization

	printf("HSTX DVI Wireframe Test\n");

#if defined(WIREFRAME_BENCH)
	bench();
#endif
	main_loop();
}
