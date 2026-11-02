// Copyright (c) 2024 fruit-bat.

// See el_story.h.

#include "el_story_data.h"
#include "el_companion.h"
#include <ctype.h>
#include <stdio.h>
#include <string.h>

// One jump reaches this far, and a far system is up to this far, in tenths
// of a light year
#define NEAR_REACH 70
#define FAR_REACH 200

static StorySave_t* _s;
// The page waiting: the story and exit it is from, or a note
#define PENDING_NONE (-1)
#define PENDING_NOTE (-2)
static int32_t _pending = PENDING_NONE;
static const StoryExit_t* _pending_exit;
static const char* _note_title;
static const char* _note_text;

StorySave_t* story_save(void) {
	return _s;
}

void story_bind(StorySave_t* s) {
	_s = s;
	_pending = PENDING_NONE;
	if (!_s->seed) _s->seed = 0x2545f491u;
}

void story_reset(StorySave_t* s) {
	memset(s, 0, sizeof(*s));
	s->seed = 0x2545f491u;
	if (s == _s) _pending = PENDING_NONE;
}

// Repeatable, from the save, so the harness can go over the same ground
static uint32_t rnd(void) {
	uint32_t x = _s->seed;
	x ^= x << 13;
	x ^= x >> 17;
	x ^= x << 5;
	_s->seed = x;
	return x;
}

// ----------------------------------------------------------------------------
// Conditions

static bool has_item(uint32_t item) {
	for (uint32_t i = 0; i < STORY_ITEMS; ++i) {
		if (_s->item[i] == item) return true;
	}
	return false;
}

static bool holds(const StoryIf_t* w, const StoryProgress_t* p) {
	switch (w->what) {
		case IF_ITEM: return has_item((uint32_t)w->a);
		case IF_NO_ITEM: return !has_item((uint32_t)w->a);
		case IF_COMPANION: return w->a ? companion() == (Companion_e)w->a : companion() != COMPANION_NONE;
		case IF_NO_COMPANION: return _s->companion == COMPANION_NONE;
		case IF_CABIN: return story_host_has_cabin();
		case IF_CASH: return story_host_cash() >= (uint32_t)w->a * 10u;
		case IF_COUNT_AT_LEAST: return p->counter >= w->a;
		case IF_COUNT_UNDER: return p->counter < w->a;
		case IF_FLAG: return (_s->flags & (uint32_t)w->a) != 0;
		case IF_NO_FLAG: return (_s->flags & (uint32_t)w->a) == 0;
		case IF_ENDED: return _s->story[w->a].status == STORY_TOLD && _s->story[w->a].ending == w->b;
		case IF_TOLD: return _s->story[w->a].status == STORY_TOLD;
		case IF_ROOM: return story_host_room() >= (uint32_t)w->a;
		case IF_FITTED: return story_host_fitted((uint32_t)w->a);
		default: return true;
	}
}

// ----------------------------------------------------------------------------
// Picking the systems

static bool lawless(const GalaxySystem_t* g) { return g->government <= 1; }
static bool orderly(const GalaxySystem_t* g) { return g->government >= 5; }
static bool industrial(const GalaxySystem_t* g) { return g->economy <= 2; }
static bool rich(const GalaxySystem_t* g) { return g->economy <= 1; }
static bool high_tech(const GalaxySystem_t* g) { return g->tech >= 8; }

static bool suits(uint8_t pick, const GalaxySystem_t* g) {
	switch (pick) {
		case PICK_HERE_ANARCHY: case PICK_NEAR_ANARCHY: return lawless(g);
		case PICK_HERE_SAFE: case PICK_NEAR_SAFE: return orderly(g);
		case PICK_HERE_INDUSTRY: return industrial(g);
		case PICK_HERE_RICH: return rich(g);
		case PICK_NEAR_TECH: return high_tech(g);
		default: return true;
	}
}

