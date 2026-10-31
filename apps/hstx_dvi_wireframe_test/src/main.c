// Copyright (c) 2024 fruit-bat.

// Wireframe ships in the style of Elite, with each edge drawn as a sprite.
//
// A sprite is just a box and a callback that draws one row of it, so an edge
// can be one: its box is the edge's bounding box and its callback draws the
// short run of the line that falls on that row. The renderer only calls the
// edges that cross each line.
//
// The edges at the back of a ship are hidden by only drawing an edge when one
// of the two faces it joins is facing us, as the original game did. The ships
// are convex so that is all it takes.
//
// This example requires an external digital video connector connected to
// GPIOs 12 through 19 (the HSTX-capable GPIOs) with appropriate
// current-limiting resistors, e.g. 270 ohms. The pinout used in this example
// matches the Pico DVI Sock board, which can be soldered onto a Pico 2:
// https://github.com/Wren6991/Pico-DVI-Sock

#include "hstx_dvi_core.h"
#include "hstx_dvi_sprite.h"
#include "app_led.h"
#include "pico/stdlib.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "game_pallet.h"
#include "game_render.h"
#include "game_util.h"
#include "ship_model.h"

// Enough for every edge of every ship, though the sprites run out first
#define MAX_EDGES 200
#define MAX_VERTS 32
#define MAX_FACES 16
#define SHIPS 3
#define STARS 48
// The distance to the screen, in pixels, which sets the field of view
#define FOCAL 320.0f
// Nothing closer than this is drawn
#define NEAR_Z 16.0f
// The view, a little shorter than the screen to leave room for the text
#define VIEW_TOP 40
#define VIEW_BOTTOM (GAME_SCREEN_H - 40)
#define CENTRE_X (GAME_SCREEN_W / 2)
#define CENTRE_Y ((VIEW_TOP + VIEW_BOTTOM) / 2)
// 16.16 fixed point for the x of the edges
#define FX 16

// The run of an edge on each row, x in 16.16 and absolute on the screen
typedef struct {
	int32_t x0;   // where the edge starts, on row 0
	int32_t x1;   // where the edge ends, on the last row
	int32_t dxdy; // how far x moves for each row
	int32_t half; // half of dxdy, for the run either side of the middle of a row
	int32_t last; // the last row
} WireSpan_t;

typedef struct {
	float x, y, z;
} Vec3_t;

typedef struct {
	Vec3_t pos;
	float yaw, pitch, roll;
	uint32_t colour;
} Ship_t;

typedef struct {
	float x, y, z;
} Star_t;

// The renderer on core 1 reads the spans of the frame it is drawing while
// this core works out the next, so there are two sets
static WireSpan_t _spans[2][MAX_EDGES];
static uint32_t _span_set = 0;
static uint32_t _span_count = 0;

// Not const so they live in RAM; the renderer reads them every row
static hstx_dvi_pixel_t _colours[4][3];
static hstx_dvi_pixel_t _star_colours[3];

static Ship_t _ships[SHIPS];
static Star_t _stars[STARS];

static GameText_t _title;
static GameText_t _status[2];

// Draws the run of the edge that falls on this row. The run covers the line
// from half a row above the middle of the row to half a row below it, so
// shallow lines join up, and stops at the ends of the edge.
static void __not_in_flash_func(edge_renderer)(
	const void* d1,
	const void* d2,
	hstx_dvi_row_t* r,
	const int32_t x,
	const int32_t row,
	const SpriteId spriteId
) {
	const WireSpan_t* s = (const WireSpan_t*)d1;
	const int32_t xm = s->x0 + s->dxdy * row;
	int32_t xa = row == 0 ? s->x0 : xm - s->half;
	int32_t xb = row == s->last ? s->x1 : xm + s->half;
	if (xa > xb) {
		const int32_t t = xa;
		xa = xb;
		xb = t;
	}
	int32_t i0 = (xa + (1 << (FX - 1))) >> FX;
	int32_t i1 = (xb + (1 << (FX - 1))) >> FX;
	if (i0 < 0) i0 = 0;
	if (i1 > GAME_SCREEN_W - 1) i1 = GAME_SCREEN_W - 1;
	const hstx_dvi_pixel_t p = ((const hstx_dvi_pixel_t*)d2)[0];
	for (int32_t i = i0; i <= i1; ++i) hstx_dvi_row_set_pixel(r, i, p);
}

