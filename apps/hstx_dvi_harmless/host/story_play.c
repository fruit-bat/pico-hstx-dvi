// Plays the stories by hand on the PC: lists them, starts the one picked,
// and at each step shows what it waits for, with the conditions and whether
// they hold, so they can be made to happen one at a time, and pages chosen.
//
// host/story_play.sh builds and runs it.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "el_story_data.h"
#include "el_companion.h"

static GalaxySystem_t _gal[GALAXY_SYSTEMS];
static uint32_t _here;
static bool _cabin;
static uint32_t _cash;
static bool _offender, _witch, _targeting;
static StorySave_t _save;
// The story ships spawned, by id, for naming them
#define SHIP_IDS 16
static int8_t _ship[SHIP_IDS];

// What the game would do, said as it happens
const GalaxySystem_t* story_host_system(uint32_t i) { return &_gal[i % GALAXY_SYSTEMS]; }
uint32_t story_host_galaxy(void) { return 0; }
uint32_t story_host_here(void) { return _here; }
bool story_host_has_cabin(void) { return _cabin; }
uint32_t story_host_cash(void) { return _cash; }
// A Cobra's hold, less the cabin and what the stories put there
uint32_t story_host_room(void) { return 20 - (_cabin ? 5 : 0) - story_item_tonnes(); }
bool story_host_fitted(uint32_t kit) { return kit == KIT_TARGETING && _targeting; }

void story_host_add_cash(int32_t tenths) {
	_cash = (int32_t)_cash + tenths < 0 ? 0 : (uint32_t)((int32_t)_cash + tenths);
	printf("   [cash %+d Cr, now %u]\n", tenths / 10, _cash / 10);
}

void story_host_offender(bool on) {
	_offender = on;
	printf("   [%s]\n", on ? "now an offender" : "record cleared");
}

void story_host_spawn(uint32_t id, const StoryShip_t* s) {
	static const char* const goals[] = { "fights", "idles", "races", "drifts", "follows", "weaves" };
	if (id < SHIP_IDS) _ship[id] = (int8_t)(s - story_ships);
	printf("   [ship %u: %s, a %s, %s%s%s%s%s]\n", id, s->label, s->cls, goals[s->goal],
		s->at_station ? ", at the station" : "", s->disable ? ", can be disabled" : "",
		s->drop ? ", leaves " : "", s->drop ? story_items[s->drop].name : "");
}

void story_host_canister(uint32_t item) { printf("   [canister ahead: %s]\n", story_items[item].name); }
void story_host_calm(void) { printf("   [story ships calm down]\n"); }

void story_host_witch(void) {
	_witch = true;
	printf("   [next jump goes into witchspace]\n");
}

void story_host_drain(void) { printf("   [shields down]\n"); }

void story_host_laser(uint32_t laser) {
	static const char* const lasers[] = { "pulse", "beam", "military", "none" };
	printf("   [front laser: %s]\n", lasers[laser < 4 ? laser : 3]);
}

void story_host_say(Speaker_e who, const char* name, const char* text) {
	static const char* const whos[] = { "page", "companion", "pet", "hail" };
	printf("   (%s) %s: %s\n", whos[who], name, text);
}

// ----------------------------------------------------------------------------

static const char* const _event_names[STORY_EVENTS] = {
	"DOCK", "ARRIVE", "JUMP", "WITCH", "SCOOP", "DISABLED", "DESTROYED", "ESCAPED", "NEAR", "HULL HIT", "SUN", "NOW", "LAUNCH", "DEPART", "APPROACH", "HERE"
};

// The story ship with an id: the one spawned, or else the one story i spawns as it
static const char* ship_name(uint32_t i, uint32_t id) {
	if (id < SHIP_IDS && _ship[id] >= 0) return story_ships[_ship[id]].label;
	for (uint32_t s = 0; s < story_data[i].n; ++s) {
		const StoryStep_t* st = &story_data[i].steps[s];
		for (uint32_t x = 0; x < st->n; ++x) {
			for (uint32_t c = 0; c < 2; ++c) {
				for (const StoryOp_t* op = st->exits[x].choice[c].ops; op && op->op != OP_END; ++op) {
					if (op->op == OP_SPAWN && (uint32_t)op->b == id) return story_ships[op->a].label;
				}
			}
		}
	}
	return "a story ship";
}

static const char* item_name(uint32_t item) {
	return item < story_items_count ? story_items[item].name : "?";
}

static const char* flag_name(uint32_t f) {
	switch (f) {
		case STORY_FLAG_THARGOID_FRIEND: return "THARGOID FRIEND";
		case STORY_FLAG_PIRATE_FRIEND: return "PIRATE FRIEND";
		case STORY_FLAG_CAT: return "CAT";
		case STORY_FLAG_MEDAL: return "MEDAL";
		case STORY_FLAG_SOLD_THARGOID: return "SOLD THARGOID";
		case STORY_FLAG_CAPTAIN_FRIEND: return "CAPTAIN FRIEND";
		default: return "?";
	}
}

