#include "ww_world.h"
#include "game_util.h"
#include "game_render.h"
#include <string.h>

#define WW_MAP_PX_W 64
#define WW_MAP_PX_H 30
#define WW_MAP_CELL_W 8
#define WW_MAP_CELL_H 5
// Rooms with fewer open tiles than this are too cramped
#define WW_ROOM_MIN_OPEN 100
#define WW_SPOT_TRIES 60

typedef struct {
	uint8_t exits;
	uint8_t visits;
	uint8_t map[WW_ROOM_CELLS];
} WwRoom_t;

static WwRoom_t _rooms[WW_ROOMS];
static uint32_t _start = 0;
static uint32_t _room = 0;
static uint32_t _seed = 0;
// Drawn from one while the other is being set up
static uint8_t _gfx[2][WW_ROOM_CELLS];
static uint32_t _gfx_i = 0;
static uint32_t _map_px[2][WW_MAP_PX_H * WW_MAP_PX_W / 4];
static uint32_t _map_i = 0;
static GamePixels_t _map_pixels[2];

static const int8_t _dc[4] = { 0, 1, 0, -1 };
static const int8_t _dr[4] = { -1, 0, 1, 0 };

uint32_t ww_world_next(uint32_t room, uint32_t dir) {
	if (room >= WW_ROOMS || !(_rooms[room].exits & (1u << dir))) return WW_NO_ROOM;
	const int32_t mx = (int32_t)(room % WW_MAP_W) + _dc[dir];
	const int32_t my = (int32_t)(room / WW_MAP_W) + _dr[dir];
	if (mx < 0 || my < 0 || mx >= WW_MAP_W || my >= WW_MAP_H) return WW_NO_ROOM;
	return my * WW_MAP_W + mx;
}

static void join(uint32_t room, uint32_t dir) {
	const uint32_t mx = room % WW_MAP_W + _dc[dir];
	const uint32_t my = room / WW_MAP_W + _dr[dir];
	_rooms[room].exits |= 1u << dir;
	_rooms[my * WW_MAP_W + mx].exits |= 1u << ((dir + 2) & 3);
}

static bool on_map(uint32_t room, uint32_t dir) {
	const int32_t mx = (int32_t)(room % WW_MAP_W) + _dc[dir];
	const int32_t my = (int32_t)(room / WW_MAP_W) + _dr[dir];
	return mx >= 0 && my >= 0 && mx < WW_MAP_W && my < WW_MAP_H;
}

// A maze with every room reachable, then a few extra ways through so it is not too much of a trek
static void make_maze(void) {
	uint8_t stack[WW_ROOMS];
	bool seen[WW_ROOMS] = { false };
	uint32_t n = 0;
	stack[n++] = _start;
	seen[_start] = true;
	while (n) {
		const uint32_t room = stack[n - 1];
		uint32_t dirs[4];
		uint32_t nd = 0;
		for (uint32_t d = 0; d < 4; ++d) {
			if (!on_map(room, d)) continue;
			const uint32_t next = room + _dc[d] + _dr[d] * WW_MAP_W;
			if (!seen[next]) dirs[nd++] = d;
		}
		if (!nd) {
			--n;
			continue;
		}
		const uint32_t d = dirs[game_rand() % nd];
		const uint32_t next = room + _dc[d] + _dr[d] * WW_MAP_W;
		join(room, d);
		seen[next] = true;
		stack[n++] = next;
	}
	for (uint32_t i = 0; i < WW_ROOMS / 5; ++i) {
		const uint32_t room = game_rand() % WW_ROOMS;
		const uint32_t d = game_rand() & 3;
		if (on_map(room, d)) join(room, d);
	}
}

static __force_inline uint8_t* cell(uint8_t* m, int32_t c, int32_t r) {
	return &m[r * WW_ROOM_COLS + c];
}

// Whether a tile is kept clear, in front of the ways in and in the middle of the first room
static bool reserved(uint32_t exits, bool start, int32_t c, int32_t r) {
	const bool gap_c = c >= WW_GAP_C0 - 1 && c < WW_GAP_C0 + WW_GAP_N + 1;
	const bool gap_r = r >= WW_GAP_R0 - 1 && r < WW_GAP_R0 + WW_GAP_N + 1;
	if ((exits & (1u << WW_DIR_N)) && gap_c && r < 3) return true;
	if ((exits & (1u << WW_DIR_S)) && gap_c && r >= WW_ROOM_ROWS - 3) return true;
	if ((exits & (1u << WW_DIR_W)) && gap_r && c < 3) return true;
	if ((exits & (1u << WW_DIR_E)) && gap_r && c >= WW_ROOM_COLS - 3) return true;
	if (start && c >= 7 && c < 13 && r >= 5 && r < 9) return true;
	return false;
}

