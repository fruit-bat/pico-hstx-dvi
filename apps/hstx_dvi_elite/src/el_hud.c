// Copyright (c) 2024 fruit-bat.

// See el_hud.h. It is drawn in outlines, or solid when filled. Each part of it is an object of its own, as an
// object's sprite looks at all its parts on every row it covers.

#include "el_hud.h"
#include "game_pallet.h"
#include <math.h>

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

// A box that is filled or outlined
static void hud_block(bool filled, float x0, float y0, float x1, float y1, hstx_dvi_pixel_t c) {
	if (filled) add_rect(x0, y0, x1 + 1.0f, y1 + 1.0f, c);
	else add_box(x0, y0, x1, y1, c);
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
	add_box(x0, y, x1, y + GAUGE_H - 1, track);
	object_end();
}

// The speed: up to full speed along most of the bar, with the rest kept for
// the overdrive, past a mark, the whole bar red when in it
#define SPEED_SPAN 0.85f

static void hud_speed(bool filled, float x0, float x1, float y, float speed, float over,
	hstx_dvi_pixel_t c, hstx_dvi_pixel_t hot, hstx_dvi_pixel_t track) {
	if (!object_begin()) return;
	const float top = x0 + (x1 - x0) * SPEED_SPAN;
	const float x = over > 0.0f ? top + (x1 - top) * clamp01(over) : x0 + (top - x0) * clamp01(speed);
	if (x > x0 + 1.0f) hud_block(filled, x0, y, x, y + GAUGE_H - 1, over > 0.0f ? hot : c);
	add_line((Vec2_t){ top, y }, (Vec2_t){ top, y + GAUGE_H - 1 }, track);
	add_box(x0, y, x1, y + GAUGE_H - 1, track);
	object_end();
}

// A marker on a track, from -1 at the left to 1 at the right
static void hud_marker(bool filled, float x0, float x1, float y, float v, hstx_dvi_pixel_t c, hstx_dvi_pixel_t track) {
	if (!object_begin()) return;
	const float m = x0 + (x1 - x0) * 0.5f * (clamp01(0.5f + 0.5f * v) * 2.0f);
	hud_block(filled, m - 3.0f, y, m + 3.0f, y + GAUGE_H - 1, c);
	add_line((Vec2_t){ (x0 + x1) * 0.5f, y }, (Vec2_t){ (x0 + x1) * 0.5f, y + GAUGE_H - 1 }, track);
	add_box(x0, y, x1, y + GAUGE_H - 1, track);
	object_end();
}

// A ship on the scanner: a blob above or below where it is, on a stalk
static void hud_blip(const Blip_t* b, float range) {
	// Within range every way, as the stalks must stay on the scanner
	const float x = b->pos.x / range;
	const float y = b->pos.y / range;
	const float z = b->pos.z / range;
	if (x * x + y * y + z * z > 1.0f) return;
	const float sx = SCANNER_X + SCANNER_RX * x;
	const float sy = SCANNER_Y - SCANNER_RY * z;
	const float by = sy - y * SCANNER_RY;
	if (!object_begin()) return;
	const hstx_dvi_pixel_t c = shade(b->rgb, 1.0f);
	const float w = b->big ? 3.0f : 2.0f;
	add_rect(sx - w, by - 1.0f, sx + w, by + 1.0f, c);
	if (fabsf(by - sy) > 1.0f) add_line((Vec2_t){ sx, sy }, (Vec2_t){ sx, by }, c);
	object_end();
}

void hud_init(void) {
	for (uint32_t g = 0; g < GAUGES; ++g) game_text_set(&_gauge_labels[g], _gauge_names[g], 1);
}

