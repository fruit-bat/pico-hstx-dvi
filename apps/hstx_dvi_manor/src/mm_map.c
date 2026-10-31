#include "mm_map.h"
#include "mm_thing.h"
#include "game_fx.h"
#include "game_pallet.h"
#include "game_util.h"
#include <string.h>

// The house as it starts each night. Each character is a tile:
//
//   ~ sky              # stone wall       = floor           ^ roof
//   _ grass            - shelf            T table           Z bed
//   W window           P portrait         % cobweb          R books
//   D front door       Q lantern, where Hazel starts again after losing a life
//   1-9 gates          a-i what opens the gate of the same number, see _group_type
//
// and things that move about or can be picked up:
//
//   @ Hazel            G ghost            V bat             S spider
//   J pumpkin          K kitten           X crate           * sweet
//   + heart            F the float spell
//
// Things stand on the tile below them; spiders hang from the one above.
static const char* const _house[MM_MAP_H] = {
	"~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~",
	"~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~",
	"~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~",
	"~~~~~~~~~~~~~~~~~~~~^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^",
	"~~~~~~~~~~~~~~~~~~~~^         #                                                       %#",
	"~~~~~~~~~~~~~~~~~~~^          #                                                        #",
	"~~~~~~~~~~~~~~~~~~^           #         V                                              #",
	"~~~~~~~~~~~~~~~~~^            #   e                                                    #",
	"~~~~~~~~~~~~~~~~^             #                                                        #",
	"~~~~~~~~~~~~~~~^              #                              e                         #",
	"~~~~~~~~~~~~~~^               5      *                ***                              #",
	"~~~~~~~~~~~~~^                5     ---                                                #",
	"~~~~~~~~~~~~^         * K *   5  +      *   J   *             *   G   *    J         Q #",
	"~~~~~~~~~~~~========================================       =====================    ====",
	"~~~~~~~~~~~~#%                                 #                %#      S       ---    #",
	"~~~~~~~~~~~~#                                  #                 #                     #",
	"~~~~~~~~~~~~#                                  #          V      #               ----  #",
	"~~~~~~~~~~~~#  W               W           P   #     V  P        # W        P          #",
	"~~~~~~~~~~~~#  W               W        V      #              F  # W          ----     #",
	"~~~~~~~~~~~~#           **                     #            ---  #                     #",
	"~~~~~~~~~~~~#          ----                    #        *        #                -----#",
	"~~~~~~~~~~~~#       *                          4       ---            * * *            #",
	"~~~~~~~~~~~~#     ----                         4  *                          ----      #",
	"~~~~~~~~~~~~# +  *         G *           * Q   4 ZZZZ Q    *                  K      * #",
	"~~~~~~~~~~~~========================    =============================       ============",
	"~~~~~~~~~~~~#%                 %#    ---              %#                      %        #",
	"~~~~~~~~~~~~#                   #      *               #                               #",
	"~~~~~~~~~~~~#                   #    -----            a#                       K       #",
	"~~~~~~~~~~~~#  W     P     W    #  *        W   P      #    W      W         ----     b#",
	"~~~~~~~~~~~~#  W           W    #----       W   V      #    W      W               *   #",
	"~~~~~~~~~~~~# K                 #     *                #                          ---  #",
	"~~~~~~~~~~~~#----  *            #    ----              #               b       *       #",
	"~~~~~~~~~~~~D                     *                    1                      ---      #",
	"~~~~~~~~~~~~D           *        ----       RRRRRRRR   1        b          *           #",
	"~~~~@~*~Q~*~D    * G * TTT  * Q           Q RRRRRRRR*  1 *     Q  J  *  * TTT    *     #",
	"____________==============================================2222==========================",
	"#############%        S          S      #                 ---             S           %#",
	"#############                           #                                              #",
	"#############                           #                   ----                       #",
	"#############                           #                                              #",
	"#############                           #               ----                           #",
	"#############              d            3                                              #",
	"#############            ----           3                    ----                      #",
	"#############   K  *          *    *    3      *  X  Q              *  *      *   +    #",
	"############################################c###########################################",
};