// A tile just inside one of the ways in
static void gap_cell(uint32_t dir, int32_t* c, int32_t* r) {
	switch (dir) {
		case WW_DIR_N: *c = WW_GAP_C0 + 1; *r = 0; break;
		case WW_DIR_S: *c = WW_GAP_C0 + 1; *r = WW_ROOM_ROWS - 1; break;
		case WW_DIR_W: *c = 0; *r = WW_GAP_R0 + 1; break;
		default: *c = WW_ROOM_COLS - 1; *r = WW_GAP_R0 + 1; break;
	}
}

// Marks the tiles that can be reached from the first way in, returning how
// many. The other ways in have to be reached for the room to be any good.
static uint32_t flood(uint8_t* m, uint32_t exits) {
	// Kept off the stack, which is small
	static uint16_t q[WW_ROOM_CELLS];
	uint32_t qh = 0, qt = 0;
	for (uint32_t i = 0; i < WW_ROOM_CELLS; ++i) m[i] &= ~WW_L_REACH;
	for (uint32_t d = 0; d < 4; ++d) {
		if (!(exits & (1u << d))) continue;
		int32_t c, r;
		gap_cell(d, &c, &r);
		uint8_t* p = cell(m, c, r);
		if (ww_tile_solid(*p)) return 0;
		*p |= WW_L_REACH;
		q[qt++] = r * WW_ROOM_COLS + c;
		break;
	}
	while (qh < qt) {
		const uint32_t i = q[qh++];
		const int32_t c = i % WW_ROOM_COLS, r = i / WW_ROOM_COLS;
		for (uint32_t d = 0; d < 4; ++d) {
			const int32_t nc = c + _dc[d], nr = r + _dr[d];
			if (nc < 0 || nr < 0 || nc >= WW_ROOM_COLS || nr >= WW_ROOM_ROWS) continue;
			uint8_t* p = cell(m, nc, nr);
			if (ww_tile_solid(*p) || (*p & WW_L_REACH)) continue;
			*p |= WW_L_REACH;
			q[qt++] = nr * WW_ROOM_COLS + nc;
		}
	}
	return qt;
}

static bool connected(uint8_t* m, uint32_t exits, bool start) {
	if (flood(m, exits) < WW_ROOM_MIN_OPEN) return false;
	for (uint32_t d = 0; d < 4; ++d) {
		if (!(exits & (1u << d))) continue;
		int32_t c, r;
		gap_cell(d, &c, &r);
		if (!(*cell(m, c, r) & WW_L_REACH)) return false;
	}
	return !start || (*cell(m, 10, 7) & WW_L_REACH);
}

// Tries to put something down, taking it away again if it blocks the way
static void place(uint8_t* m, uint32_t exits, bool start, int32_t c0, int32_t r0, int32_t w, int32_t h, uint8_t type) {
	static uint8_t keep[WW_ROOM_CELLS];
	memcpy(keep, m, sizeof(keep));
	for (int32_t r = r0; r < r0 + h; ++r) {
		for (int32_t c = c0; c < c0 + w; ++c) {
			if (c < 1 || r < 1 || c >= WW_ROOM_COLS - 1 || r >= WW_ROOM_ROWS - 1 || reserved(exits, start, c, r)) {
				memcpy(m, keep, sizeof(keep));
				return;
			}
			uint8_t* p = cell(m, c, r);
			// Trees go on clear ground
			if (type == WW_L_TREE && (*p & WW_L_TYPE) != WW_L_GRASS) {
				memcpy(m, keep, sizeof(keep));
				return;
			}
			*p = type == WW_L_TREE ? WW_L_TREE + (r - r0) * 2 + (c - c0) : type;
		}
	}
	if (!connected(m, exits, start)) memcpy(m, keep, sizeof(keep));
}