// The systems for story i, starting here, or false if there are none to suit.
// The last system, 255, is left out, as that marks a place not used.
static bool pick_places(const Story_t* st, uint32_t here, uint8_t place[STORY_PLACES]) {
	if (here >= 0xff) return false;
	uint32_t from = here;
	for (uint32_t k = 0; k < STORY_PLACES; ++k) {
		const uint8_t pick = st->pick[k];
		place[k] = 0xff;
		if (pick == PICK_NONE) continue;
		if (pick <= PICK_HERE_RICH) {
			if (!suits(pick, story_host_system(here))) return false;
			place[k] = (uint8_t)here;
			from = here;
			continue;
		}
		// Among those that suit, in reach of the system before, not here or used
		const uint32_t lo = pick == PICK_FAR ? NEAR_REACH + 1 : 1;
		const uint32_t hi = pick == PICK_FAR ? FAR_REACH : NEAR_REACH;
		// Kept off the stack, which is small
		static uint8_t found[GALAXY_SYSTEMS];
		uint32_t n = 0;
		for (uint32_t i = 0; i < 0xff; ++i) {
			if (i == here) continue;
			bool used = false;
			for (uint32_t j = 0; j < k; ++j) used |= place[j] == i;
			if (used) continue;
			const GalaxySystem_t* g = story_host_system(i);
			const uint32_t d = galaxy_distance(story_host_system(from), g);
			if (d >= lo && d <= hi && suits(pick, g)) found[n++] = (uint8_t)i;
		}
		if (!n) return false;
		place[k] = found[rnd() % n];
		from = place[k];
	}
	return true;
}

// ----------------------------------------------------------------------------
// Moving through a story

static bool matches(const StoryExit_t* x, const StoryProgress_t* p, StoryEvent_e ev, uint32_t arg) {
	if (x->ev != ev) return false;
	switch (ev) {
		case STORY_DOCK:
		case STORY_ARRIVE:
			if (x->place == PLACE_ANY) break;
			if (p->galaxy != story_host_galaxy() || p->place[x->place] != arg) return false;
			break;
		case STORY_SCOOP:
		case STORY_DISABLED:
		case STORY_DESTROYED:
		case STORY_ESCAPED:
		case STORY_NEAR:
			if (x->arg && x->arg != arg) return false;
			break;
		case STORY_JUMP:
			if (p->jumps < x->arg) return false;
			break;
		default:
			break;
	}
	return holds(&x->when, p);
}

static void give(uint32_t item) {
	if (has_item(item)) return;
	for (uint32_t i = 0; i < STORY_ITEMS; ++i) {
		if (!_s->item[i]) {
			_s->item[i] = (uint8_t)item;
			return;
		}
	}
}

static void take(uint32_t item) {
	for (uint32_t i = 0; i < STORY_ITEMS; ++i) {
		if (_s->item[i] == item) _s->item[i] = 0;
	}
}

static void run(const StoryOp_t* op, StoryProgress_t* p) {
	for (; op && op->op != OP_END; ++op) {
		switch (op->op) {
			case OP_GIVE: give((uint32_t)op->a); break;
			case OP_TAKE: take((uint32_t)op->a); break;
			case OP_WORSE:
				for (uint32_t i = 0; i < STORY_ITEMS; ++i) {
					if (_s->item[i] >= op->a && _s->item[i] < op->b) {
						++_s->item[i];
						break;
					}
				}
				break;
			case OP_CASH: story_host_add_cash((int32_t)op->a * 10); break;
			case OP_OFFENDER: story_host_offender(op->a != 0); break;
			case OP_FLAG: _s->flags |= (uint32_t)op->a; break;
			case OP_COUNT: p->counter = (uint8_t)op->a; break;
			case OP_ADD: {
				const int32_t c = (int32_t)p->counter + op->a;
				p->counter = (uint8_t)(c < 0 ? 0 : c > 255 ? 255 : c);
				break;
			}
			case OP_SPAWN: story_host_spawn((uint32_t)op->b, &story_ships[op->a]); break;
			case OP_CANISTER: story_host_canister((uint32_t)op->a); break;
			case OP_CALM: story_host_calm(); break;
			case OP_WITCH: story_host_witch(); break;
			case OP_LASER: story_host_laser((uint32_t)op->a); break;
			case OP_JOIN: companion_join((uint32_t)op->a); break;
			case OP_LEAVE: companion_leave(); break;
			case OP_AWAY: companion_away(true); break;
			case OP_BACK: companion_away(false); break;
			case OP_LOYAL: companion_loyalty(op->a, (uint32_t)op->b); break;
			case OP_PET: companion_pet((uint32_t)op->a); break;
			case OP_ENDING: p->ending = (uint8_t)op->a; break;
			case OP_DRAIN: story_host_drain(); break;
			default: break;
		}
	}
}

static bool step(uint32_t i, StoryEvent_e ev, uint32_t arg);

