// Copyright (c) 2024 fruit-bat.

// Drawing in 3D with sprites, as in the wireframe test.
//
// Each object is a sprite, with a list of the lines and convex shapes that
// make it up, drawn by sprite_renderer_parts. Objects added earlier keep
// priority over those added later, so the nearest are added first and no
// depth buffer is needed.
//
// The renderer on core 1 reads the objects of the frame it is drawing while
// this core works out the next, so there are two sets, swapped by
// draw_frame_begin.
#pragma once

#include "hstx_dvi_sprite.h"
#include "game_render.h"
#include "ship_model.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// The view, between the text at the top and the dashboard
#define VIEW_TOP 36
#define VIEW_BOTTOM 332
#define CENTRE_X (GAME_SCREEN_W / 2)
#define CENTRE_Y ((VIEW_TOP + VIEW_BOTTOM) / 2)
// The distance to the screen, in pixels, which sets the field of view
#define FOCAL 320.0f
// Nothing closer than this is drawn
#define NEAR_Z 16.0f
#define PI_F 3.14159265f

typedef enum {
	STYLE_SOLID = 0,
	STYLE_WIRE,
	STYLE_OUTLINED,
	STYLE_FILLED,
	STYLE_COUNT
} Style_e;

extern const char* const style_names[STYLE_COUNT];

typedef struct {
	float x, y, z;
} Vec3_t;

typedef struct {
	float x, y;
} Vec2_t;

// The way of drawing chosen, and whether lit faces are dithered, for more shades
extern Style_e draw_style;
extern bool draw_dither;
// Towards the light, in the view
extern Vec3_t draw_light;
// How wide a pixel is drawn for its height. The display is 4:3, and a 16:9
// screen stretches it across, so everything is drawn narrower to look right.
#define ASPECT_4_3 1.0f
#define ASPECT_16_9 0.75f
extern float draw_aspect;

// Starts and ends the sprites of a frame
void draw_frame_begin(void);
void draw_frame_end(void);
// 0 or 1, which set of objects is being made. Text a frame draws must be kept
// until the renderer has drawn it, so it alternates between two too.
uint32_t draw_set(void);
// For sprites added other than through the objects, such as text, so they are counted
void draw_count_sprite(void);

typedef struct {
	uint32_t parts, sprites, hidden;
} DrawStats_t;

DrawStats_t draw_stats(void);

// Lines and shapes are cut to the view, or to the whole screen for the dashboard
void draw_clip_view(void);
void draw_clip_screen(void);

// An object is made of the lines and shapes added between these. Returns false if there is no room.
bool object_begin(void);
void object_end(void);

// A line on the screen
void add_line(Vec2_t p0, Vec2_t p1, hstx_dvi_pixel_t colour);
// A line between two points in the view, cut off where it goes behind us
void add_edge(Vec3_t a, Vec3_t b, hstx_dvi_pixel_t colour);
// A convex shape on the screen, its corners in order round it
void add_fill(const Vec2_t* corners, uint32_t n, SpriteColour_t colour);
// A flat convex shape in the view, cut off where it goes behind us
void add_shape(const Vec3_t* corners, uint32_t n, SpriteColour_t colour);

// The outline of something solid on the screen, for leaving out what is
// wholly behind it. Add the nearest first.
void add_occluder(const Vec2_t* p, uint32_t n);
// Whether a circle on the screen is wholly behind an occluder
bool hidden(Vec2_t c, float r);

Vec2_t project(const Vec3_t* v);
Vec3_t rotate(const Vec3_t* v, const float m[3][3]);
// Roll about z, then pitch about x, then yaw about y
void make_matrix(float m[3][3], float yaw, float pitch, float roll);
// The outline round some points, in order. Returns how many corners it has.
uint32_t outline(Vec2_t* p, uint32_t n, Vec2_t* out);
// The edges of a box, and a box filled from x0 up to x1 and y0 up to y1
void add_box(float x0, float y0, float x1, float y1, hstx_dvi_pixel_t c);
void add_rect(float x0, float y0, float x1, float y1, hstx_dvi_pixel_t c);
// n points round an ellipse. circle_points is round on the screen, r tall.
void circle_points(Vec2_t* p, uint32_t n, float cx, float cy, float r);
void ellipse_points(Vec2_t* p, uint32_t n, float cx, float cy, float rx, float ry);

hstx_dvi_pixel_t shade(const uint8_t rgb[3], float k);
// The colour of a lit face, dithered if draw_dither is set
SpriteColour_t face_colour(const uint8_t rgb[3], float k);

// A model at pos in the view, turned by m from the model to the view.
// draw_object makes it an object of its own.
void draw_model(const WireModel_t* model, Vec3_t pos, const float m[3][3], const uint8_t rgb[3], Style_e style);
void draw_object(const WireModel_t* model, Vec3_t pos, const float m[3][3], const uint8_t rgb[3], Style_e style);

// A sun at m on the screen, rp across, with rays that reach out to
// SUN_RAY_REACH times that
#define SUN_RAY_REACH 1.8f
void draw_sun(Vec2_t m, float rp, Style_e style, uint32_t frame);

#ifdef __cplusplus
}
#endif