static void make_room(uint32_t room) {
	WwRoom_t* rm = &_rooms[room];
	uint8_t* m = rm->map;
	const uint32_t exits = rm->exits;
	const bool start = room == _start;
	for (int32_t r = 0; r < WW_ROOM_ROWS; ++r) {
		for (int32_t c = 0; c < WW_ROOM_COLS; ++c) {
			const bool gap_c = c >= WW_GAP_C0 && c < WW_GAP_C0 + WW_GAP_N;
			const bool gap_r = r >= WW_GAP_R0 && r < WW_GAP_R0 + WW_GAP_N;
			bool edge = false;
			if (r == 0) edge = !(gap_c && (exits & (1u << WW_DIR_N)));
			if (r == WW_ROOM_ROWS - 1) edge = !(gap_c && (exits & (1u << WW_DIR_S)));
			if (c == 0) edge = edge || !(gap_r && (exits & (1u << WW_DIR_W)));
			if (c == WW_ROOM_COLS - 1) edge = edge || !(gap_r && (exits & (1u << WW_DIR_E)));
			*cell(m, c, r) = edge ? WW_L_BUSH : WW_L_GRASS;
		}
	}
	bool pond = false;
	const uint32_t attempts = 26 + game_rand() % 10;
	for (uint32_t i = 0; i < attempts; ++i) {
		const uint32_t k = game_rand() % 100;
		const int32_t c = game_rand() % WW_ROOM_COLS;
		const int32_t r = game_rand() % WW_ROOM_ROWS;
		if (k < 55) {
			place(m, exits, start, c, r, 1 + game_rand() % 4, 1 + game_rand() % 3, WW_L_BUSH);
		}
		else if (k < 72) {
			place(m, exits, start, c, r, 2, 2, WW_L_TREE);
		}
		else if (k < 88) {
			place(m, exits, start, c, r, 1, 1, WW_L_ROCK);
		}
		else if (!pond) {
			place(m, exits, start, c, r, 2 + game_rand() % 3, 2 + game_rand() % 2, WW_L_WATER);
			pond = true;
		}
	}
	// Flowers and toadstools dotted about
	for (uint32_t i = 0; i < WW_ROOM_CELLS; ++i) {
		if ((m[i] & WW_L_TYPE) != WW_L_GRASS) continue;
		const uint32_t k = game_rand() % 100;
		if (k < 7) m[i] = WW_L_FLOWERS;
		else if (k < 9) m[i] = WW_L_MUSHROOM;
	}
	flood(m, exits);
}

void ww_world_new(void) {
	memset(_rooms, 0, sizeof(_rooms));
	_seed = game_rand();
	_start = (WW_MAP_H / 2) * WW_MAP_W + WW_MAP_W / 2;
	make_maze();
	for (uint32_t i = 0; i < WW_ROOMS; ++i) make_room(i);
	ww_world_enter(_start);
}

uint32_t ww_world_start(void) {
	return _start;
}

uint32_t ww_world_room(void) {
	return _room;
}

void ww_world_enter(uint32_t room) {
	_room = room;
	if (_rooms[room].visits < 255) ++_rooms[room].visits;
	_gfx_i ^= 1;
	ww_tiles_build(_gfx[_gfx_i], _rooms[room].map, _seed + room * 31);
}

const uint8_t* ww_world_logic(uint32_t room) {
	return _rooms[room].map;
}

bool __not_in_flash_func(ww_world_solid)(int32_t x, int32_t y) {
	const int32_t c = game_clamp(x >> WW_TILE_SHIFT, 0, WW_ROOM_COLS - 1);
	const int32_t r = game_clamp(y >> WW_TILE_SHIFT, 0, WW_ROOM_ROWS - 1);
	return ww_tile_solid(_rooms[_room].map[r * WW_ROOM_COLS + c]);
}

bool __not_in_flash_func(ww_world_box_free)(int32_t x0, int32_t y0, int32_t x1, int32_t y1) {
	// The tiles are bigger than anything tested, so the corners are enough
	return !ww_world_solid(x0, y0) && !ww_world_solid(x1, y0) && !ww_world_solid(x0, y1) && !ww_world_solid(x1, y1);
}

static __force_inline bool box_free_at(int32_t x, int32_t y, int32_t hw, int32_t hh) {
	return ww_world_box_free(x - hw, y - hh, x + hw - 1, y - 1);
}

// How far round a corner it will slide
#define WW_SLIDE 10