// Does what a choice says, and goes on to its next step
static void take_choice(uint32_t i, const StoryChoice_t* c) {
	StoryProgress_t* p = &_s->story[i];
	run(c->ops, p);
	if (c->next == NEXT_SAME) return;
	if (c->next == NEXT_DONE || c->next == NEXT_AGAIN) {
		p->status = c->next == NEXT_DONE ? STORY_TOLD : STORY_UNTOLD;
		if (_s->active == i + 1) _s->active = 0;
		return;
	}
	p->step = c->next;
	p->jumps = 0;
	// What came of it, at once
	step(i, STORY_NOW, 0);
}

// Fills in the systems and the companion
static void fill(char* out, uint32_t n, const char* in, uint32_t len, const StoryProgress_t* p) {
	uint32_t k = 0;
	for (uint32_t i = 0; i < len && k + 1 < n; ++i) {
		const char c = in[i];
		if (c == '%' && i + 1 < len) {
			const char w = in[++i];
			const char* s = "";
			if (w >= '0' && w <= '2' && p && p->place[w - '0'] != 0xff) s = story_host_system(p->place[w - '0'])->name;
			else if (w == 'C') s = companion_name() ? companion_name() : "YOUR FRIEND";
			for (; *s && k + 1 < n; ++s) out[k++] = (char)toupper((unsigned char)*s);
			continue;
		}
		out[k++] = c;
	}
	out[k] = 0;
}

// Says a line, filled in, for someone
static void say(const StoryExit_t* x, const StoryProgress_t* p) {
	char line[PAGE_LINE_MAX];
	fill(line, sizeof(line), x->text, (uint32_t)strlen(x->text), p);
	const char* name = x->who == SPEAK_COMPANION ? companion_name() : x->who == SPEAK_PET ? pet_name() : "HAIL";
	if (name) story_host_say((Speaker_e)x->who, name, line);
}

static void take_exit(uint32_t i, const StoryExit_t* x) {
	const StoryProgress_t* p = &_s->story[i];
	if (x->text && x->who == SPEAK_PAGE) {
		_pending = (int32_t)i;
		_pending_exit = x;
		return;
	}
	if (x->text) say(x, p);
	take_choice(i, &x->choice[0]);
}

// Whether story i at its step has an exit for this, and takes it
static bool step(uint32_t i, StoryEvent_e ev, uint32_t arg) {
	const Story_t* st = &story_data[i];
	StoryProgress_t* p = &_s->story[i];
	const StoryStep_t* s = &st->steps[p->step];
	for (uint32_t k = 0; k < s->n; ++k) {
		if (matches(&s->exits[k], p, ev, arg)) {
			take_exit(i, &s->exits[k]);
			return true;
		}
	}
	return false;
}

bool story_start(uint32_t i, uint32_t here) {
	if (i >= story_data_count || _s->active) return false;
	const Story_t* st = &story_data[i];
	StoryProgress_t* p = &_s->story[i];
	if (p->status != STORY_UNTOLD || !holds(&st->when, p)) return false;
	uint8_t place[STORY_PLACES];
	if (!pick_places(st, here, place)) return false;
	memset(p, 0, sizeof(*p));
	memcpy(p->place, place, sizeof(place));
	p->galaxy = (uint8_t)story_host_galaxy();
	p->status = STORY_GOING;
	_s->active = (uint8_t)(i + 1);
	return true;
}

void story_event(StoryEvent_e ev, uint32_t arg) {
	if (!_s || _pending != PENDING_NONE) return;
	if (_s->active) {
		const uint32_t i = _s->active - 1u;
		if (ev == STORY_JUMP && _s->story[i].jumps < 255) ++_s->story[i].jumps;
		step(i, ev, arg);
		return;
	}
	// Nothing going: maybe a story starts, on docking or coming in
	if (ev != STORY_DOCK && ev != STORY_ARRIVE) return;
	const uint32_t first = rnd() % story_data_count;
	for (uint32_t k = 0; k < story_data_count; ++k) {
		const uint32_t i = (first + k) % story_data_count;
		const Story_t* st = &story_data[i];
		if (_s->story[i].status != STORY_UNTOLD || rnd() % 100 >= st->chance) continue;
		// Only if it would start on this
		bool starts = false;
		for (uint32_t x = 0; x < st->steps[0].n; ++x) starts |= st->steps[0].exits[x].ev == ev;
		if (!starts || !story_start(i, arg)) continue;
		if (step(i, ev, arg)) return;
		// It cannot start after all
		_s->story[i].status = STORY_UNTOLD;
		_s->active = 0;
	}
}

