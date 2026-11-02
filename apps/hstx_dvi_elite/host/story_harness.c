// Walks every story through every choice on the PC, printing what is shown
// and done, and checks the text fits; then what each companion and pet says,
// and a companion leaving.
//
// host/story_harness.sh builds and runs it. It exits with 1 if anything is
// wrong: text too long, a step never reached, or a story with no ending.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "el_story_data.h"
#include "el_companion.h"

// The longest page line and choice, and a line said in flight with its name
#define PAGE_WIDTH 40
#define CHOICE_WIDTH 28
#define SAY_WIDTH 38

static GalaxySystem_t _gal[GALAXY_SYSTEMS];
static uint32_t _here;
static bool _cabin;
static uint32_t _cash;
static bool _offender, _witch;
static int _depth;
static int _problems;
static StorySave_t _save;

static void indent(void) {
	for (int i = 0; i < _depth; ++i) printf("  ");
}

static void problem(const char* what, const char* text) {
	indent();
	printf("!! %s: \"%s\"\n", what, text);
	++_problems;
}

// What the game would do
const GalaxySystem_t* story_host_system(uint32_t i) { return &_gal[i % GALAXY_SYSTEMS]; }
uint32_t story_host_galaxy(void) { return 0; }
uint32_t story_host_here(void) { return _here; }
bool story_host_has_cabin(void) { return _cabin; }
uint32_t story_host_cash(void) { return _cash; }

void story_host_add_cash(int32_t tenths) {
	_cash = (int32_t)_cash + tenths < 0 ? 0 : (uint32_t)((int32_t)_cash + tenths);
	indent();
	printf("[cash %+d Cr, now %u]\n", tenths / 10, _cash / 10);
}

void story_host_offender(bool on) {
	_offender = on;
	indent();
	printf("[%s]\n", on ? "offender" : "record cleared");
}

void story_host_spawn(uint32_t id, const StoryShip_t* s) {
	static const char* const goals[] = { "fights", "idles", "races", "drifts", "follows", "weaves" };
	indent();
	printf("[ship %u: %s, a %s, %s%s%s%s]\n", id, s->label, s->cls, goals[s->goal],
		s->disable ? ", can be disabled" : "", s->drop ? ", leaves " : "", s->drop ? story_items[s->drop].name : "");
}

void story_host_canister(uint32_t item) {
	indent();
	printf("[canister ahead: %s]\n", story_items[item].name);
}

void story_host_calm(void) {
	indent();
	printf("[story ships calm down]\n");
}

void story_host_witch(void) {
	_witch = true;
	indent();
	printf("[next jump goes into witchspace]\n");
}

void story_host_laser(uint32_t laser) {
	indent();
	printf("[front laser %u fitted]\n", laser);
}

void story_host_say(Speaker_e who, const char* name, const char* text) {
	(void)who;
	indent();
	printf("%s: %s\n", name, text);
	if (strlen(name) + 2 + strlen(text) > SAY_WIDTH) problem("said line too long", text);
}

static const char* const _event_names[STORY_EVENTS] = {
	"DOCK", "ARRIVE", "JUMP", "WITCH", "SCOOP", "DISABLED", "DESTROYED", "ESCAPED", "NEAR", "HULL_HIT", "SUN", "NOW"
};

// ----------------------------------------------------------------------------

// What has been reached in the story being walked
static bool _step_seen[64];
static bool _ending_seen[16];
static uint32_t _states;
#define STATES_MAX 4096
static StorySave_t _seen[STATES_MAX];
static uint32_t _seen_n;

typedef struct {
	StorySave_t save;
	uint32_t cash, here;
	bool offender, witch, cabin;
	StoryPending_t pending;
} Snap_t;

static void snap(Snap_t* s) {
	s->save = _save;
	s->cash = _cash;
	s->here = _here;
	s->offender = _offender;
	s->witch = _witch;
	s->cabin = _cabin;
	story_pending_keep(&s->pending);
}

static void unsnap(const Snap_t* s) {
	_save = s->save;
	_cash = s->cash;
	_here = s->here;
	_offender = s->offender;
	_witch = s->witch;
	_cabin = s->cabin;
	story_pending_put(&s->pending);
}

static bool seen_before(void) {
	for (uint32_t i = 0; i < _seen_n; ++i) {
		if (!memcmp(&_seen[i], &_save, sizeof(_save))) return true;
	}
	if (_seen_n < STATES_MAX) _seen[_seen_n++] = _save;
	return false;
}

static void walk(uint32_t story);

