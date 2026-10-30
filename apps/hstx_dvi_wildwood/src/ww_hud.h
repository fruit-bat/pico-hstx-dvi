#pragma once

#include "pico/stdlib.h"

#ifdef __cplusplus
extern "C" {
#endif

// An extra life each time the score passes a multiple of this
#define WW_HUD_BONUS 10000
#define WW_HUD_START_LIVES 3
#define WW_HUD_MAX_LIVES 5

// The moon charm comes in pieces, one for each way of helping out
#define WW_PIECE_HEAL 0
#define WW_PIECE_CALM 1
#define WW_PIECE_HELP 2
#define WW_PIECE_HUSH 3
#define WW_PIECES 4
#define WW_PIECES_ALL ((1u << WW_PIECES) - 1)

void ww_hud_init(void);
void ww_hud_reset(void);

void ww_hud_add_score(uint32_t points);
uint32_t ww_hud_score(void);
void ww_hud_set_score(uint32_t score);

uint32_t ww_hud_lives(void);
// Returns false if there were none left
bool ww_hud_take_life(void);

uint32_t ww_hud_pieces(void);
void ww_hud_add_piece(uint32_t i);
void ww_hud_clear_pieces(void);
bool ww_hud_charm_whole(void);
// Where a piece goes in the HUD, on the screen
void ww_hud_piece_pos(uint32_t i, int32_t* x, int32_t* y);
// One piece of the charm, sx and sy being its top left on the screen
void ww_hud_draw_piece(uint32_t i, int32_t sx, int32_t sy);
// A picture of the whole charm, for outside of the game
void ww_hud_draw_charm(int32_t x, int32_t y, uint32_t pieces);

// The hearts and lives are only shown when in_game is set
void ww_hud_draw(uint32_t night, uint32_t frame, bool in_game);

// Words said by the animals and the witch, floating up from a point on the screen
void ww_hud_say_clear(void);
void ww_hud_say(int32_t x, int32_t y, const char* s, uint32_t colour);
void ww_hud_say_update(void);
void ww_hud_say_draw(void);

#ifdef __cplusplus
}
#endif