void hud_draw(const Hud_t* hud, const Blip_t* blips, uint32_t n, bool filled) {
	const hstx_dvi_pixel_t frame_c = hud_rgb(36, 109, 219);
	const hstx_dvi_pixel_t track = hud_rgb(36, 73, 109);
	const hstx_dvi_pixel_t scan = hud_rgb(0, 146, 182);
	draw_clip_screen();

	// The sights in the middle of the view
	// The sights in the middle of the view, or green on the ship a targeting
	// computer follows
	if (hud->sights && object_begin()) {
		const hstx_dvi_pixel_t c = hud->locked ? hud_rgb(0, 255, 0) : hud_rgb(255, 255, 255);
		const float x = CENTRE_X + hud->sights_off.x, y = CENTRE_Y + hud->sights_off.y;
		add_line((Vec2_t){ x - 26, y }, (Vec2_t){ x - 8, y }, c);
		add_line((Vec2_t){ x + 8, y }, (Vec2_t){ x + 26, y }, c);
		add_line((Vec2_t){ x, y - 18 }, (Vec2_t){ x, y - 6 }, c);
		add_line((Vec2_t){ x, y + 6 }, (Vec2_t){ x, y + 18 }, c);
		object_end();
	}

	// The gauges
	const hstx_dvi_pixel_t yellow = hud_rgb(255, 255, 0);
	const hstx_dvi_pixel_t green = hud_rgb(0, 255, 0);
	const hstx_dvi_pixel_t red = hud_rgb(255, 73, 0);
	const float left[6] = { hud->fore_shield, hud->aft_shield, hud->fuel, hud->cabin_temp, hud->laser_temp, hud->altitude };
	const hstx_dvi_pixel_t left_c[6] = { yellow, yellow, green, red, red, green };
	for (uint32_t g = GAUGE_FS; g <= GAUGE_AL; ++g) {
		hud_bar(filled, GAUGE_X0, GAUGE_X1, gauge_y(g), left[g], left_c[g], track);
	}
	hud_speed(filled, GAUGE_RIGHT_X0, GAUGE_RIGHT_X1, gauge_y(0), hud->speed, hud->overdrive, yellow, red, track);
	hud_marker(filled, GAUGE_RIGHT_X0, GAUGE_RIGHT_X1, gauge_y(1), hud->roll, green, track);
	hud_marker(filled, GAUGE_RIGHT_X0, GAUGE_RIGHT_X1, gauge_y(2), hud->pitch, green, track);
	for (uint32_t b = 0; b < 4; ++b) {
		// The banks empty from the top
		hud_bar(filled, GAUGE_RIGHT_X0, GAUGE_RIGHT_X1, gauge_y(3 + b), hud->energy * 4.0f - (float)(3 - b), yellow, track);
	}

	// The ships on the scanner, over it
	for (uint32_t i = 0; i < n; ++i) hud_blip(&blips[i], hud->scanner_range);

	// The compass, with a dot towards what it points at, red when that is behind us
	if (object_begin()) {
		const Vec3_t* s = &hud->compass;
		const float l = sqrtf(s->x * s->x + s->y * s->y + s->z * s->z);
		if (l > 0.0f) {
			const float dx = COMPASS_X + COMPASS_R * draw_aspect * s->x / l;
			const float dy = COMPASS_Y - COMPASS_R * s->y / l;
			add_rect(dx - 2.0f * draw_aspect, dy - 2.0f, dx + 2.0f * draw_aspect, dy + 2.0f, s->z >= 0.0f ? green : red);
		}
		Vec2_t c[16];
		circle_points(c, 16, COMPASS_X, COMPASS_Y, COMPASS_R + 2.0f);
		for (uint32_t i = 0; i < 16; ++i) add_line(c[i], c[(i + 1) % 16], scan);
		object_end();
	}

	// The missiles, those that are left ready
	if (object_begin()) {
		for (uint32_t i = 0; i < MISSILES; ++i) {
			const float x = SCANNER_X - 112.0f + 14.0f * (float)i;
			if (i >= hud->missiles) add_box(x, DASH_BOTTOM - 14, x + 9, DASH_BOTTOM - 6, track);
			else hud_block(filled, x, DASH_BOTTOM - 14, x + 9, DASH_BOTTOM - 6, green);
		}
		object_end();
	}

	// The scanner: an ellipse, with lines showing ahead and to the sides
	if (object_begin()) {
		Vec2_t e[SCANNER_SIDES];
		ellipse_points(e, SCANNER_SIDES, SCANNER_X, SCANNER_Y, SCANNER_RX, SCANNER_RY);
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
		add_box(_panels[i][0], DASH_TOP, _panels[i][1], DASH_BOTTOM, frame_c);
		object_end();
	}

	// The labels. Text draws straight into the row, so it goes after
	// everything else to be drawn over it.
	for (uint32_t g = 0; g < GAUGES; ++g) {
		const bool right = g >= GAUGE_SP;
		const uint32_t row = right ? g - GAUGE_SP : g;
		game_spr_text(&_gauge_labels[g], right ? GAUGE_RIGHT_X0 - 22 : 8, (int32_t)gauge_y(row) + 1, game_pallet(GAME_CYAN));
		draw_count_sprite();
	}

	draw_clip_view();
}
