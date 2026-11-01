// Copyright (c) 2024 fruit-bat.

#include "el_space.h"
#include <math.h>
#include <stdlib.h>

Vec3_t vec_add(Vec3_t a, Vec3_t b) {
	return (Vec3_t){ a.x + b.x, a.y + b.y, a.z + b.z };
}

Vec3_t vec_sub(Vec3_t a, Vec3_t b) {
	return (Vec3_t){ a.x - b.x, a.y - b.y, a.z - b.z };
}

Vec3_t vec_scale(Vec3_t a, float k) {
	return (Vec3_t){ a.x * k, a.y * k, a.z * k };
}

float vec_dot(Vec3_t a, Vec3_t b) {
	return a.x * b.x + a.y * b.y + a.z * b.z;
}

float vec_length(Vec3_t a) {
	return sqrtf(vec_dot(a, a));
}

Vec3_t vec_unit(Vec3_t a) {
	const float l = vec_length(a);
	return l > 0.0f ? vec_scale(a, 1.0f / l) : (Vec3_t){ 0.0f, 0.0f, 1.0f };
}

static Vec3_t cross(Vec3_t a, Vec3_t b) {
	return (Vec3_t){ a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x };
}

void frame_set(Frame_t* fr, Vec3_t pos, Vec3_t f, Vec3_t u) {
	fr->pos = pos;
	fr->f = f;
	fr->u = u;
	frame_tidy(fr);
}

void frame_roll(Frame_t* fr, float a) {
	const float c = cosf(a), s = sinf(a);
	const Vec3_t r = fr->r, u = fr->u;
	fr->r = vec_sub(vec_scale(r, c), vec_scale(u, s));
	fr->u = vec_add(vec_scale(u, c), vec_scale(r, s));
}

void frame_pitch(Frame_t* fr, float a) {
	const float c = cosf(a), s = sinf(a);
	const Vec3_t f = fr->f, u = fr->u;
	fr->f = vec_add(vec_scale(f, c), vec_scale(u, s));
	fr->u = vec_sub(vec_scale(u, c), vec_scale(f, s));
}

void frame_tidy(Frame_t* fr) {
	fr->f = vec_unit(fr->f);
	fr->r = vec_unit(cross(fr->u, fr->f));
	fr->u = cross(fr->f, fr->r);
}

Vec3_t frame_view(const Frame_t* eye, Vec3_t p) {
	const Vec3_t d = vec_sub(p, eye->pos);
	return (Vec3_t){ vec_dot(d, eye->r), vec_dot(d, eye->u), vec_dot(d, eye->f) };
}

void frame_matrix(const Frame_t* eye, const Frame_t* body, float m[3][3]) {
	const Vec3_t* e[3] = { &eye->r, &eye->u, &eye->f };
	const Vec3_t* b[3] = { &body->r, &body->u, &body->f };
	for (uint32_t i = 0; i < 3; ++i) {
		for (uint32_t j = 0; j < 3; ++j) m[i][j] = vec_dot(*e[i], *b[j]);
	}
}

// ----------------------------------------------------------------------------
// The dust

#define DUST 40
#define DUST_NEAR 60.0f
#define DUST_FAR 1600.0f

static Vec3_t _dust[DUST];
// Not const so they live in RAM; the renderer reads them every row
static hstx_dvi_pixel_t _dust_colours[3];

static float rand_range(float lo, float hi) {
	return lo + (hi - lo) * (float)(rand() & 0xffff) / 65535.0f;
}

// Somewhere in the view, at a distance from lo to hi
static void new_speck(Vec3_t* s, float lo, float hi) {
	const float z = rand_range(lo, hi);
	const float w = z * (float)CENTRE_X / (FOCAL * draw_aspect);
	const float h = z * (float)(CENTRE_Y - VIEW_TOP) / FOCAL;
	*s = (Vec3_t){ rand_range(-w, w), rand_range(-h, h), z };
}

void dust_init(void) {
	_dust_colours[0] = hstx_dvi_pixel_rgb(73, 73, 109);
	_dust_colours[1] = hstx_dvi_pixel_rgb(146, 146, 182);
	_dust_colours[2] = hstx_dvi_pixel_rgb(255, 255, 255);
	for (uint32_t i = 0; i < DUST; ++i) new_speck(&_dust[i], DUST_NEAR, DUST_FAR);
}

void dust_move(float roll, float pitch, float speed) {
	const float cr = cosf(roll), sr = sinf(roll);
	const float cp = cosf(pitch), sp = sinf(pitch);
	for (uint32_t i = 0; i < DUST; ++i) {
		Vec3_t* s = &_dust[i];
		// As frame_roll and frame_pitch turn us, so the view of a point fixed in space turns
		const float x = s->x * cr - s->y * sr;
		const float y = s->x * sr + s->y * cr;
		s->x = x;
		s->y = y * cp - s->z * sp;
		s->z = s->z * cp + y * sp - speed;
		if (s->z < DUST_NEAR || s->z > DUST_FAR) {
			// Going forward new specks come from far ahead, and going backward from close by
			if (speed >= 0.0f) new_speck(s, DUST_FAR * 0.6f, DUST_FAR);
			else new_speck(s, DUST_NEAR, DUST_NEAR * 3.0f);
		}
	}
}

