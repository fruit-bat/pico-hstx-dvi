// Copyright (c) 2024 fruit-bat.

// See el_draw.h. This is the drawing of the wireframe test, made into a part
// of its own.

#include "el_draw.h"
#include <math.h>

// Enough for a busy scene
#define MAX_OBJECTS 128
#define MAX_PRIMS 1200
#define MAX_ALL_BANDS 2400
#define MAX_VERTS 48
#define MAX_FACES 32
// A shape can gain a corner for each side of the view it is cut by, and one
// at the near plane. Planets close up need many corners for a smooth edge.
#define MAX_POLY 100
// Smaller than this, in pixels across, and a ship is just a dot of two
// pixels, or of one smaller than PIXEL_SIZE
#define DOT_SIZE 6.0f
#define PIXEL_SIZE 2.0f
// Smaller than this and a ship has no dark outlines when outlined
#define OUTLINE_SIZE 40.0f
// Smaller than this and a wireframe is just its outline
#define DETAIL_SIZE 32.0f
// The fixed point the renderer uses for the x of the edges and shapes
#define FX SPRITE_PARTS_FX

#define SUN_SIDES 24
#define SUN_RAYS 16
#define SUN_RAY_GAP 1.2f
#define SUN_RAY_LONG 1.65f
#define SUN_RAY_SHORT 1.4f

const char* const style_names[STYLE_COUNT] = { "SOLID WIREFRAME", "WIREFRAME", "OUTLINED", "FILLED" };

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

Vec3_t draw_light = { -0.43f, 0.64f, -0.64f };
Style_e draw_style = STYLE_SOLID;
float draw_aspect = ASPECT_16_9;
Lens_e draw_lens = LENS_NORMAL;
const char* const lens_names[LENS_COUNT] = { "WIDE", "NORMAL", "NARROW" };
static const float _focals[LENS_COUNT] = { 320.0f, 400.0f, 440.0f };
float draw_focal = 400.0f;

void draw_set_lens(Lens_e lens) {
	draw_lens = lens < LENS_COUNT ? lens : LENS_NORMAL;
	draw_focal = _focals[draw_lens];
}
bool draw_dither = false;
bool draw_hud_filled = false;