// Cuts a line down to the part inside the view, returning false if none of it is
static bool clip_line(float* x0, float* y0, float* x1, float* y1) {
	const float dx = *x1 - *x0;
	const float dy = *y1 - *y0;
	const float p[4] = { -dx, dx, -dy, dy };
	const float q[4] = {
		*x0,
		(float)(GAME_SCREEN_W - 1) - *x0,
		*y0 - (float)VIEW_TOP,
		(float)(VIEW_BOTTOM - 1) - *y0
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
	const float ox = *x0, oy = *y0;
	*x0 = ox + dx * t0;
	*y0 = oy + dy * t0;
	*x1 = ox + dx * t1;
	*y1 = oy + dy * t1;
	return true;
}

// Adds a sprite for an edge between two points on the screen
static void add_edge(float x0, float y0, float x1, float y1, hstx_dvi_pixel_t* p) {
	if (_span_count >= MAX_EDGES) return;
	if (!clip_line(&x0, &y0, &x1, &y1)) return;
	// Top to bottom
	if (y1 < y0) {
		float t = x0; x0 = x1; x1 = t;
		t = y0; y0 = y1; y1 = t;
	}
	const int32_t top = (int32_t)lroundf(y0);
	const int32_t rows = (int32_t)lroundf(y1) - top + 1;
	WireSpan_t* s = &_spans[_span_set][_span_count];
	s->x0 = (int32_t)(x0 * (1 << FX));
	s->x1 = (int32_t)(x1 * (1 << FX));
	s->dxdy = rows > 1 ? (s->x1 - s->x0) / (rows - 1) : 0;
	s->half = s->dxdy / 2;
	s->last = rows - 1;
	const int32_t left = (int32_t)(x0 < x1 ? x0 : x1);
	const int32_t w = (int32_t)fabsf(x1 - x0) + 2;
	if (game_spr_add(left, top, w, rows, s, p, edge_renderer)) ++_span_count;
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

static void draw_ship(const Ship_t* ship, const WireModel_t* model) {
	float m[3][3];
	make_matrix(m, ship->yaw, ship->pitch, ship->roll);

	// Each corner relative to us; y is up in the model but down the screen
	Vec3_t cam[MAX_VERTS];
	for (uint32_t i = 0; i < model->nv; ++i) {
		const Vec3_t v = { model->v[i].x, model->v[i].y, model->v[i].z };
		const Vec3_t r = rotate(&v, m);
		cam[i] = (Vec3_t){ r.x + ship->pos.x, r.y + ship->pos.y, r.z + ship->pos.z };
	}

	// A face is facing us if we are in front of it
	bool facing[MAX_FACES];
	for (uint32_t i = 0; i < model->nf; ++i) {
		const WireFace_t* f = &model->f[i];
		const Vec3_t n = rotate(&(Vec3_t){ f->nx, f->ny, f->nz }, m);
		const Vec3_t* c = &cam[f->v];
		facing[i] = n.x * c->x + n.y * c->y + n.z * c->z < 0.0f;
	}

	for (uint32_t i = 0; i < model->ne; ++i) {
		const WireEdge_t* e = &model->e[i];
		if (!facing[e->f0] && !facing[e->f1]) continue;
		Vec3_t a = cam[e->v0];
		Vec3_t b = cam[e->v1];
		if (a.z < NEAR_Z && b.z < NEAR_Z) continue;
		// Cut off the part behind us
		if (a.z < NEAR_Z || b.z < NEAR_Z) {
			const float t = (NEAR_Z - a.z) / (b.z - a.z);
			const Vec3_t c = { a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, NEAR_Z };
			if (a.z < NEAR_Z) a = c; else b = c;
		}
		add_edge(
			CENTRE_X + FOCAL * a.x / a.z, CENTRE_Y - FOCAL * a.y / a.z,
			CENTRE_X + FOCAL * b.x / b.z, CENTRE_Y - FOCAL * b.y / b.z,
			&_colours[ship->colour][e->colour]);
	}
}

static float rand_range(float lo, float hi) {
	return lo + (hi - lo) * (float)(rand() & 0xffff) / 65535.0f;
}

static void new_star(Star_t* s, float z) {
	s->x = rand_range(-1600.0f, 1600.0f);
	s->y = rand_range(-1200.0f, 1200.0f);
	s->z = z;
}

// Stars streaming past, brighter as they come closer
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
	}
}

