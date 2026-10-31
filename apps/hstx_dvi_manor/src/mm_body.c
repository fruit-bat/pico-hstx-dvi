#include "mm_body.h"
#include "mm_map.h"
#include "mm_thing.h"

static bool solid(int32_t x0, int32_t y0, int32_t x1, int32_t y1, const void* self) {
	return mm_map_box_solid(x0, y0, x1, y1) || mm_thing_crate_box(x0, y0, x1, y1, self);
}

// Something to stand on in pixel row y
static bool floor_row(int32_t x0, int32_t x1, int32_t y, bool drop, const void* self) {
	if (solid(x0, y, x1, y + 1, self)) return true;
	if (drop) return false;
	if ((y & (MM_TILE - 1)) == 0) {
		for (int32_t cx = x0 >> MM_TILE_SHIFT; cx <= (x1 - 1) >> MM_TILE_SHIFT; ++cx) {
			if (mm_map_platform(cx, y >> MM_TILE_SHIFT)) return true;
		}
	}
	return mm_thing_top_at(x0, x1, y, self);
}

bool mm_body_fits(const MmBody_t* b, int32_t x, int32_t y, const void* self) {
	return !solid(x - b->hw, y - b->h, x + b->hw, y, self);
}

bool mm_body_floor(const MmBody_t* b, int32_t x, int32_t y, bool drop, const void* self) {
	return floor_row(x - b->hw, x + b->hw, y, drop, self);
}

void mm_body_move(MmBody_t* b, bool drop, const void* self) {
	b->wall = false;
	b->head = false;
	int32_t px = b->x >> GAME_FP;
	int32_t py = b->y >> GAME_FP;
	// Something stuck inside a wall is let out
	const bool stuck = !mm_body_fits(b, px, py, self);

	// Across, a pixel at a time
	const int32_t tx = (b->x + b->vx) >> GAME_FP;
	const int32_t sx = tx > px ? 1 : -1;
	while (px != tx) {
		if (!stuck && !mm_body_fits(b, px + sx, py, self)) {
			b->wall = true;
			break;
		}
		px += sx;
	}
	if (b->wall) {
		b->x = GAME_PX(px);
		b->vx = 0;
	}
	else {
		b->x += b->vx;
	}

	// Up or down
	const int32_t ty = (b->y + b->vy) >> GAME_FP;
	bool stopped = false;
	if (ty > py) {
		while (py != ty) {
			if (floor_row(px - b->hw, px + b->hw, py, drop, self)) {
				stopped = true;
				break;
			}
			++py;
		}
	}
	else {
		while (py != ty) {
			if (!stuck && solid(px - b->hw, py - b->h - 1, px + b->hw, py - b->h, self)) {
				stopped = true;
				b->head = true;
				break;
			}
			--py;
		}
	}
	if (stopped) {
		b->y = GAME_PX(py);
		b->vy = 0;
	}
	else {
		b->y += b->vy;
	}
	b->ground = b->vy >= 0 && floor_row(px - b->hw, px + b->hw, b->y >> GAME_FP, drop, self);
}