// Starts an object, returning false if there is no room
bool object_begin(void) {
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
void object_end(void) {
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

hstx_dvi_pixel_t shade(const uint8_t rgb[3], float k) {
	if (k > 1.0f) k = 1.0f;
	return hstx_dvi_pixel_rgb((uint8_t)(rgb[0] * k), (uint8_t)(rgb[1] * k), (uint8_t)(rgb[2] * k));
}

// The colour of a lit face, dithered for the shades between the screen's colours if asked
SpriteColour_t face_colour(const uint8_t rgb[3], float k) {
	if (!draw_dither) return sprite_colour(shade(rgb, k));
	if (k > 1.0f) k = 1.0f;
	return sprite_colour_dither((uint8_t)(rgb[0] * k + 0.5f), (uint8_t)(rgb[1] * k + 0.5f), (uint8_t)(rgb[2] * k + 0.5f));
}

Vec2_t project(const Vec3_t* v) {
	return (Vec2_t){ CENTRE_X + FOCAL * draw_aspect * v->x / v->z, CENTRE_Y - FOCAL * v->y / v->z };
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
void add_line(Vec2_t p0, Vec2_t p1, hstx_dvi_pixel_t colour) {
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
void add_edge(Vec3_t a, Vec3_t b, hstx_dvi_pixel_t colour) {
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
void add_fill(const Vec2_t* corners, uint32_t n, SpriteColour_t colour) {
	if (n < 3 || n > MAX_POLY) return;
	// Cut to the view. Static, as they are too big for the stack.
	static Vec2_t p[MAX_POLY + 4], q[MAX_POLY + 4];
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
void add_shape(const Vec3_t* corners, uint32_t n, SpriteColour_t colour) {
	static Vec2_t p[MAX_POLY];
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
uint32_t outline(Vec2_t* p, uint32_t n, Vec2_t* out) {
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

Vec3_t rotate(const Vec3_t* v, const float m[3][3]) {
	return (Vec3_t){
		m[0][0] * v->x + m[0][1] * v->y + m[0][2] * v->z,
		m[1][0] * v->x + m[1][1] * v->y + m[1][2] * v->z,
		m[2][0] * v->x + m[2][1] * v->y + m[2][2] * v->z,
	};
}

// Roll about z, then pitch about x, then yaw about y
void make_matrix(float m[3][3], float yaw, float pitch, float roll) {
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

void add_occluder(const Vec2_t* p, uint32_t n) {
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
bool hidden(Vec2_t c, float r) {
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

void draw_model(const WireModel_t* model, Vec3_t pos, const float m[3][3], const uint8_t rgb[3], Style_e style) {
	if (pos.z < -model->radius) return;
	const Vec2_t centre = project(&pos);
	// Wholly behind something solid and nearer, it is left out
	if (style != STYLE_WIRE && pos.z - model->radius > NEAR_Z &&
		hidden(centre, FOCAL * model->radius / (pos.z - model->radius) + 1.0f)) {
		++_hidden_count;
		return;
	}
	// Far away it is just a dot, in its own colour
	const float size = pos.z > model->radius ? FOCAL * model->radius * 2.0f / pos.z : (float)GAME_SCREEN_W;
	if (size < DOT_SIZE) {
		const float d = size < PIXEL_SIZE ? 0.0f : 1.0f;
		add_line(centre, (Vec2_t){ centre.x + d, centre.y + d }, shade(rgb, 1.0f));
		return;
	}

	// Each corner relative to us. The big arrays here are static, as core 0's
	// stack is only 4KB, and draw_model is never called from within itself.
	static Vec3_t cam[MAX_VERTS];
	bool behind = false;
	for (uint32_t i = 0; i < model->nv; ++i) {
		const Vec3_t v = { model->v[i].x, model->v[i].y, model->v[i].z };
		const Vec3_t r = rotate(&v, m);
		cam[i] = (Vec3_t){ r.x + pos.x, r.y + pos.y, r.z + pos.z };
		if (i < model->nh && cam[i].z < NEAR_Z) behind = true;
	}

	// The outline on the screen, which a solid object hides everything behind
	static Vec2_t hull[2 * MAX_VERTS];
	uint32_t hull_n = 0;
	if (!behind) {
		static Vec2_t p[MAX_VERTS];
		for (uint32_t i = 0; i < model->nh; ++i) p[i] = project(&cam[i]);
		hull_n = outline(p, model->nh, hull);
		if (style != STYLE_WIRE && size >= OCCLUDER_SIZE) add_occluder(hull, hull_n);
	}

	// Small and far away, a wireframe is just its outline. A convex outline
	// crosses each row twice, where all the edges can cross it many times,
	// and each costs about as much on a row however short it is.
	if (!behind && size < DETAIL_SIZE && (style == STYLE_SOLID || style == STYLE_WIRE)) {
		const hstx_dvi_pixel_t c = shade(rgb, 1.0f);
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
		light[i] = fmaxf(0.0f, n.x * draw_light.x + n.y * draw_light.y + n.z * draw_light.z);
	}

	// The edges go first so they are drawn over the faces
	for (uint32_t i = 0; i < model->ne; ++i) {
		const WireEdge_t* e = &model->e[i];
		if (!facing[e->f0] && !facing[e->f1]) continue;
		const uint8_t* edge_rgb = e->colour == PART_HULL ? rgb : _part_rgb[e->colour];
		hstx_dvi_pixel_t colour;
		if (style == STYLE_SOLID || style == STYLE_WIRE || e->colour == PART_TRIM) {
			colour = shade(edge_rgb, 1.0f);
		}
		else if (style == STYLE_OUTLINED && e->colour == PART_HULL && size >= OUTLINE_SIZE && facing[e->f0] && facing[e->f1]) {
			// Only between faces we can see, as dark edges round the outside
			// hardly show against space. Each edge costs about as much on a
			// row however short it is, so small ships have none.
			colour = shade(edge_rgb, 0.25f);
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
		add_shape(corners, f->n, face_colour(rgb, 0.3f + 0.7f * light[i]));
	}
}

void draw_object(const WireModel_t* model, Vec3_t pos, const float m[3][3], const uint8_t rgb[3], Style_e style) {
	if (!object_begin()) return;
	draw_model(model, pos, m, rgb, style);
	object_end();
}

// A sun at m on the screen, rp across. It is an outline with rays that turn
// slowly and flicker, made of parts like the ships, filled in black for the
// solid wireframes and yellow when filled.
void draw_sun(Vec2_t m, float rp, Style_e style, uint32_t frame) {
	const hstx_dvi_pixel_t edge = hstx_dvi_pixel_rgb(255, 255, 0);
	const hstx_dvi_pixel_t ray = hstx_dvi_pixel_rgb(255, 146, 0);
	Vec2_t outline[SUN_SIDES];
	for (uint32_t i = 0; i < SUN_SIDES; ++i) {
		const float a = 2.0f * 3.1416f * (float)i / SUN_SIDES;
		outline[i] = (Vec2_t){ m.x + rp * draw_aspect * cosf(a), m.y + rp * sinf(a) };
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
		const float ca = cosf(a) * draw_aspect, sa = sinf(a);
		add_line(
			(Vec2_t){ m.x + rp * SUN_RAY_GAP * ca, m.y + rp * SUN_RAY_GAP * sa },
			(Vec2_t){ m.x + rp * len * ca, m.y + rp * len * sa }, ray);
		object_end();
	}
}


void add_box(float x0, float y0, float x1, float y1, hstx_dvi_pixel_t c) {
	add_line((Vec2_t){ x0, y0 }, (Vec2_t){ x1, y0 }, c);
	add_line((Vec2_t){ x1, y0 }, (Vec2_t){ x1, y1 }, c);
	add_line((Vec2_t){ x1, y1 }, (Vec2_t){ x0, y1 }, c);
	add_line((Vec2_t){ x0, y1 }, (Vec2_t){ x0, y0 }, c);
}

// Fills the pixels from x0 up to x1 and y0 up to y1
void add_rect(float x0, float y0, float x1, float y1, hstx_dvi_pixel_t c) {
	const Vec2_t r[4] = { { x0, y0 }, { x1, y0 }, { x1, y1 }, { x0, y1 } };
	add_fill(r, 4, sprite_colour(c));
}

void circle_points(Vec2_t* p, uint32_t n, float cx, float cy, float r) {
	ellipse_points(p, n, cx, cy, r * draw_aspect, r);
}

void ellipse_points(Vec2_t* p, uint32_t n, float cx, float cy, float rx, float ry) {
	for (uint32_t i = 0; i < n; ++i) {
		const float a = 2.0f * PI_F * (float)i / (float)n;
		p[i] = (Vec2_t){ cx + rx * cosf(a), cy + ry * sinf(a) };
	}
}

void draw_frame_begin(void) {
	_set ^= 1;
	_object_count = 0;
	_prim_count = 0;
	_band_count = 0;
	_sprite_count = 0;
	_occluder_count = 0;
	_hidden_count = 0;
	draw_clip_view();
	game_spr_begin();
}

void draw_frame_end(void) {
	game_spr_end();
}

uint32_t draw_set(void) {
	return _set;
}

void draw_count_sprite(void) {
	++_sprite_count;
}

DrawStats_t draw_stats(void) {
	return (DrawStats_t){ _prim_count, _sprite_count, _hidden_count };
}

void draw_clip_view(void) {
	_clip_top = VIEW_TOP;
	_clip_bottom = VIEW_BOTTOM;
}

void draw_clip_screen(void) {
	_clip_top = 0;
	_clip_bottom = GAME_SCREEN_H;
}