// A condition, in words
static void cond_text(char* out, size_t n, const StoryIf_t* w) {
	switch (w->what) {
		case IF_ALWAYS: snprintf(out, n, "always"); break;
		case IF_ITEM: snprintf(out, n, "has %s", item_name((uint32_t)w->a)); break;
		case IF_NO_ITEM: snprintf(out, n, "has no %s", item_name((uint32_t)w->a)); break;
		case IF_COMPANION: snprintf(out, n, "%s aboard", w->a ? companion_name_of((uint32_t)w->a) : "a companion"); break;
		case IF_NO_COMPANION: snprintf(out, n, "no companion"); break;
		case IF_CABIN: snprintf(out, n, "cabin fitted"); break;
		case IF_CASH: snprintf(out, n, "%d Cr", w->a); break;
		case IF_COUNT_AT_LEAST: snprintf(out, n, "counter >= %d", w->a); break;
		case IF_COUNT_UNDER: snprintf(out, n, "counter < %d", w->a); break;
		case IF_FLAG: snprintf(out, n, "flag %s", flag_name((uint32_t)w->a)); break;
		case IF_NO_FLAG: snprintf(out, n, "no flag %s", flag_name((uint32_t)w->a)); break;
		case IF_ENDED: snprintf(out, n, "%s ended %d", story_name((uint32_t)w->a), w->b); break;
		case IF_TOLD: snprintf(out, n, "%s told", story_name((uint32_t)w->a)); break;
		case IF_ROOM: snprintf(out, n, "%d t free", w->a); break;
		case IF_FITTED: {
			static const char* const kits[] = { "targeting computer", "docking computer", "fuel scoops", "ECM" };
			snprintf(out, n, "%s fitted", (uint32_t)w->a < 4 ? kits[w->a] : "?");
			break;
		}
		default: snprintf(out, n, "?"); break;
	}
}

static const char* next_text(uint8_t next, char* buf, size_t n) {
	if (next == NEXT_SAME) return "stays";
	if (next == NEXT_DONE) return "ends";
	if (next == NEXT_AGAIN) return "offered again later";
	snprintf(buf, n, "step %u", next);
	return buf;
}

static void state(void) {
	printf("  here %s, cash %u Cr, room %u t, cabin %s, targeting %s, %s", _gal[_here].name, _cash / 10, story_host_room(),
		_cabin ? "yes" : "no", _targeting ? "yes" : "no",
		_save.companion ? companion_name_of(_save.companion) : "nobody aboard");
	if (_save.companion) printf(" (mood %d, loyalty %d%s)", _save.mood, _save.loyalty, _save.away ? ", away" : "");
	if (_save.pet) printf(", pet %s", pet_name());
	if (_offender) printf(", offender");
	printf("\n");
	const char* names[STORY_ITEMS];
	const uint32_t n = story_item_names(names, STORY_ITEMS);
	if (n) {
		printf("  aboard:");
		for (uint32_t i = 0; i < n; ++i) printf(" %s%s", names[i], i + 1 < n ? "," : "");
		printf("\n");
	}
	if (_save.flags) {
		printf("  flags:");
		for (uint32_t f = 1; f; f <<= 1) {
			if (_save.flags & f) printf(" %s", flag_name(f));
		}
		printf("\n");
	}
}

static void list(void) {
	printf("\nSTORIES\n");
	for (uint32_t i = 0; i < story_count(); ++i) {
		const StoryProgress_t* p = story_progress(i);
		char c[64];
		cond_text(c, sizeof(c), &story_data[i].when);
		printf("  %2u  %-22s %3u%%  %-18s", i, story_name(i), story_data[i].chance, c);
		if (p->status == STORY_GOING) printf(" GOING, step %u", p->step);
		if (p->status == STORY_TOLD) printf(" told, ending %u", p->ending);
		printf("\n");
	}
}

// Lines from a page, or what an exit says, shortened for the list
static void preview(const char* text) {
	if (!text) return;
	const char* e = strchr(text, '\n');
	const int len = e ? (int)(e - text) : (int)strlen(text);
	printf("        \"%.*s%s\"\n", len > 50 ? 50 : len, text, len > 50 || e ? "..." : "");
}

