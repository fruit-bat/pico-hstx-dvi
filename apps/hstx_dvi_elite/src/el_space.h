// Copyright (c) 2024 fruit-bat.

// Things in space: where things are and which way they face, the dust that
// streams past as we fly, and planets.
#pragma once

#include "el_draw.h"

#ifdef __cplusplus
extern "C" {
#endif

// Where something is in space, and its axes: right, up and forward
typedef struct {
	Vec3_t pos;
	Vec3_t r, u, f;
} Frame_t;

Vec3_t vec_add(Vec3_t a, Vec3_t b);
Vec3_t vec_sub(Vec3_t a, Vec3_t b);
Vec3_t vec_scale(Vec3_t a, float k);
float vec_dot(Vec3_t a, Vec3_t b);
float vec_length(Vec3_t a);
Vec3_t vec_unit(Vec3_t a);

// Facing forward along f, with up as near u as it can be
void frame_set(Frame_t* fr, Vec3_t pos, Vec3_t f, Vec3_t u);
// Rolls to the right about the forward axis, and pitches the nose up about the right axis
void frame_roll(Frame_t* fr, float a);
void frame_pitch(Frame_t* fr, float a);
// Keeps the axes square to each other and of length 1
void frame_tidy(Frame_t* fr);
// A point in space as seen from eye: x to the right, y up and z ahead
Vec3_t frame_view(const Frame_t* eye, Vec3_t p);
// The turn from a body's model to the view from eye, for draw_model
void frame_matrix(const Frame_t* eye, const Frame_t* body, float m[3][3]);

// The dust: specks close by that stream past as we fly, and turn as we turn,
// as streaks, longer the faster we go. The specks are fixed in space, so as
// we turn and move they go the other way. Both are in the view: turn by the
// angles about x, y and z (as a roll is about z, and a pitch about x), and
// move as far as we go along each.
void dust_init(void);
void dust_move(Vec3_t turn, Vec3_t move);
// Adds them, after everything else so they are behind it all
void dust_draw(void);

// A planet at c in the view, of radius r, turned by m from its own axes to
// the view. It has a meridian and an equator, or a crater, on the side
// facing us, as in the original game.
void draw_planet(Vec3_t c, float r, const float m[3][3], const uint8_t rgb[3], bool crater, Style_e style);

#ifdef __cplusplus
}
#endif