// What opens each gate. Keys are picked up, so their letter is where the key starts.
static const uint8_t _group_type[MM_GROUPS] = {
	MM_GROUP_NONE,
	MM_GROUP_LEVER, // 1, the library to the kitchen
	MM_GROUP_CANDLES, // 2, the trapdoor to the cellar
	MM_GROUP_PLATE, // 3, the far end of the cellar
	MM_GROUP_KEY, // 4, the bedroom
	MM_GROUP_CANDLES, // 5, the end of the attic
	MM_GROUP_NONE,
	MM_GROUP_NONE,
	MM_GROUP_NONE,
	MM_GROUP_NONE,
};

#define MM_PLATES 8

typedef struct {
	bool open;
	uint8_t candles;
	uint8_t lit;
} MmGroup_t;

static uint8_t _kind[MM_MAP_H][MM_MAP_W];
static uint8_t _cell_group[MM_MAP_H][MM_MAP_W];
// Levers pulled, candles and lanterns lit and plates held down
static uint8_t _on[MM_MAP_H][MM_MAP_W];
static uint8_t _gfx[MM_MAP_H][MM_MAP_W];
static MmGroup_t _group[MM_GROUPS];
static uint8_t _plate[MM_PLATES][2];
static uint32_t _plates = 0;

static char ch_at(int32_t cx, int32_t cy) {
	if (cx < 0 || cy < 0 || cx >= MM_MAP_W || cy >= MM_MAP_H) return '#';
	return _house[cy][cx];
}

static bool is_wall_ch(char ch) {
	return ch == '#' || ch == '=' || ch == '^' || ch == '_';
}

static uint32_t style_at(int32_t cx, int32_t cy) {
	if (ch_at(cx, cy) == '~' || (cx < 12 && cy < MM_MAP_GROUND)) return MM_STYLE_SKY;
	// The floors take the wallpaper of the room under them, which shows through holes
	if (cy <= 12) return MM_STYLE_ATTIC;
	if (cy <= 23) return MM_STYLE_UPPER;
	if (cy <= 34) return MM_STYLE_GROUND;
	return MM_STYLE_CELLAR;
}

// Tables and beds have ends
static uint32_t ends(int32_t cx, int32_t cy, uint32_t l) {
	const char ch = ch_at(cx, cy);
	if (ch_at(cx - 1, cy) != ch) return l;
	if (ch_at(cx + 1, cy) != ch) return l + 2;
	return l + 1;
}

static uint32_t decor_at(int32_t cx, int32_t cy) {
	const char ch = ch_at(cx, cy);
	switch (ch) {
	case '#': return cx < 12 && cy > MM_MAP_GROUND ? MM_DECOR_EARTH : MM_DECOR_STONE;
	case '=': return MM_DECOR_FLOOR;
	case '^': return MM_DECOR_ROOF;
	case '_': return MM_DECOR_GRASS;
	case '-': return MM_DECOR_SHELF;
	case 'T': return ends(cx, cy, MM_DECOR_TABLE_L);
	case 'Z': return ends(cx, cy, MM_DECOR_BED_L);
	case 'W': return ch_at(cx, cy - 1) == 'W' ? MM_DECOR_WINDOW_BOTTOM : MM_DECOR_WINDOW_TOP;
	case 'P': return MM_DECOR_PORTRAIT;
	case '%': return is_wall_ch(ch_at(cx + 1, cy)) ? MM_DECOR_COBWEB_R : MM_DECOR_COBWEB_L;
	case 'R': return MM_DECOR_BOOKS;
	case 'D': return ch_at(cx, cy - 1) == 'D' ? MM_DECOR_DOORWAY : MM_DECOR_DOORWAY_TOP;
	case 'Q': return _on[cy][cx] ? MM_DECOR_LANTERN_LIT : MM_DECOR_LANTERN;
	default: break;
	}
	const uint32_t g = _cell_group[cy][cx];
	switch (_kind[cy][cx]) {
	case MM_C_GATE:
		if (_group[g].open) return MM_DECOR_NONE;
		if (_group_type[g] == MM_GROUP_KEY) {
			if (ch_at(cx, cy - 1) != ch) return MM_DECOR_DOOR_TOP;
			if (ch_at(cx, cy + 1) != ch) return MM_DECOR_DOOR_BOTTOM;
			return MM_DECOR_DOOR_MID;
		}
		// Gates that lie across, in a floor, are trapdoors
		return ch_at(cx, cy - 1) != ch && ch_at(cx, cy + 1) != ch ? MM_DECOR_TRAPDOOR : MM_DECOR_BARS;
	case MM_C_LEVER: return _on[cy][cx] ? MM_DECOR_LEVER_ON : MM_DECOR_LEVER;
	case MM_C_CANDLE: return _on[cy][cx] ? MM_DECOR_CANDLE_LIT : MM_DECOR_CANDLE;
	case MM_C_PLATE: return _on[cy][cx] ? MM_DECOR_PLATE_DOWN : MM_DECOR_PLATE;
	default: return MM_DECOR_NONE;
	}
}