bool story_page(StoryPage_t* page) {
	if (!_s || _pending == PENDING_NONE) return false;
	memset(page, 0, sizeof(*page));
	const char* text;
	const StoryProgress_t* p = NULL;
	if (_pending == PENDING_NOTE) {
		page->title = _note_title;
		text = _note_text;
	}
	else {
		p = &_s->story[_pending];
		page->title = story_data[_pending].name;
		text = _pending_exit->text;
		for (uint32_t c = 0; c < 2; ++c) {
			const StoryChoice_t* ch = &_pending_exit->choice[c];
			if (!ch->label) continue;
			page->choice[page->choices] = ch->label;
			page->can[page->choices] = holds(&ch->when, p);
			++page->choices;
		}
	}
	// The lines, split
	while (text && *text && page->lines < PAGE_LINES) {
		const char* end = strchr(text, '\n');
		const uint32_t len = end ? (uint32_t)(end - text) : (uint32_t)strlen(text);
		fill(page->line[page->lines++], PAGE_LINE_MAX, text, len, p);
		text = end ? end + 1 : NULL;
	}
	return true;
}

void story_choose(uint32_t choice) {
	if (_pending == PENDING_NONE) return;
	if (_pending == PENDING_NOTE) {
		_pending = PENDING_NONE;
		return;
	}
	const uint32_t i = (uint32_t)_pending;
	const StoryExit_t* x = _pending_exit;
	const StoryChoice_t* c = &x->choice[0];
	if (x->choice[0].label) {
		// The choices shown are those with labels, in order
		uint32_t shown = 0;
		const StoryChoice_t* found = NULL;
		for (uint32_t k = 0; k < 2; ++k) {
			if (!x->choice[k].label) continue;
			if (shown++ == choice) found = &x->choice[k];
		}
		if (!found || !holds(&found->when, &_s->story[i])) return;
		c = found;
	}
	_pending = PENDING_NONE;
	take_choice(i, c);
}

void story_pending_keep(StoryPending_t* k) {
	k->pending = _pending;
	k->exit = _pending_exit;
	k->title = _note_title;
	k->text = _note_text;
}

void story_pending_put(const StoryPending_t* k) {
	_pending = k->pending;
	_pending_exit = (const StoryExit_t*)k->exit;
	_note_title = k->title;
	_note_text = k->text;
}

void story_note(const char* title, const char* text) {
	_note_title = title;
	_note_text = text;
	_pending = PENDING_NOTE;
}

const char* story_item_name(uint32_t item) {
	return item && item < story_items_count ? story_items[item].name : "";
}

bool story_has_item(uint32_t item) {
	return _s && has_item(item);
}

uint32_t story_item_names(const char** names, uint32_t max) {
	uint32_t n = 0;
	for (uint32_t i = 0; _s && i < STORY_ITEMS && n < max; ++i) {
		if (_s->item[i] && _s->item[i] < story_items_count) names[n++] = story_items[_s->item[i]].name;
	}
	return n;
}

// Story i's ship with an id, from where the story spawns it
static const StoryShip_t* ship_of(uint32_t i, uint32_t id) {
	for (uint32_t s = 0; s < story_data[i].n; ++s) {
		const StoryStep_t* st = &story_data[i].steps[s];
		for (uint32_t x = 0; x < st->n; ++x) {
			for (uint32_t c = 0; c < 2; ++c) {
				for (const StoryOp_t* op = st->exits[x].choice[c].ops; op && op->op != OP_END; ++op) {
					if (op->op == OP_SPAWN && (uint32_t)op->b == id) return &story_ships[op->a];
				}
			}
		}
	}
	return NULL;
}

static const char* ship_label(uint32_t i, uint32_t id) {
	const StoryShip_t* ship = ship_of(i, id);
	return ship ? ship->label : "A SHIP";
}

// Whether destroying a ship is worth a hint: not one to be disabled, nor
// one that is not out to fight, which it would be a shame to
static bool hint_destroy(uint32_t i, const StoryStep_t* s, uint32_t id) {
	const StoryShip_t* ship = ship_of(i, id);
	if (!ship || ship->goal != GOAL_FIGHT) return false;
	for (uint32_t k = 0; k < s->n; ++k) {
		if (s->exits[k].ev == STORY_DISABLED && s->exits[k].arg == id) return false;
	}
	return true;
}