// What the story waits for at its step: each exit, numbered from 1
static uint32_t waits(uint32_t i) {
	const StoryProgress_t* p = story_progress(i);
	const StoryStep_t* s = &story_data[i].steps[p->step];
	printf("\n%s, step %u (counter %u, %u jumps at this step)", story_name(i), p->step, p->counter, p->jumps);
	printf(", places %s", _gal[p->place[0]].name);
	for (uint32_t k = 1; k < STORY_PLACES; ++k) {
		if (p->place[k] != 0xff) printf(" / %s", _gal[p->place[k]].name);
	}
	printf("\n");
	state();
	// The log, as the ship's log shows it
	static char lines[12][STORY_LOG_LINE];
	bool choice[12];
	int32_t story;
	const uint32_t n = story_log(lines, choice, 12, &story);
	if (n && story == (int32_t)i) {
		printf("  so far:\n");
		for (uint32_t k = 0; k < n; ++k) printf("    %s%s\n", choice[k] ? "  " : "", lines[k]);
	}
	printf("  waits for:\n");
	for (uint32_t k = 0; k < s->n; ++k) {
		const StoryExit_t* x = &s->exits[k];
		char c[64];
		cond_text(c, sizeof(c), &x->when);
		const bool ok = story_holds(i, &x->when);
		printf("  %u) %s", k + 1, _event_names[x->ev]);
		switch (x->ev) {
			case STORY_DOCK:
			case STORY_ARRIVE:
			case STORY_LAUNCH:
			case STORY_DEPART:
			case STORY_APPROACH:
			case STORY_HERE:
				if (x->place == PLACE_ANY) printf(" anywhere");
				else printf(" at %s (%%%u)", _gal[p->place[x->place]].name, x->place);
				break;
			case STORY_SCOOP: printf(" %s", x->arg ? item_name(x->arg) : "anything"); break;
			case STORY_DISABLED:
			case STORY_DESTROYED:
			case STORY_ESCAPED:
			case STORY_NEAR: printf(" %s", x->arg ? ship_name(i, x->arg) : "any story ship"); break;
			case STORY_JUMP: printf(" x%u", x->arg ? x->arg : 1); break;
			default: break;
		}
		if (x->when.what != IF_ALWAYS) printf("   if %s %s", c, ok ? "(yes)" : "(NO)");
		if (x->choice[0].label) {
			printf("   -> choice");
		}
		else {
			char b[16];
			printf("   -> %s", next_text(x->choice[0].next, b, sizeof(b)));
		}
		printf("\n");
		preview(x->text);
	}
	return s->n;
}

// Shows the pages waiting, asking for each choice
static char _line[128];

static bool ask(const char* prompt) {
	printf("%s", prompt);
	fflush(stdout);
	if (!fgets(_line, sizeof(_line), stdin)) exit(0);
	_line[strcspn(_line, "\r\n")] = 0;
	return true;
}

static void pages(void) {
	StoryPage_t page;
	while (story_page(&page)) {
		printf("\n  +---- %s\n", page.title);
		for (uint32_t i = 0; i < page.lines; ++i) printf("  | %s\n", page.line[i]);
		if (!page.choices) {
			ask("  +---- (enter to go on) ");
			story_choose(0);
			continue;
		}
		for (uint32_t c = 0; c < page.choices; ++c) {
			printf("  +-> %u) %s%s\n", c + 1, page.choice[c], page.can[c] ? "" : "   (can't, here)");
		}
		for (;;) {
			ask("  choose: ");
			const int c = atoi(_line) - 1;
			if (c >= 0 && (uint32_t)c < page.choices && page.can[c]) {
				story_choose((uint32_t)c);
				break;
			}
		}
	}
}

// For going back a move
typedef struct {
	StorySave_t save;
	uint32_t cash, here;
	bool offender, witch, cabin;
	int8_t ship[SHIP_IDS];
} Snap_t;
#define UNDOS 64
static Snap_t _undo[UNDOS];
static uint32_t _undos;

static void push(void) {
	if (_undos == UNDOS) {
		memmove(_undo, _undo + 1, sizeof(Snap_t) * (UNDOS - 1));
		--_undos;
	}
	Snap_t* s = &_undo[_undos++];
	s->save = _save;
	s->cash = _cash;
	s->here = _here;
	s->offender = _offender;
	s->witch = _witch;
	s->cabin = _cabin;
	memcpy(s->ship, _ship, sizeof(_ship));
}

static bool pop(void) {
	if (!_undos) return false;
	const Snap_t* s = &_undo[--_undos];
	_save = s->save;
	_cash = s->cash;
	_here = s->here;
	_offender = s->offender;
	_witch = s->witch;
	_cabin = s->cabin;
	memcpy(_ship, s->ship, sizeof(_ship));
	return true;
}

static void help(void) {
	printf("\n"
		"  1-9   make that happen\n"
		"  j     jump once, anywhere\n"
		"  c     fit or remove the cabin\n"
		"  t     fit or remove the targeting computer\n"
		"  p     next companion aboard\n"
		"  $     add 1000 Cr\n"
		"  u     undo\n"
		"  l     back to the list\n"
		"  q     quit\n");
}