static void update_gfx(int32_t cx, int32_t cy) {
	const uint32_t decor = decor_at(cx, cy);
	// A shadow under anything solid
	const bool shadow = is_wall_ch(ch_at(cx, cy - 1)) && !is_wall_ch(ch_at(cx, cy));
	_gfx[cy][cx] = mm_tiles_get(style_at(cx, cy), decor, shadow);
}

void mm_map_reset(MmSpawn_t spawn) {
	memset(_group, 0, sizeof(_group));
	memset(_on, 0, sizeof(_on));
	_plates = 0;
	for (int32_t cy = 0; cy < MM_MAP_H; ++cy) {
		for (int32_t cx = 0; cx < MM_MAP_W; ++cx) {
			const char ch = _house[cy][cx];
			uint8_t kind = MM_C_EMPTY;
			uint8_t g = 0;
			if (is_wall_ch(ch)) kind = MM_C_SOLID;
			else if (ch == '-' || ch == 'T' || ch == 'Z') kind = MM_C_PLATFORM;
			else if (ch == 'Q') kind = MM_C_LANTERN;
			else if (ch >= '1' && ch <= '9') {
				kind = MM_C_GATE;
				g = ch - '0';
			}
			else if (ch >= 'a' && ch <= 'i') {
				g = ch - 'a' + 1;
				switch (_group_type[g]) {
				case MM_GROUP_LEVER: kind = MM_C_LEVER; break;
				case MM_GROUP_CANDLES: kind = MM_C_CANDLE; ++_group[g].candles; break;
				case MM_GROUP_PLATE:
					kind = MM_C_PLATE;
					if (_plates < MM_PLATES) {
						_plate[_plates][0] = cx;
						_plate[_plates][1] = cy;
						++_plates;
					}
					break;
				case MM_GROUP_KEY: spawn('k', cx, cy, g); break;
				default: break;
				}
			}
			else if (strchr("@GVSJKX*+F", ch)) spawn(ch, cx, cy, 0);
			_kind[cy][cx] = kind;
			_cell_group[cy][cx] = g;
		}
	}
	for (int32_t cy = 0; cy < MM_MAP_H; ++cy) {
		for (int32_t cx = 0; cx < MM_MAP_W; ++cx) update_gfx(cx, cy);
	}
}

const uint8_t* mm_map_gfx(void) {
	return &_gfx[0][0];
}

uint32_t mm_map_kind(int32_t cx, int32_t cy) {
	if (cx < 0 || cy < 0 || cx >= MM_MAP_W || cy >= MM_MAP_H) return MM_C_SOLID;
	return _kind[cy][cx];
}

uint32_t mm_map_group(int32_t cx, int32_t cy) {
	if (cx < 0 || cy < 0 || cx >= MM_MAP_W || cy >= MM_MAP_H) return 0;
	return _cell_group[cy][cx];
}