uint32_t __not_in_flash_func(ww_world_move)(int32_t* x, int32_t* y, int32_t dx, int32_t dy, int32_t hw, int32_t hh) {
	uint32_t blocked = 0;
	if (dx) {
		const int32_t nx = *x + dx;
		if (box_free_at(nx >> GAME_FP, *y >> GAME_FP, hw, hh)) *x = nx;
		else blocked |= 1;
	}
	if (dy) {
		const int32_t ny = *y + dy;
		if (box_free_at(*x >> GAME_FP, ny >> GAME_FP, hw, hh)) *y = ny;
		else blocked |= 2;
	}
	// Going straight at the corner of something, so slip round it
	const int32_t px = *x >> GAME_FP, py = *y >> GAME_FP;
	if (blocked == 1 && !dy) {
		const int32_t sx = px + game_sign(dx) * 2;
		for (int32_t s = 1; s <= WW_SLIDE; ++s) {
			if (box_free_at(sx, py - s, hw, hh)) { *y -= GAME_PX(1); break; }
			if (box_free_at(sx, py + s, hw, hh)) { *y += GAME_PX(1); break; }
		}
	}
	if (blocked == 2 && !dx) {
		const int32_t sy = py + game_sign(dy) * 2;
		for (int32_t s = 1; s <= WW_SLIDE; ++s) {
			if (box_free_at(px - s, sy, hw, hh)) { *x -= GAME_PX(1); break; }
			if (box_free_at(px + s, sy, hw, hh)) { *x += GAME_PX(1); break; }
		}
	}
	return blocked;
}

bool ww_world_line_clear(int32_t x0, int32_t y0, int32_t x1, int32_t y1) {
	const int32_t steps = (game_abs(x1 - x0) > game_abs(y1 - y0) ? game_abs(x1 - x0) : game_abs(y1 - y0)) / 8 + 1;
	for (int32_t i = 0; i <= steps; ++i) {
		if (ww_world_solid(x0 + (x1 - x0) * i / steps, y0 + (y1 - y0) * i / steps)) return false;
	}
	return true;
}

bool ww_world_random_spot(uint32_t room, int32_t* x, int32_t* y) {
	const uint8_t* m = _rooms[room].map;
	for (uint32_t i = 0; i < WW_SPOT_TRIES; ++i) {
		const int32_t c = 2 + game_rand() % (WW_ROOM_COLS - 4);
		const int32_t r = 2 + game_rand() % (WW_ROOM_ROWS - 4);
		const uint8_t l = m[r * WW_ROOM_COLS + c];
		if (ww_tile_solid(l) || !(l & WW_L_REACH)) continue;
		// Keep away from the ways in, so nobody gets jumped on as they arrive
		if (c >= WW_GAP_C0 - 2 && c < WW_GAP_C0 + WW_GAP_N + 2 && (r < 4 || r >= WW_ROOM_ROWS - 4)) continue;
		if (r >= WW_GAP_R0 - 2 && r < WW_GAP_R0 + WW_GAP_N + 2 && (c < 5 || c >= WW_ROOM_COLS - 5)) continue;
		*x = c * WW_TILE + WW_TILE / 2;
		*y = r * WW_TILE + WW_TILE - 6;
		return true;
	}
	return false;
}

// Rooms apart from the given one, through the maze
static void room_distances(uint32_t from, uint8_t* dist) {
	uint8_t q[WW_ROOMS];
	uint32_t qh = 0, qt = 0;
	memset(dist, 0xff, WW_ROOMS);
	dist[from] = 0;
	q[qt++] = from;
	while (qh < qt) {
		const uint32_t room = q[qh++];
		for (uint32_t d = 0; d < 4; ++d) {
			const uint32_t next = ww_world_next(room, d);
			if (next == WW_NO_ROOM || dist[next] != 0xff) continue;
			dist[next] = dist[room] + 1;
			q[qt++] = next;
		}
	}
}

uint32_t ww_world_distance(uint32_t a, uint32_t b) {
	uint8_t dist[WW_ROOMS];
	room_distances(a, dist);
	return dist[b];
}

int32_t ww_world_route(uint32_t from, uint32_t to) {
	uint8_t dist[WW_ROOMS];
	room_distances(to, dist);
	for (uint32_t d = 0; d < 4; ++d) {
		const uint32_t next = ww_world_next(from, d);
		if (next != WW_NO_ROOM && dist[next] < dist[from]) return d;
	}
	return -1;
}

int32_t ww_world_route_unvisited(uint32_t from) {
	uint8_t dist[WW_ROOMS];
	room_distances(from, dist);
	uint32_t best = WW_NO_ROOM;
	for (uint32_t i = 0; i < WW_ROOMS; ++i) {
		if (_rooms[i].visits || dist[i] == 0xff) continue;
		if (best == WW_NO_ROOM || dist[i] < dist[best]) best = i;
	}
	return best == WW_NO_ROOM ? -1 : ww_world_route(from, best);
}

