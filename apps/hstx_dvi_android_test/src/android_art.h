#pragma once

#include "pico/stdlib.h"

#ifdef __cplusplus
extern "C" {
#endif

// The android, facing right, from art/android_art.py
#define ANDROID_W 32
#define ANDROID_BODY_H 48
#define ANDROID_HAIR_H 32
#define ANDROID_WALK_FRAMES 8
#define ANDROID_HAIR_FRAMES 8

// A character for each layer, see game_art_make
extern const char* const android_body_key;
extern const char* const android_hair_key;

extern const int32_t android_cx;
// The hair goes down with the body
extern const int8_t android_bob[ANDROID_WALK_FRAMES];
extern const int16_t android_step[ANDROID_WALK_FRAMES];

const char* const* android_body_rows(uint32_t frame);
const char* const* android_hair_rows(uint32_t frame);

#ifdef __cplusplus
}
#endif