uint32_t mm_map_group_type(uint32_t group) {
	return group < MM_GROUPS ? _group_type[group] : MM_GROUP_NONE;
}

bool mm_map_group_open(uint32_t group) {
	return group < MM_GROUPS && _group[group].open;
}

bool mm_map_solid(int32_t cx, int32_t cy) {
	if (cx < 0 || cy < 0 || cx >= MM_MAP_W || cy >= MM_MAP_H) return true;
	const uint32_t k = _kind[cy][cx];
	return k == MM_C_SOLID || (k == MM_C_GATE && !_group[_cell_group[cy][cx]].open);
}

bool mm_map_platform(int32_t cx, int32_t cy) {
	return mm_map_kind(cx, cy) == MM_C_PLATFORM;
}

bool mm_map_box_solid(int32_t x0, int32_t y0, int32_t x1, int32_t y1) {
	for (int32_t cy = y0 >> MM_TILE_SHIFT; cy <= (y1 - 1) >> MM_TILE_SHIFT; ++cy) {
		for (int32_t cx = x0 >> MM_TILE_SHIFT; cx <= (x1 - 1) >> MM_TILE_SHIFT; ++cx) {
			if (mm_map_solid(cx, cy)) return true;
		}
	}
	return false;
}

static void sparkle(int32_t cx, int32_t cy, uint32_t colour) {
	game_fx_explode(GAME_PX(cx * MM_TILE + MM_TILE / 2), GAME_PX(cy * MM_TILE + MM_TILE / 2), colour, 10, GAME_PX(2));
}

static void set_group(uint32_t g, bool open) {
	if (_group[g].open == open) return;
	_group[g].open = open;
	for (int32_t cy = 0; cy < MM_MAP_H; ++cy) {
		for (int32_t cx = 0; cx < MM_MAP_W; ++cx) {
			if (_kind[cy][cx] != MM_C_GATE || _cell_group[cy][cx] != g) continue;
			update_gfx(cx, cy);
			if (open) sparkle(cx, cy, GAME_GREY);
		}
	}
}

bool mm_map_on(int32_t cx, int32_t cy) {
	if (cx < 0 || cy < 0 || cx >= MM_MAP_W || cy >= MM_MAP_H) return false;
	return _on[cy][cx] != 0;
}

uint32_t mm_map_cast(int32_t cx, int32_t cy) {
	const uint32_t k = mm_map_kind(cx, cy);
	if ((k != MM_C_LEVER && k != MM_C_CANDLE) || _on[cy][cx]) return MM_CAST_NOTHING;
	const uint32_t g = _cell_group[cy][cx];
	_on[cy][cx] = 1;
	update_gfx(cx, cy);
	if (k == MM_C_LEVER) {
		sparkle(cx, cy, GAME_GREEN);
		set_group(g, true);
		return MM_CAST_LEVER;
	}
	sparkle(cx, cy, GAME_YELLOW);
	if (++_group[g].lit < _group[g].candles) return MM_CAST_CANDLE;
	set_group(g, true);
	return MM_CAST_LAST_CANDLE;
}

void mm_map_update_plates(void) {
	for (uint32_t i = 0; i < _plates; ++i) {
		const int32_t cx = _plate[i][0], cy = _plate[i][1];
		const bool down = mm_thing_crate_in(cx, cy);
		if (_on[cy][cx] == down) continue;
		_on[cy][cx] = down;
		update_gfx(cx, cy);
		if (down) sparkle(cx, cy, GAME_GREEN);
		set_group(_cell_group[cy][cx], down);
	}
}

bool mm_map_light_lantern(int32_t cx, int32_t cy) {
	if (mm_map_kind(cx, cy) != MM_C_LANTERN || _on[cy][cx]) return false;
	_on[cy][cx] = 1;
	update_gfx(cx, cy);
	sparkle(cx, cy, GAME_YELLOW);
	return true;
}

void mm_map_unlock(uint32_t group) {
	if (group < MM_GROUPS) set_group(group, true);
}