// Shows the page waiting, if any, and goes down each choice
static bool pages(uint32_t story) {
	StoryPage_t page;
	if (!story_page(&page)) return false;
	indent();
	printf("+-- %s\n", page.title);
	for (uint32_t i = 0; i < page.lines; ++i) {
		indent();
		printf("| %s\n", page.line[i]);
		if (strlen(page.line[i]) > PAGE_WIDTH) problem("page line too long", page.line[i]);
	}
	if (!page.choices) {
		indent();
		printf("+-- (fire to go on)\n");
		story_choose(0);
		if (!pages(story)) walk(story);
		return true;
	}
	Snap_t before;
	snap(&before);
	for (uint32_t c = 0; c < page.choices; ++c) {
		if (strlen(page.choice[c]) > CHOICE_WIDTH) problem("choice too long", page.choice[c]);
		indent();
		if (!page.can[c]) {
			printf("+-> %s (can't, here)\n", page.choice[c]);
			continue;
		}
		printf("+-> %s\n", page.choice[c]);
		unsnap(&before);
		++_depth;
		story_choose(c);
		if (!pages(story)) walk(story);
		--_depth;
	}
	return true;
}

// Makes each thing the story waits for happen, in turn, from here
static void walk(uint32_t story) {
	const StoryProgress_t* p = story_progress(story);
	if (p->status == STORY_TOLD) {
		indent();
		printf("=> ENDING %u\n", p->ending);
		_ending_seen[p->ending] = true;
		return;
	}
	if (p->status != STORY_GOING) {
		indent();
		printf("=> offered again later\n");
		return;
	}
	_step_seen[p->step] = true;
	if (_depth > 40) {
		indent();
		printf("!! too deep\n");
		++_problems;
		return;
	}
	if (seen_before()) {
		indent();
		printf("(as before)\n");
		return;
	}
	++_states;
	StoryEvent_e ev[8];
	uint32_t arg[8];
	const uint32_t n = story_waits(story, ev, arg, 8);
	Snap_t before;
	snap(&before);
	for (uint32_t k = 0; k < n; ++k) {
		// Once for each different thing, as exits for the same thing differ only in their conditions
		bool again = false;
		for (uint32_t j = 0; j < k; ++j) again |= ev[j] == ev[k] && arg[j] == arg[k];
		if (again) continue;
		unsnap(&before);
		uint32_t a = arg[k];
		if ((ev[k] == STORY_DOCK || ev[k] == STORY_ARRIVE) && a == 0xffffffffu) a = p->place[0];
		if (ev[k] == STORY_DOCK || ev[k] == STORY_ARRIVE) _here = a;
		indent();
		if (ev[k] == STORY_DOCK || ev[k] == STORY_ARRIVE) printf("* %s %s\n", _event_names[ev[k]], _gal[a].name);
		else printf("* %s %u\n", _event_names[ev[k]], a);
		++_depth;
		const StorySave_t was = _save;
		if (ev[k] == STORY_JUMP) {
			// As many jumps as it waits for
			for (uint32_t j = 0; j < (a ? a : 1); ++j) story_event(STORY_JUMP, 0);
		}
		else {
			story_event(ev[k], a);
		}
		if (!pages(story)) {
			if (!memcmp(&was, &_save, sizeof(was))) {
				indent();
				printf("(nothing changes)\n");
			}
			else {
				walk(story);
			}
		}
		--_depth;
	}
}

// Walks story i with or without a cabin and a companion aboard
static void walk_story(uint32_t i, bool cabin, uint32_t comp) {
	story_reset(&_save);
	story_bind(&_save);
	_cash = 1000 * 10;
	_offender = _witch = false;
	_cabin = cabin;
	// What was seen without a cabin may go differently with one
	_seen_n = 0;
	if (comp) {
		_save.companion = (uint8_t)comp;
		_save.loyalty = 4;
	}
	printf("\n==== %s (%s, %s) ====\n", story_name(i), cabin ? "cabin" : "no cabin",
		comp ? companion_name_of(comp) : "nobody aboard");
	// The first system it can start from
	uint32_t here = 0;
	for (; here < GALAXY_SYSTEMS; ++here) {
		if (story_start(i, here)) break;
	}
	if (here == GALAXY_SYSTEMS) {
		printf("!! cannot start anywhere\n");
		++_problems;
		return;
	}
	const StoryProgress_t* p = story_progress(i);
	printf("from %s", _gal[p->place[0]].name);
	for (uint32_t k = 1; k < STORY_PLACES; ++k) {
		if (p->place[k] != 0xff) printf(", %%%u is %s", k, _gal[p->place[k]].name);
	}
	printf("\n");
	_here = here;
	_depth = 0;
	walk(i);
}