void dust_draw(void) {
	for (uint32_t i = 0; i < DUST; ++i) {
		Vec3_t* s = &_dust[i];
		const Vec2_t p = project(s);
		const int32_t x = (int32_t)p.x, y = (int32_t)p.y;
		if (x < 0 || x >= GAME_SCREEN_W - 2 || y < VIEW_TOP || y >= VIEW_BOTTOM) {
			new_speck(s, DUST_FAR * 0.6f, DUST_FAR);
			continue;
		}
		const uint32_t b = s->z < 300.0f ? 2 : s->z < 800.0f ? 1 : 0;
		game_spr_hline(x, y, b == 2 ? 2 : 1, &_dust_colours[b]);
		draw_count_sprite();
	}
}

// ----------------------------------------------------------------------------
// Planets

// The most sides of the outline, for a planet close up
#define PLANET_MAX_SIDES 96
#define PLANET_MIN_SIDES 16
// An object's sprite looks at all its parts on every row it covers, so the
// lines are added in runs of this many, each an object of its own
#define PLANET_RUN 16
#define OCCLUDER_SIDES 24

// Adds a loop of n points on the planet, the lines between them that are on
// the side facing us, in runs
static void planet_loop(const Vec3_t* p, const bool* front, uint32_t n, hstx_dvi_pixel_t colour) {
	for (uint32_t first = 0; first < n; first += PLANET_RUN) {
		if (!object_begin()) return;
		for (uint32_t i = first; i < first + PLANET_RUN && i < n; ++i) {
			const uint32_t j = (i + 1) % n;
			if (front[i] && front[j]) add_edge(p[i], p[j], colour);
		}
		object_end();
	}
}

void draw_planet(Vec3_t c, float r, const float m[3][3], const uint8_t rgb[3], bool crater, Style_e style) {
	const float d2 = vec_dot(c, c);
	if (d2 <= r * r * 1.0001f) return;
	// The edge we see is a circle on the planet, round the line to its middle
	const float k = r * r / d2;
	const Vec3_t lc = vec_scale(c, 1.0f - k);
	const float lr = r * sqrtf(1.0f - k);
	if (lc.z < -lr) return;
	const Vec3_t n = vec_unit(c);
	const Vec3_t a = fabsf(n.x) < 0.9f ? (Vec3_t){ 1.0f, 0.0f, 0.0f } : (Vec3_t){ 0.0f, 1.0f, 0.0f };
	const Vec3_t e1 = vec_unit(vec_sub(a, vec_scale(n, vec_dot(a, n))));
	const Vec3_t e2 = cross(n, e1);

	// More sides the bigger it looks, so the edge stays smooth
	const float px = FOCAL * lr / fmaxf(lc.z, NEAR_Z);
	uint32_t sides = (uint32_t)(3.0f * sqrtf(px));
	if (sides < PLANET_MIN_SIDES) sides = PLANET_MIN_SIDES;
	if (sides > PLANET_MAX_SIDES) sides = PLANET_MAX_SIDES;
	sides &= ~1u;

	static Vec3_t edge[PLANET_MAX_SIDES];
	static bool front[PLANET_MAX_SIDES];
	bool all_ahead = true;
	for (uint32_t i = 0; i < sides; ++i) {
		const float t = 2.0f * PI_F * (float)i / (float)sides;
		edge[i] = vec_add(lc, vec_add(vec_scale(e1, lr * cosf(t)), vec_scale(e2, lr * sinf(t))));
		front[i] = true;
		if (edge[i].z < NEAR_Z) all_ahead = false;
	}
	const bool filled = style == STYLE_OUTLINED || style == STYLE_FILLED;
	const hstx_dvi_pixel_t line = shade(rgb, filled ? 0.35f : 1.0f);

	// Its lines, which are on the side facing us where the way to them is
	// against the way out of the planet
	static Vec3_t loop[PLANET_MAX_SIDES];
	static bool seen[PLANET_MAX_SIDES];
	const uint32_t loops = crater ? 1 : 2;
	for (uint32_t l = 0; l < loops; ++l) {
		for (uint32_t i = 0; i < sides; ++i) {
			const float t = 2.0f * PI_F * (float)i / (float)sides;
			Vec3_t v;
			if (crater) {
				// A ring round a point d a little north of the equator, between
				// d and the axes (0, 0, 1) and (d.y, -d.x, 0) square to it
				const float lat = 0.5f, size = 0.45f, cs = cosf(size), ss = sinf(size);
				const float dx = cosf(lat), dy = sinf(lat);
				v = (Vec3_t){ cs * dx + ss * sinf(t) * dy, cs * dy - ss * sinf(t) * dx, ss * cosf(t) };
			}
			else if (l == 0) {
				v = (Vec3_t){ cosf(t), sinf(t), 0.0f };
			}
			else {
				v = (Vec3_t){ cosf(t), 0.0f, sinf(t) };
			}
			const Vec3_t w = rotate(&v, m);
			loop[i] = vec_add(c, vec_scale(w, r));
			seen[i] = vec_dot(w, loop[i]) < 0.0f;
		}
		planet_loop(loop, seen, sides, line);
	}

	// Its edge, then filled in black to hide what is behind, or in its colour
	planet_loop(edge, front, sides, shade(rgb, 1.0f));
	if (style == STYLE_WIRE || !object_begin()) return;
	add_shape(edge, sides, filled ? face_colour(rgb, 0.9f) : sprite_colour(hstx_dvi_pixel_rgb(0, 0, 0)));
	object_end();
	if (all_ahead) {
		// A shape inside its edge on the screen, to hide what is wholly behind it
		Vec2_t o[OCCLUDER_SIDES];
		for (uint32_t i = 0; i < OCCLUDER_SIDES; ++i) o[i] = project(&edge[i * sides / OCCLUDER_SIDES]);
		add_occluder(o, OCCLUDER_SIDES);
	}
}