// The big ship turns in the middle while two smaller ones fly round it,
// passing above and below it
static void move_ships(uint32_t frame) {
	const float t = (float)frame / 60.0f;
	Ship_t* s = &_ships[0];
	s->pos = (Vec3_t){ 0.0f, 0.0f, 400.0f };
	s->yaw = t * 0.7f;
	s->pitch = 0.35f * sinf(t * 0.4f);
	s->roll = 0.25f * sinf(t * 0.3f);

	for (uint32_t i = 1; i < SHIPS; ++i) {
		s = &_ships[i];
		const float a = t * 0.4f + (float)i * 3.1416f;
		s->pos = (Vec3_t){ 560.0f * cosf(a), (i == 1 ? 130.0f : -120.0f) + 30.0f * sinf(a * 2.0f), 720.0f + 380.0f * sinf(a) };
		// Nose along the way it is flying, banked into the turn
		s->yaw = -a;
		s->pitch = 0.0f;
		s->roll = 0.6f;
	}
}

void main_loop(void) {
	for (uint32_t frame = 0; ; ++frame) {
		move_ships(frame);
		_span_set ^= 1;
		_span_count = 0;

		game_spr_begin();
		game_spr_text_centre(&_title, 12, game_pallet(GAME_YELLOW));
		GameText_t* status = &_status[frame & 1];
		game_spr_text_centre(status, GAME_SCREEN_H - 24, game_pallet(GAME_CYAN));
		draw_stars();
		for (uint32_t i = 0; i < SHIPS; ++i) draw_ship(&_ships[i], &ship_cobra);
		game_spr_end();

		char s[GAME_TEXT_MAX];
		snprintf(s, sizeof(s), "EDGES %3u", (unsigned)_span_count);
		game_text_set(status, s, 2);
		hstx_dvi_sprite_wait_for_frame();
	}
}

int main(void)
{
	stdio_init_all();
	app_led_on();

	game_pallet_init();
	static const uint8_t ship_rgb[SHIPS][3][3] = {
		// Hull, engines, vents and gun
		{ { 255, 255, 255 }, { 255, 146, 0 }, { 0, 219, 255 } },
		{ { 0, 255, 0 }, { 255, 146, 0 }, { 0, 219, 255 } },
		{ { 0, 219, 255 }, { 255, 146, 0 }, { 255, 255, 255 } },
	};
	for (uint32_t i = 0; i < SHIPS; ++i) {
		for (uint32_t c = 0; c < 3; ++c) {
			_colours[i][c] = hstx_dvi_pixel_rgb(ship_rgb[i][c][0], ship_rgb[i][c][1], ship_rgb[i][c][2]);
		}
		_ships[i].colour = i;
	}
	_star_colours[0] = hstx_dvi_pixel_rgb(73, 73, 109);
	_star_colours[1] = hstx_dvi_pixel_rgb(146, 146, 182);
	_star_colours[2] = hstx_dvi_pixel_rgb(255, 255, 255);
	for (uint32_t i = 0; i < STARS; ++i) new_star(&_stars[i], rand_range(NEAR_Z, 2000.0f));

	game_text_set(&_title, "COBRA MK III", 2);
	game_text_set(&_status[0], "", 2);
	game_text_set(&_status[1], "", 2);

	// Initialize the HSTX DVI sprite system
	hstx_dvi_sprite_init_all();

	sleep_ms(2000); // Allow time for initialization

	printf("HSTX DVI Wireframe Test\n");

	main_loop();
}