static void companions(void) {
	printf("\n==== COMPANIONS ====\n");
	for (uint32_t c = 1; c < COMPANIONS; ++c) {
		printf("\n-- %s\n", companion_name_of(c));
		for (uint32_t e = 0; e < CE_EVENTS; ++e) {
			const char* lines[2];
			const uint32_t n = companion_lines(c, (CompanionEvent_e)e, lines, 2);
			const int32_t likes = companion_likes(c, (CompanionEvent_e)e);
			if (!n && !likes) continue;
			printf("  %-14s %+d", companion_event_name((CompanionEvent_e)e), likes);
			for (uint32_t k = 0; k < n; ++k) {
				printf("  \"%s\"", lines[k]);
				if (strlen(companion_name_of(c)) + 2 + strlen(lines[k]) > SAY_WIDTH) problem("companion line too long", lines[k]);
			}
			printf("\n");
		}
	}
	static const char* const pets[] = { NULL, "KITTEN", "PARROT" };
	for (uint32_t p = 1; p < PETS; ++p) {
		printf("\n-- %s\n", pets[p]);
		for (uint32_t e = 0; e < CE_EVENTS; ++e) {
			const char* lines[2];
			const uint32_t n = pet_lines(p, (CompanionEvent_e)e, lines, 2);
			if (!n) continue;
			printf("  %-14s", companion_event_name((CompanionEvent_e)e));
			for (uint32_t k = 0; k < n; ++k) {
				printf("  \"%s\"", lines[k]);
				if (strlen(pets[p]) + 2 + strlen(lines[k]) > SAY_WIDTH) problem("pet line too long", lines[k]);
			}
			printf("\n");
		}
	}

	// A rough trip with Pip: rammed and shot, then docking, until Pip leaves
	printf("\n-- A rough time with PIP, who hates it\n");
	story_reset(&_save);
	story_bind(&_save);
	companion_join(COMPANION_PIP);
	for (uint32_t trip = 1; trip <= 6 && companion(); ++trip) {
		for (uint32_t f = 0; f < 600; ++f) {
			companion_tick(true, false);
			if (f == 100) companion_event(CE_RAMMED, false);
			if (f == 400) companion_event(CE_HULL_HIT, false);
			if (f == 500) companion_event(CE_CABIN_HOT, false);
		}
		companion_dock();
		printf("  after trip %u: mood %d, loyalty %d%s\n", trip, _save.mood, _save.loyalty, companion() ? "" : ", gone");
		StoryPage_t page;
		if (story_page(&page)) {
			for (uint32_t i = 0; i < page.lines; ++i) printf("  | %s\n", page.line[i]);
			story_choose(0);
		}
	}
	if (companion()) {
		printf("!! Pip never left\n");
		++_problems;
	}

	// And a good trip with Marge, docking well
	printf("\n-- A smooth time with MARGE\n");
	story_reset(&_save);
	story_bind(&_save);
	companion_join(COMPANION_MARGE);
	for (uint32_t trip = 1; trip <= 3; ++trip) {
		for (uint32_t f = 0; f < 600; ++f) companion_tick(true, false);
		companion_event(CE_DOCK, false);
		companion_dock();
		printf("  after trip %u: mood %d, loyalty %d\n", trip, _save.mood, _save.loyalty);
	}
}

int main(int argc, char** argv) {
	galaxy_make(0, _gal);
	srand(1);
	const int only = argc > 1 ? atoi(argv[1]) : -1;
	for (uint32_t i = 0; i < story_count(); ++i) {
		if (only >= 0 && (uint32_t)only != i) continue;
		memset(_step_seen, 0, sizeof(_step_seen));
		memset(_ending_seen, 0, sizeof(_ending_seen));
		_states = 0;
		_seen_n = 0;
		// Nobody, then a cabin, then with Quill aboard, or for a kidnap with Pip
		const bool needs_one = story_data[i].when.what == IF_COMPANION;
		const bool needs_none = story_data[i].when.what == IF_NO_COMPANION;
		if (!needs_one) {
			walk_story(i, false, COMPANION_NONE);
			walk_story(i, true, COMPANION_NONE);
		}
		if (!needs_none) walk_story(i, true, needs_one ? COMPANION_PIP : COMPANION_QUILL);
		printf("\n-- %s: %u states;", story_name(i), _states);
		printf(" endings");
		for (uint32_t e = 0; e < 16; ++e) {
			if (_ending_seen[e]) printf(" %u", e);
		}
		printf("\n");
		for (uint32_t s = 0; s < story_data[i].n; ++s) {
			if (!_step_seen[s]) {
				// A step only passed through at once is seen as it goes
				bool now = false;
				for (uint32_t x = 0; x < story_data[i].steps[s].n; ++x) now |= story_data[i].steps[s].exits[x].ev == STORY_NOW;
				if (now) continue;
				printf("!! step %u never reached\n", s);
				++_problems;
			}
		}
		bool any = false;
		for (uint32_t e = 0; e < 16; ++e) any |= _ending_seen[e];
		if (!any) {
			printf("!! no ending reached\n");
			++_problems;
		}
	}
	if (only < 0) companions();
	printf("\n%d problem%s\n", _problems, _problems == 1 ? "" : "s");
	return _problems ? 1 : 0;
}