// Plays story i until it ends or we go back to the list
static void play(uint32_t i) {
	_undos = 0;
	for (;;) {
		const StoryProgress_t* p = story_progress(i);
		if (p->status != STORY_GOING) {
			if (p->status == STORY_TOLD) printf("\n=> %s ENDED, ending %u\n", story_name(i), p->ending);
			else printf("\n=> %s will be offered again later\n", story_name(i));
			state();
			ask("u to undo, enter for the list: ");
			if (_line[0] == 'u' && pop()) continue;
			return;
		}
		const uint32_t n = waits(i);
		ask("> (? for help) ");
		const char c = (char)tolower((unsigned char)_line[0]);
		if (c == 'q') exit(0);
		if (c == 'l') return;
		if (c == '?' || c == 'h') {
			help();
			continue;
		}
		if (c == 'u') {
			if (!pop()) printf("  nothing to undo\n");
			continue;
		}
		push();
		if (c == 'c') {
			_cabin = !_cabin;
			continue;
		}
		if (c == 't') {
			_targeting = !_targeting;
			continue;
		}
		if (c == '$') {
			_cash += 1000 * 10;
			continue;
		}
		if (c == 'p') {
			_save.companion = (uint8_t)((_save.companion + 1) % COMPANIONS);
			_save.loyalty = 4;
			_save.mood = 0;
			_save.away = 0;
			continue;
		}
		if (c == 'j') {
			printf("  * JUMP\n");
			story_event(STORY_JUMP, 0);
			pages();
			continue;
		}
		const int k = atoi(_line) - 1;
		if (k < 0 || (uint32_t)k >= n) {
			--_undos;
			continue;
		}
		const StoryExit_t* x = &story_data[i].steps[p->step].exits[k];
		uint32_t arg = x->arg;
		StoryEvent_e ev = (StoryEvent_e)x->ev;
		if (story_at_place(ev)) {
			arg = x->place == PLACE_ANY ? _here : p->place[x->place];
			_here = arg;
		}
		// Flying here, as if by launching
		if (ev == STORY_HERE) ev = STORY_DEPART;
		printf("  * %s\n", _event_names[ev]);
		if (x->ev == STORY_JUMP) {
			// As many jumps as it waits for, from where it is
			for (uint32_t j = p->jumps; j < (x->arg ? x->arg : 1u); ++j) story_event(STORY_JUMP, 0);
		}
		else {
			story_event(ev, arg);
		}
		pages();
	}
}

int main(void) {
	galaxy_make(0, _gal);
	story_reset(&_save);
	story_bind(&_save);
	_cash = 1000 * 10;
	_cabin = true;
	memset(_ship, -1, sizeof(_ship));
	printf("Story player: pick a story by number, then make things happen.\n"
		"Stories carry on from each other: what is told stays told.\n");
	for (;;) {
		list();
		state();
		ask("story number (r to reset, c cabin, t targeting, p companion, q quit): ");
		const char c = (char)tolower((unsigned char)_line[0]);
		if (c == 'q') return 0;
		if (c == 'c') {
			_cabin = !_cabin;
			continue;
		}
		if (c == 't') {
			_targeting = !_targeting;
			continue;
		}
		if (c == 'p') {
			_save.companion = (uint8_t)((_save.companion + 1) % COMPANIONS);
			_save.loyalty = 4;
			continue;
		}
		if (c == 'r') {
			story_reset(&_save);
			_cash = 1000 * 10;
			_offender = _witch = false;
			memset(_ship, -1, sizeof(_ship));
			continue;
		}
		if (!isdigit((unsigned char)c)) continue;
		const uint32_t i = (uint32_t)atoi(_line);
		if (i >= story_count()) continue;
		const StoryProgress_t* p = story_progress(i);
		if (p->status == STORY_GOING) {
			play(i);
			continue;
		}
		if (story_active() >= 0) {
			printf("  %s is going: finish it first\n", story_name((uint32_t)story_active()));
			continue;
		}
		if (p->status == STORY_TOLD) {
			printf("  told already (r to reset)\n");
			continue;
		}
		// The first system it can start from, from here
		uint32_t k = 0;
		for (; k < GALAXY_SYSTEMS; ++k) {
			if (story_start(i, (_here + k) % GALAXY_SYSTEMS)) break;
		}
		if (k == GALAXY_SYSTEMS) {
			char w[64];
			cond_text(w, sizeof(w), &story_data[i].when);
			printf("  can't start: needs %s\n", w);
			continue;
		}
		_here = story_progress(i)->place[0];
		printf("\n  * started at %s\n", _gal[_here].name);
		pages();
		play(i);
	}
}
