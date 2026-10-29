#include "inv_mot.h"
#include "game_pallet.h"
#include "inv_input.h"
#include "inv_bullets.h"
#include "inv_collisions.h"
#include "inv_gun.h"
#include "game_art.h"
#include "game_fx.h"
#include "game_util.h"

#define INV_GUN_COOLDOWN 20
#define INV_GUN_Y (MODE_V_ACTIVE_LINES - 64)
#define INV_GUN_MAX_LIVES 9

typedef enum  {
	GUN_STATE_ALIVE = 0,
	GUN_STATE_EXPLODE,
	GUN_STATE_WAIT,
	GUN_STATE_DEAD
 } InvGunState_e;

typedef struct {
	InvGunState_e state;
	uint32_t end;
} InvGunState_t;

static SpriteId _sprite_index = 0;
static uint32_t _cooldown = 0;
static uint32_t _lives = 0;
static InvGunState_t _gun_state;

#define INV_GUN_LAYERS 3
#define INV_GUN_SPEED 3
#define INV_GUN_START_X 20

static const char* const _art_gun_rows[] = {
	"...........WW...........",
	"...........WW...........",
	"..........WWWW..........",
	".........GGGGGG.........",
	"...GGGGGGGGGGGGGGGGGG...",
	"..GGGGGGGGGGGGGGGGGGGG..",
	".GGGGGCCGGGGGGGGCCGGGGG.",
	".GGGGGGGGGGGGGGGGGGGGGG.",
	".GGGGGGGGGGGGGGGGGGGGGG.",
	"GGGGGGGGGGGGGGGGGGGGGGGG",
};

static const uint8_t _gun_colours[INV_GUN_LAYERS] = { GAME_GREEN, GAME_WHITE, GAME_CYAN };
// Flickering while it blows up
static const uint8_t _gun_burning[2][INV_GUN_LAYERS] = {
	{ GAME_RED, GAME_YELLOW, GAME_ORANGE },
	{ GAME_ORANGE, GAME_RED, GAME_YELLOW },
};

static GameArt_t _art_gun;
static bool _art_made = false;
static int32_t _x = INV_GUN_START_X;

static void gun_draw(const uint8_t* colours) {
	game_art_sprites(_sprite_index, INV_GUN_LAYERS, &_art_gun, _x, INV_GUN_Y, colours);
}

static SpriteCollisionMask gun_collisions(void) {
	SpriteCollisionMask m = 0;
	for (uint32_t l = 0; l < INV_GUN_LAYERS; ++l) m |= _spriteCollisions.m[_sprite_index + l];
	return m;
}

SpriteId inv_gun_init(SpriteId start) {
	if (!_art_made) {
		game_art_make(&_art_gun, _art_gun_rows, GAME_ART_ROWS(_art_gun_rows), "GWC", _gun_colours);
		_art_made = true;
	}
	_sprite_index = start;
	_x = INV_GUN_START_X;
	for (uint32_t l = 0; l < INV_GUN_LAYERS; ++l) {
		init_sprite(start + l, 0, 0, 0, 0, 0, NULL, NULL, NULL);
		hstx_dvi_sprite_set_sprite_collision_mask(start + l, INV_GUN_COLLISION_MASK);
	}
	gun_draw(NULL);

	_cooldown = 0;
	_lives = 3;
	_gun_state.state = GUN_STATE_ALIVE;
	_gun_state.end = 0;

	return start + INV_GUN_LAYERS;
}

static void inv_gun_explode(const uint32_t frame) {
	if (_gun_state.state != GUN_STATE_ALIVE) return;
	const int32_t cx = GAME_PX(_x + (INV_GUN_W >> 1));
	const int32_t cy = GAME_PX(INV_GUN_Y + 5);
	game_fx_explode(cx, cy, GAME_COLOUR_RANDOM, 40, GAME_PX(3));
	game_fx_explode(cx, cy, GAME_WHITE, 16, GAME_PX(1));
	_gun_state.state = GUN_STATE_EXPLODE;
	_gun_state.end = frame + 40;
}

void __not_in_flash_func(inv_gun_update)(uint32_t frame, uint8_t input) {
	switch(_gun_state.state) {
		case GUN_STATE_ALIVE: {
			if (gun_collisions() & (INV_BOMB_COLLISION_MASK | INV_INVADER_COLLISION_MASK)) {
				inv_gun_explode(frame);
				break;
			}
			if (is_inv_input_left(input)) {
				_x -= INV_GUN_SPEED;
				if (_x < 0) _x = 0;
			}
			if (is_inv_input_right(input)) {
				_x += INV_GUN_SPEED;
				if (_x + INV_GUN_W > MODE_H_ACTIVE_PIXELS) _x = MODE_H_ACTIVE_PIXELS - INV_GUN_W;
			}
			if (_cooldown) {
				--_cooldown;
			}
			else if (is_inv_input_fire(input)) {
				inv_bullets_fire(_x + (INV_GUN_W >> 1) - 1, INV_GUN_Y);
				_cooldown = INV_GUN_COOLDOWN;
			}
			gun_draw(NULL);
			break;
		}
		case GUN_STATE_EXPLODE: {
			const int32_t r = (int32_t)(_gun_state.end - frame);
			if (r < 0) {
				game_art_sprites_hide(_sprite_index, INV_GUN_LAYERS);
				if (--_lives > 0) {
					_gun_state.state = GUN_STATE_WAIT;
					_gun_state.end = frame + 60; // Wait for 60 frames
				}
				else {
					// Gun is dead
					_gun_state.state = GUN_STATE_DEAD;
				}
			}
			else {
				gun_draw(_gun_burning[(r >> 2) & 1]);
			}
			break;
		}
		case GUN_STATE_WAIT: {
			const int32_t r = (int32_t)(_gun_state.end - frame);
			if (r < 0) {
				// Reset the gun
				_x = INV_GUN_START_X;
				_gun_state.state = GUN_STATE_ALIVE;
				gun_draw(NULL);
			}
			break;
		}
		case GUN_STATE_DEAD: {
			// Do nothing, the sprite is already disabled. The game loop ends
			// once the lives reach zero; the score is reset by inv_score_init.
			break;
		}
		default: {
			break;
		}
	}
}

uint32_t __not_in_flash_func(inv_gun_get_lives)(void) {
	return _lives;
}

void inv_gun_add_life(void) {
	if (_lives < INV_GUN_MAX_LIVES) ++_lives;
}

void inv_gun_destroy(uint32_t frame) {
	switch(_gun_state.state) {
		case GUN_STATE_ALIVE: {
			_lives = 1;
			inv_gun_explode(frame);
			break;
		}
		case GUN_STATE_EXPLODE: {
			// The life is taken when the explosion ends
			_lives = 1;
			break;
		}
		case GUN_STATE_WAIT: {
			_lives = 0;
			_gun_state.state = GUN_STATE_DEAD;
			break;
		}
		default: {
			break;
		}
	}
}

bool __not_in_flash_func(inv_gun_is_alive)(void) {
	return _gun_state.state == GUN_STATE_ALIVE;
}

int32_t __not_in_flash_func(inv_gun_get_x)(void) {
	return _x;
}

int32_t __not_in_flash_func(inv_gun_get_y)(void) {
	return INV_GUN_Y;
}
