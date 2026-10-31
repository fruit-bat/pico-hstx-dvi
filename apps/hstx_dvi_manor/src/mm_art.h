#pragma once

#include "pico/stdlib.h"
#include "mm_gfx.h"

#ifdef __cplusplus
extern "C" {
#endif

// All the pictures, drawn twice their size in the house

// Hazel, facing right
enum {
	MM_HAZEL_STAND = 0,
	MM_HAZEL_WALK1,
	MM_HAZEL_WALK2,
	MM_HAZEL_JUMP,
	MM_HAZEL_FALL,
	MM_HAZEL_CAST,
	MM_HAZEL_DIZZY,
	MM_HAZEL_FRONT,
	MM_HAZEL_CHEER,
	MM_HAZEL_FRAMES
};
extern MmImg_t mm_art_hazel[MM_HAZEL_FRAMES];
extern MmPal_t mm_pal_hazel;

// Creatures are grumpy until a spell cheers them up
enum {
	MM_MOOD_GRUMPY = 0,
	MM_MOOD_HAPPY,
	MM_MOODS
};
// Two frames of each
extern MmImg_t mm_art_ghost[MM_MOODS][2];
extern MmPal_t mm_pal_ghost[MM_MOODS];
extern MmImg_t mm_art_bat[MM_MOODS][2];
extern MmPal_t mm_pal_bat;
extern MmImg_t mm_art_spider[MM_MOODS][2];
extern MmPal_t mm_pal_spider;
extern MmImg_t mm_art_pumpkin[MM_MOODS][2];
extern MmPal_t mm_pal_pumpkin[MM_MOODS];

#define MM_KITTEN_COATS 5
extern MmImg_t mm_art_kitten[2];
extern MmPal_t mm_pal_kitten[MM_KITTEN_COATS];

#define MM_SWEET_COLOURS 4
extern MmImg_t mm_art_sweet;
extern MmPal_t mm_pal_sweet[MM_SWEET_COLOURS];
extern MmImg_t mm_art_heart;
extern MmPal_t mm_pal_heart;
extern MmImg_t mm_art_key;
extern MmPal_t mm_pal_key;
extern MmImg_t mm_art_feather;
extern MmPal_t mm_pal_feather;
extern MmImg_t mm_art_crate;
extern MmPal_t mm_pal_crate;
extern MmImg_t mm_art_star[2];
extern MmPal_t mm_pal_star;
extern MmImg_t mm_art_moon;
extern MmPal_t mm_pal_moon;
// For the HUD
extern MmImg_t mm_art_hat;
extern MmImg_t mm_art_kitten_face;
extern MmPal_t mm_pal_kitten_none;

// Every colour white, for flashing things
extern MmPal_t mm_pal_white;

void mm_art_init(void);

#ifdef __cplusplus
}
#endif
