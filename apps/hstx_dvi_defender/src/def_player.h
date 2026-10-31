#pragma once

#include "pico/stdlib.h"
#include "hstx_dvi_sprite.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DEF_SHIP_W 32
#define DEF_SHIP_H 13
// The row the lasers come out of
#define DEF_SHIP_NOSE 7
// The top of the exhaust flame, from the top of the ship
#define DEF_SHIP_ENGINE 5

void def_player_init(void);

// input is the button state and pressed the buttons that have just gone down
void def_player_update(uint32_t frame, uint8_t input, uint8_t pressed);

void def_player_draw(uint32_t frame);
void def_player_draw_lasers(uint32_t frame);

void def_player_kill(void);
void def_player_clear_lasers(void);

bool def_player_alive(void);
// True once the last ship has gone
bool def_player_over(void);
int32_t def_player_x(void);
int32_t def_player_y(void);
int32_t def_player_vx(void);
bool def_player_facing_right(void);

// Checks for a box, in world x and screen y, overlapping the ship
bool def_player_hit(int32_t x, int32_t y, int32_t w, int32_t h);

// Checks for a box being hit by a laser beam. The beam is used up if it is.
bool def_player_laser_hit(int32_t x, int32_t y, int32_t w, int32_t h);

Tile16x8p2_t* def_player_icon(void);

#ifdef __cplusplus
}
#endif