bool ww_world_step(int32_t fc, int32_t fr, int32_t tc, int32_t tr, int32_t* nc, int32_t* nr) {
	const uint8_t* m = _rooms[_room].map;
	// Kept off the stack, which is small
	static uint16_t dist[WW_ROOM_CELLS];
	static uint16_t q[WW_ROOM_CELLS];
	uint32_t qh = 0, qt = 0;
	fc = game_clamp(fc, 0, WW_ROOM_COLS - 1);
	fr = game_clamp(fr, 0, WW_ROOM_ROWS - 1);
	tc = game_clamp(tc, 0, WW_ROOM_COLS - 1);
	tr = game_clamp(tr, 0, WW_ROOM_ROWS - 1);
	if (fc == tc && fr == tr) {
		*nc = tc;
		*nr = tr;
		return true;
	}
	// Worked out back from the target, so the first step can be read off
	memset(dist, 0xff, sizeof(dist));
	dist[tr * WW_ROOM_COLS + tc] = 0;
	q[qt++] = tr * WW_ROOM_COLS + tc;
	while (qh < qt) {
		const uint32_t i = q[qh++];
		const int32_t c = i % WW_ROOM_COLS, r = i / WW_ROOM_COLS;
		for (uint32_t d = 0; d < 4; ++d) {
			const int32_t cc = c + _dc[d], rr = r + _dr[d];
			if (cc < 0 || rr < 0 || cc >= WW_ROOM_COLS || rr >= WW_ROOM_ROWS) continue;
			const uint32_t j = rr * WW_ROOM_COLS + cc;
			if (dist[j] != 0xffff) continue;
			// The start may be on a tile it is squeezing past
			if (ww_tile_solid(m[j]) && !(cc == fc && rr == fr)) continue;
			dist[j] = dist[i] + 1;
			if (cc == fc && rr == fr) {
				*nc = c;
				*nr = r;
				return true;
			}
			q[qt++] = j;
		}
	}
	return false;
}

uint32_t ww_world_visits(uint32_t room) {
	return _rooms[room].visits;
}

uint32_t ww_world_visited_count(void) {
	uint32_t n = 0;
	for (uint32_t i = 0; i < WW_ROOMS; ++i) n += _rooms[i].visits != 0;
	return n;
}

void ww_world_draw(uint32_t frame) {
	ww_tiles_draw(_gfx[_gfx_i], frame);
}

static void map_fill(uint8_t* px, int32_t x, int32_t y, int32_t w, int32_t h, uint8_t p) {
	for (int32_t j = y; j < y + h; ++j) {
		for (int32_t i = x; i < x + w; ++i) px[j * WW_MAP_PX_W + i] = p;
	}
}

void ww_world_draw_map(int32_t x, int32_t y, uint32_t frame) {
	_map_i ^= 1;
	uint8_t* px = (uint8_t*)_map_px[_map_i];
	memset(px, 0, WW_MAP_PX_W * WW_MAP_PX_H);
	for (uint32_t room = 0; room < WW_ROOMS; ++room) {
		if (!_rooms[room].visits) continue;
		const int32_t cx = (room % WW_MAP_W) * WW_MAP_CELL_W;
		const int32_t cy = (room / WW_MAP_W) * WW_MAP_CELL_H;
		const bool here = room == _room;
		const uint8_t p = here
			? ((frame >> 4) & 1 ? HSTX_DVI_PIXEL_RGB(255, 255, 255) : HSTX_DVI_PIXEL_RGB(255, 224, 0))
			: HSTX_DVI_PIXEL_RGB(0, 160, 0);
		map_fill(px, cx + 1, cy + 1, 6, 3, p);
		// The ways through, to rooms not yet visited too
		const uint8_t way = HSTX_DVI_PIXEL_RGB(96, 96, 96);
		if (_rooms[room].exits & (1u << WW_DIR_E)) map_fill(px, cx + 7, cy + 2, 1, 1, way);
		if (_rooms[room].exits & (1u << WW_DIR_W)) map_fill(px, cx, cy + 2, 1, 1, way);
		if (_rooms[room].exits & (1u << WW_DIR_S)) map_fill(px, cx + 3, cy + 4, 2, 1, way);
		if (_rooms[room].exits & (1u << WW_DIR_N)) map_fill(px, cx + 3, cy, 2, 1, way);
	}
	GamePixels_t* g = &_map_pixels[_map_i];
	g->data = _map_px[_map_i];
	g->words = WW_MAP_PX_W / 4;
	game_spr_pixels(x, y, WW_MAP_PX_W, WW_MAP_PX_H, g);
}