uint32_t story_hints(char (*line)[STORY_HINT_MAX], uint32_t max) {
	if (!_s || !_s->active) return 0;
	const uint32_t i = _s->active - 1u;
	const StoryProgress_t* p = &_s->story[i];
	const StoryStep_t* s = &story_data[i].steps[p->step];
	uint32_t n = 0;
	for (uint32_t k = 0; k < s->n && n < max; ++k) {
		const StoryExit_t* x = &s->exits[k];
		if (!holds(&x->when, p)) continue;
		char* l = line[n];
		const char* place = x->place == PLACE_ANY ? NULL : story_host_system(p->place[x->place])->name;
		switch (x->ev) {
			case STORY_DOCK:
				if (place) snprintf(l, STORY_HINT_MAX, "DOCK AT %s", place);
				else snprintf(l, STORY_HINT_MAX, "DOCK ANYWHERE");
				break;
			case STORY_ARRIVE:
				if (place) snprintf(l, STORY_HINT_MAX, "GO TO %s", place);
				else continue;
				break;
			case STORY_SCOOP: snprintf(l, STORY_HINT_MAX, "SCOOP %s", x->arg ? story_items[x->arg].name : "SOMETHING"); break;
			case STORY_DISABLED: snprintf(l, STORY_HINT_MAX, "DISABLE %s", ship_label(i, x->arg)); break;
			case STORY_DESTROYED:
				if (!hint_destroy(i, s, x->arg)) continue;
				snprintf(l, STORY_HINT_MAX, "DESTROY %s", ship_label(i, x->arg));
				break;
			case STORY_NEAR: snprintf(l, STORY_HINT_MAX, "MEET %s", ship_label(i, x->arg)); break;
			case STORY_SUN: snprintf(l, STORY_HINT_MAX, "FLY CLOSE TO THE SUN"); break;
			case STORY_WITCH: snprintf(l, STORY_HINT_MAX, "GO INTO WITCHSPACE"); break;
			case STORY_HULL_HIT: snprintf(l, STORY_HINT_MAX, "TAKE A HIT"); break;
			default: continue;
		}
		for (char* c = l; *c; ++c) *c = (char)toupper((unsigned char)*c);
		// Once each, as exits for the same thing differ only in what comes of it
		bool again = false;
		for (uint32_t j = 0; j < n; ++j) again |= !strcmp(line[j], l);
		if (!again) ++n;
	}
	return n;
}

uint32_t story_told(void) {
	uint32_t n = 0;
	for (uint32_t i = 0; _s && i < story_data_count; ++i) n += _s->story[i].status == STORY_TOLD;
	return n;
}

uint32_t story_item_tonnes(void) {
	uint32_t t = 0;
	for (uint32_t i = 0; _s && i < STORY_ITEMS; ++i) {
		if (_s->item[i] && _s->item[i] < story_items_count) t += story_items[_s->item[i]].tonnes;
	}
	return t;
}

bool story_flag(uint32_t flag) {
	return _s && (_s->flags & flag) != 0;
}

uint32_t story_count(void) {
	return story_data_count;
}

const char* story_name(uint32_t i) {
	return i < story_data_count ? story_data[i].name : NULL;
}

int32_t story_active(void) {
	return _s && _s->active ? (int32_t)_s->active - 1 : -1;
}

const StoryProgress_t* story_progress(uint32_t i) {
	return &_s->story[i];
}

bool story_holds(uint32_t i, const StoryIf_t* w) {
	return holds(w, &_s->story[i]);
}

uint32_t story_waits(uint32_t i, StoryEvent_e* ev, uint32_t* arg, uint32_t max) {
	const StoryProgress_t* p = &_s->story[i];
	const StoryStep_t* s = &story_data[i].steps[p->status == STORY_GOING ? p->step : 0];
	uint32_t n = 0;
	for (uint32_t k = 0; k < s->n && n < max; ++k) {
		const StoryExit_t* x = &s->exits[k];
		ev[n] = (StoryEvent_e)x->ev;
		if (x->ev == STORY_DOCK || x->ev == STORY_ARRIVE) arg[n] = x->place == PLACE_ANY ? 0xffffffffu : p->place[x->place];
		else arg[n] = x->arg;
		++n;
	}
	return n;
}
