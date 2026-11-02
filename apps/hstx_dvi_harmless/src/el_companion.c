// Copyright (c) 2024 fruit-bat.

// See el_companion.h.

#include "el_companion.h"
#include "el_story_data.h"
#include <stdlib.h>

// Talking: no closer together than this, in frames; not the same thing
// again for this long; and after this long quiet, small talk. Most things
// are only remarked on sometimes, in percent.
#define TALK_GAP (5 * 60)
#define TALK_AGAIN (60 * 60)
#define TALK_QUIET (120 * 60)
#define TALK_CHANCE 60
#define PET_CHANCE 25
// Mood settles back by one this often, and is kept within this
#define MOOD_SETTLE (10 * 60)
#define MOOD_MAX 10
#define LOYALTY_MAX 20
// Joining, and below this when docked they leave
#define LOYALTY_START 4
#define MOOD_START 3
#define LOYALTY_LEAVE (-8)

// What one says about something, and how much they like it
typedef struct {
	int8_t likes;
	const char* line[2];
} Says_t;

// Pip, a nervous navigator: hates fighting and the sun, likes it quiet
static const Says_t _pip[CE_EVENTS] = {
	[CE_LAUNCH] = { 0, { "HERE WE GO AGAIN.", "SEATBELT. SEATBELT." } },
	[CE_DOCK] = { 2, { "SAFE! SAFE SAFE SAFE.", "I CAN FEEL MY LEGS AGAIN." } },
	[CE_DOCK_COMPUTER] = { 2, { "LET THE MACHINE DO IT. GOOD.", "NICE AND GENTLE." } },
	[CE_HYPERSPACE] = { 0, { "I HATE THIS BIT.", "EYES SHUT. TELL ME WHEN." } },
	[CE_WITCH] = { -4, { "OH NO. OH NO NO NO.", "THE STARS ARE THE WRONG COLOUR!" } },
	[CE_RAMMED] = { -3, { "THAT WAS CLOSE! TOO CLOSE!", "WE HIT SOMETHING!" } },
	[CE_HULL_HIT] = { -2, { "THEY'RE HITTING US!", "WAS THAT US? THAT WAS US." } },
	[CE_SHIELDS_LOW] = { -3, { "SHIELDS! THE SHIELDS!", "CAN WE GO HOME NOW?" } },
	[CE_LASER_HOT] = { 0, { "IS IT MEANT TO GLOW?", NULL } },
	[CE_CABIN_HOT] = { -3, { "IT'S GETTING WARM IN HERE.", "THE SUN! WE'RE TOO CLOSE!" } },
	[CE_FUEL_SCOOP] = { 0, { "HOW CLOSE DO WE NEED TO BE?", NULL } },
	[CE_CARGO_SCOOP] = { 1, { "GOT IT!", NULL } },
	[CE_OVERDRIVE] = { -1, { "TOO FAST TOO FAST", "WHEEE... I MEAN, CAREFUL." } },
	[CE_BOUNTY] = { 1, { "IS IT OVER? IS IT GONE?", NULL } },
	[CE_INNOCENT] = { -4, { "THEY WEREN'T PIRATES!", "WHY DID YOU DO THAT?" } },
	[CE_SHOOT_STATION] = { -6, { "WHAT ARE YOU DOING?!", NULL } },
	[CE_LOCK_POLICE] = { -1, { "THAT'S THE POLICE...", NULL } },
	[CE_LOW_FUEL] = { -2, { "WE HAVE ENOUGH FUEL, RIGHT?", NULL } },
	[CE_BOMB] = { -2, { "MY EARS! MY EVERYTHING!", NULL } },
	[CE_MISSILE_IN] = { -3, { "MISSILE! MISSILE!", "IT'S COMING STRAIGHT AT US!" } },
	[CE_PIRATES] = { -2, { "PIRATES! I SEE PIRATES!", "SOMETHING NASTY OUT THERE." } },
	[CE_ESCAPE] = { -2, { "WELL THAT WAS HORRID.", NULL } },
	[CE_QUIET] = { 1, { "IT'S NICE WHEN IT'S QUIET.", "DID I EVER TELL YOU... NO." } },
	[CE_JOIN] = { 0, { "I'LL TRY NOT TO BE SICK.", NULL } },
	[CE_BACK] = { 3, { "YOU CAME FOR ME!", NULL } },
};

// Marge, the ship's engineer: hates rams, heat and rough flying
static const Says_t _marge[CE_EVENTS] = {
	[CE_LAUNCH] = { 0, { "ENGINES ARE SWEET.", "TRY NOT TO SCRATCH HER." } },
	[CE_DOCK] = { 2, { "TEXTBOOK.", "NOT BAD. NOT BAD AT ALL." } },
	[CE_DOCK_COMPUTER] = { 0, { "LAZY. BUT TIDY.", NULL } },
	[CE_HYPERSPACE] = { 0, { "DRIVE'S HUMMING NICELY.", NULL } },
	[CE_WITCH] = { -2, { "THAT'S NOT IN THE MANUAL.", NULL } },
	[CE_RAMMED] = { -4, { "MY PAINTWORK!", "YOU BREAK IT, I FIX IT." } },
	[CE_HULL_HIT] = { -2, { "THAT'LL COST YOU.", "SHE FELT THAT ONE." } },
	[CE_SHIELDS_LOW] = { -2, { "SHIELDS ARE SHOT!", NULL } },
	[CE_LASER_HOT] = { -2, { "SHE'S TOO HOT, CAPTAIN.", "EASE OFF THE TRIGGER." } },
	[CE_CABIN_HOT] = { -2, { "MY TOOLS ARE MELTING.", NULL } },
	[CE_FUEL_SCOOP] = { 1, { "SCOOPS WORKING FINE.", NULL } },
	[CE_CARGO_SCOOP] = { 0, { "IN THE HOLD. MIND IT.", NULL } },
	[CE_OVERDRIVE] = { 0, { "LISTEN TO HER SING.", NULL } },
	[CE_BOUNTY] = { 1, { "MORE MONEY FOR PARTS.", NULL } },
	[CE_INNOCENT] = { -2, { "THAT WAS SOMEONE'S SHIP.", NULL } },
	[CE_SHOOT_STATION] = { -4, { "THEY HAD A GOOD WORKSHOP!", NULL } },
	[CE_LOCK_POLICE] = { 0, { "DON'T.", NULL } },
	[CE_LOW_FUEL] = { -1, { "TANK'S NEARLY DRY.", NULL } },
	[CE_BOMB] = { -2, { "THAT'S COMING OUT OF YOUR WAGES", NULL } },
	[CE_MISSILE_IN] = { -1, { "INCOMING. DO SOMETHING.", NULL } },
	[CE_PIRATES] = { -1, { "COMPANY.", NULL } },
	[CE_ESCAPE] = { -6, { "MY LOVELY SHIP...", NULL } },
	[CE_QUIET] = { 1, { "I COULD TUNE THAT DRIVE.", "SHE'S A GOOD OLD BUS." } },
	[CE_JOIN] = { 0, { "WHERE DO I PUT MY TOOLS?", NULL } },
	[CE_BACK] = { 3, { "MISSED ME? OF COURSE.", NULL } },
	[CE_PATCHED] = { 1, { "PATCHED YOU UP. AGAIN.", "SHIELDS BACK. SORT OF." } },
};

// Sly, a retired smuggler: likes profit, dislikes the police and fuss
static const Says_t _sly[CE_EVENTS] = {
	[CE_LAUNCH] = { 0, { "QUIETLY DOES IT.", NULL } },
	[CE_DOCK] = { 1, { "THE THIRD TIME'S THE CHARM.", "SMOOTH. LIKE ME." } },
	[CE_DOCK_COMPUTER] = { 0, { "NO HANDS. NICE.", NULL } },
	[CE_HYPERSPACE] = { 0, { "NEW SYSTEM, NEW FRIENDS.", NULL } },
	[CE_WITCH] = { -1, { "NOT A GOOD PLACE FOR DEALS.", NULL } },
	[CE_RAMMED] = { -2, { "WATCH IT!", NULL } },
	[CE_HULL_HIT] = { -1, { "OUCH. THAT WAS PERSONAL.", NULL } },
	[CE_SHIELDS_LOW] = { -2, { "TIME TO LEAVE, I THINK.", NULL } },
	[CE_LASER_HOT] = { 0, { "SUBTLE.", NULL } },
	[CE_CABIN_HOT] = { -1, { "I'M NOT DRESSED FOR THIS.", NULL } },
	[CE_FUEL_SCOOP] = { 2, { "FREE FUEL. MY FAVOURITE.", NULL } },
	[CE_CARGO_SCOOP] = { 3, { "FINDERS KEEPERS.", "PROFIT!" } },
	[CE_OVERDRIVE] = { 1, { "NOTHING CATCHES US NOW.", NULL } },
	[CE_BOUNTY] = { 2, { "KER-CHING.", NULL } },
	[CE_INNOCENT] = { -1, { "BAD FOR BUSINESS, THAT.", NULL } },
	[CE_SHOOT_STATION] = { -4, { "WE'LL NEVER SELL THERE NOW.", NULL } },
	[CE_LOCK_POLICE] = { -2, { "DON'T EVEN THINK ABOUT IT.", NULL } },
	[CE_LOW_FUEL] = { -1, { "RUNNING ON FUMES.", NULL } },
	[CE_BOMB] = { 0, { "WELL, THAT SETTLES THAT.", NULL } },
	[CE_MISSILE_IN] = { -1, { "SOMEONE DOESN'T LIKE YOU.", NULL } },
	[CE_PIRATES] = { 0, { "OLD FRIENDS. NOT FRIENDLY.", NULL } },
	[CE_ESCAPE] = { -3, { "I KNEW A GUY... NEVER MIND.", NULL } },
	[CE_QUIET] = { 0, { "I KNEW A GUY ON LAVE...", "OFFICER, LOVELY TO SEE YOU." } },
	[CE_JOIN] = { 0, { "NO QUESTIONS. I LIKE THAT.", NULL } },
	[CE_BACK] = { 2, { "TOLD THEM NOTHING.", NULL } },
};

// Professor Quill, a xenobiologist: loves the strange, hates killing
static const Says_t _quill[CE_EVENTS] = {
	[CE_LAUNCH] = { 0, { "SO MUCH TO STUDY!", NULL } },
	[CE_DOCK] = { 0, { "MOST SATISFACTORY.", NULL } },
	[CE_DOCK_COMPUTER] = { 1, { "ELEGANT MACHINE.", NULL } },
	[CE_HYPERSPACE] = { 1, { "FASCINATING DISTORTION.", NULL } },
	[CE_WITCH] = { 4, { "OH, MARVELLOUS!", "LOOK AT THE COLOURS!" } },
	[CE_RAMMED] = { -2, { "MIND THE SPECIMENS!", NULL } },
	[CE_HULL_HIT] = { -1, { "MY NOTES! EVERYWHERE!", NULL } },
	[CE_SHIELDS_LOW] = { -2, { "THIS IS MOST UNWISE.", NULL } },
	[CE_LASER_HOT] = { -1, { "MUST WE SHOOT SO MUCH?", NULL } },
	[CE_CABIN_HOT] = { 0, { "A STELLAR SAUNA. NOTED.", NULL } },
	[CE_FUEL_SCOOP] = { 1, { "SOLAR PLASMA! DELICIOUS.", NULL } },
	[CE_CARGO_SCOOP] = { 0, { "WHAT HAVE WE HERE?", NULL } },
	[CE_OVERDRIVE] = { 0, { "RELATIVITY, HO!", NULL } },
	[CE_BOUNTY] = { -1, { "WAS THAT NECESSARY?", NULL } },
	[CE_INNOCENT] = { -4, { "THAT WAS UNFORGIVABLE.", NULL } },
	[CE_SHOOT_STATION] = { -3, { "BARBARIC!", NULL } },
	[CE_LOCK_POLICE] = { 0, { "I WOULDN'T.", NULL } },
	[CE_LOW_FUEL] = { 0, { "WE MAY BE STRANDED. HOW NOVEL.", NULL } },
	[CE_BOMB] = { -3, { "EVERYTHING... GONE.", NULL } },
	[CE_MISSILE_IN] = { -1, { "A MISSILE! HOW RUDE.", NULL } },
	[CE_PIRATES] = { 0, { "HOSTILE HOMINIDS AHEAD.", NULL } },
	[CE_ESCAPE] = { -2, { "MY SPECIMENS!", NULL } },
	[CE_QUIET] = { 1, { "THARGOIDS HAVE SIX HEARTS.", "THE STARS SING, YOU KNOW." } },
	[CE_JOIN] = { 0, { "SPLENDID. ONWARD!", NULL } },
	[CE_BACK] = { 3, { "I TOOK NOTES ON THEM.", NULL } },
};

static const Says_t* const _says[COMPANIONS] = { NULL, _pip, _marge, _sly, _quill };
static const char* const _names[COMPANIONS] = { NULL, "PIP", "MARGE", "SLY", "QUILL" };
// What they leave us, going
static const char* const _farewells[COMPANIONS] = {
	NULL,
	"PIP HAS LEFT A NOTE:\nI'M SORRY. I CAN'T DO THIS.\nTHE SHOOTING. THE SUN.\nTAKE CARE OF YOURSELF.",
	"MARGE HAS LEFT A NOTE:\nTOO MANY DENTS, CAPTAIN.\nI'VE A NEW JOB AT THE YARD.\nLOOK AFTER HER.",
	"SLY HAS LEFT A NOTE:\nNOTHING PERSONAL.\nI'VE HAD A BETTER OFFER.\nI ALWAYS DO.",
	"QUILL HAS LEFT A NOTE:\nI CANNOT STUDY LIFE\nWITH ONE WHO ENDS SO MUCH OF IT.\nFAREWELL.",
};

// The pets only have a little to say
static const Says_t _kitten[CE_EVENTS] = {
	[CE_DOCK] = { 0, { "PURRS.", NULL } },
	[CE_HYPERSPACE] = { 0, { "HIDES UNDER THE SEAT.", NULL } },
	[CE_CABIN_HOT] = { 0, { "STRETCHES IN THE WARM.", NULL } },
	[CE_QUIET] = { 0, { "IS ASLEEP ON THE SCANNER.", "MEOW." } },
	[CE_MISSILE_IN] = { 0, { "HISSES!", NULL } },
};
static const Says_t _parrot[CE_EVENTS] = {
	[CE_DOCK] = { 0, { "SQUAWK! PARKED IT!", NULL } },
	[CE_LAUNCH] = { 0, { "AWAY WE GO! AWAY!", NULL } },
	[CE_HULL_HIT] = { 0, { "THAT'LL BUFF OUT!", NULL } },
	[CE_CARGO_SCOOP] = { 0, { "PIECES OF EIGHT!", NULL } },
	[CE_OVERDRIVE] = { 0, { "WHEEEE!", NULL } },
	[CE_BOUNTY] = { 0, { "DEAD MEN TELL NO TALES!", NULL } },
	[CE_QUIET] = { 0, { "WHO'S A PRETTY BOY?", "SQUAWK!" } },
};
static const Says_t* const _pet_says[PETS] = { NULL, _kitten, _parrot };
static const char* const _pet_names[PETS] = { NULL, "KITTEN", "PARROT" };

static const char* const _event_names[CE_EVENTS] = {
	"LAUNCH", "DOCK", "DOCK_COMPUTER", "HYPERSPACE", "WITCH", "RAMMED", "HULL_HIT",
	"SHIELDS_LOW", "LASER_HOT", "CABIN_HOT", "FUEL_SCOOP", "CARGO_SCOOP", "OVERDRIVE",
	"BOUNTY", "INNOCENT", "SHOOT_STATION", "LOCK_POLICE", "LOW_FUEL", "BOMB",
	"MISSILE_IN", "PIRATES", "ESCAPE", "QUIET", "JOIN", "BACK", "PATCHED",
};

// When they last spoke, and last said each thing, in frames
static uint32_t _frame;
static uint32_t _spoke, _pet_spoke;
static uint32_t _said[CE_EVENTS];
static bool _ever_said[CE_EVENTS];

// What matters enough to say in a fight
static bool urgent(CompanionEvent_e e) {
	switch (e) {
		case CE_RAMMED: case CE_SHIELDS_LOW: case CE_MISSILE_IN: case CE_PIRATES:
		case CE_PATCHED: case CE_ESCAPE: case CE_SHOOT_STATION: case CE_INNOCENT:
		case CE_JOIN: case CE_BACK:
			return true;
		default:
			return false;
	}
}

static int32_t clamp(int32_t v, int32_t m) {
	return v > m ? m : v < -m ? -m : v;
}

static const char* pick(const Says_t* s) {
	if (!s->line[0]) return NULL;
	if (s->line[1] && (rand() & 1)) return s->line[1];
	return s->line[0];
}

Companion_e companion(void) {
	const StorySave_t* s = story_save();
	return s && s->companion && !s->away ? (Companion_e)s->companion : COMPANION_NONE;
}

const char* companion_name_of(uint32_t c) {
	return c < COMPANIONS ? _names[c] : NULL;
}

const char* companion_name(void) {
	return _names[companion()];
}

const char* companion_mood_name(void) {
	const StorySave_t* s = story_save();
	if (!companion()) return NULL;
	return s->mood >= 4 ? "Happy" : s->mood >= 0 ? "Fine" : s->mood >= -4 ? "Nervous" : "Cross";
}

const char* pet_name(void) {
	const StorySave_t* s = story_save();
	return s && s->pet < PETS ? _pet_names[s->pet] : NULL;
}

void companion_event(CompanionEvent_e e, bool busy) {
	StorySave_t* s = story_save();
	if (!s) return;
	const Companion_e c = companion();
	if (c) {
		const Says_t* says = &_says[c][e];
		s->mood = (int8_t)clamp(s->mood + says->likes, MOOD_MAX);
		const bool now = !busy || urgent(e);
		const bool again = !_ever_said[e] || _frame - _said[e] >= TALK_AGAIN;
		const bool chance = urgent(e) || (uint32_t)(rand() % 100) < TALK_CHANCE;
		const char* line = pick(says);
		if (line && now && again && chance && _frame - _spoke >= TALK_GAP) {
			story_host_say(SPEAK_COMPANION, _names[c], line);
			_spoke = _frame;
			_said[e] = _frame;
			_ever_said[e] = true;
			return;
		}
	}
	// The pet now and then, when the companion says nothing
	if (s->pet && s->pet < PETS && !busy && (uint32_t)(rand() % 100) < PET_CHANCE && _frame - _pet_spoke >= TALK_GAP * 4) {
		const char* line = pick(&_pet_says[s->pet][e]);
		if (line) {
			story_host_say(SPEAK_PET, _pet_names[s->pet], line);
			_pet_spoke = _frame;
		}
	}
}

void companion_tick(bool flying, bool busy) {
	StorySave_t* s = story_save();
	++_frame;
	if (!s) return;
	if (_frame % MOOD_SETTLE == 0) {
		if (s->mood > 0) --s->mood;
		else if (s->mood < 0) ++s->mood;
	}
	if (flying && !busy && _frame - _spoke >= TALK_QUIET && _frame - _pet_spoke >= TALK_QUIET) {
		_spoke = _pet_spoke = _frame;
		// Quiet for long enough: someone fills the silence
		const Companion_e c = companion();
		const char* line = c ? pick(&_says[c][CE_QUIET]) : NULL;
		if (line) story_host_say(SPEAK_COMPANION, _names[c], line);
		else if (s->pet && s->pet < PETS && (line = pick(&_pet_says[s->pet][CE_QUIET])) != NULL) story_host_say(SPEAK_PET, _pet_names[s->pet], line);
	}
}

void companion_dock(void) {
	StorySave_t* s = story_save();
	const Companion_e c = companion();
	if (!c) return;
	// A good trip counts for a little, a bad one for more
	const int32_t by = s->mood > 0 ? (s->mood + 2) / 3 : s->mood < 0 ? (s->mood - 1) / 2 : 0;
	s->loyalty = (int8_t)clamp(s->loyalty + by, LOYALTY_MAX);
	s->mood = (int8_t)(s->mood / 2);
	if (s->loyalty <= LOYALTY_LEAVE) {
		story_note("A NOTE", _farewells[c]);
		companion_leave();
	}
}

void companion_join(uint32_t c) {
	StorySave_t* s = story_save();
	if (c == COMPANION_NONE || c >= COMPANIONS) return;
	s->companion = (uint8_t)c;
	s->away = 0;
	s->mood = MOOD_START;
	s->loyalty = LOYALTY_START;
	_spoke = _frame - TALK_GAP;
	companion_event(CE_JOIN, false);
}

void companion_leave(void) {
	StorySave_t* s = story_save();
	s->companion = COMPANION_NONE;
	s->away = 0;
	s->mood = s->loyalty = 0;
}

void companion_away(bool away) {
	StorySave_t* s = story_save();
	if (!s->companion) return;
	s->away = away;
	if (!away) {
		_spoke = _frame - TALK_GAP;
		companion_event(CE_BACK, false);
	}
}

void companion_loyalty(int32_t by, uint32_t only) {
	StorySave_t* s = story_save();
	if (!s->companion || (only && only != s->companion)) return;
	s->loyalty = (int8_t)clamp(s->loyalty + by, LOYALTY_MAX);
}

void companion_pet(uint32_t p) {
	StorySave_t* s = story_save();
	s->pet = (uint8_t)(p < PETS ? p : PET_NONE);
}

static uint32_t lines_of(const Says_t* says, const char** lines, uint32_t max) {
	uint32_t n = 0;
	for (uint32_t i = 0; i < 2 && n < max; ++i) {
		if (says->line[i]) lines[n++] = says->line[i];
	}
	return n;
}

uint32_t companion_lines(uint32_t c, CompanionEvent_e e, const char** lines, uint32_t max) {
	if (c == COMPANION_NONE || c >= COMPANIONS) return 0;
	return lines_of(&_says[c][e], lines, max);
}

uint32_t pet_lines(uint32_t p, CompanionEvent_e e, const char** lines, uint32_t max) {
	if (p == PET_NONE || p >= PETS) return 0;
	return lines_of(&_pet_says[p][e], lines, max);
}

int32_t companion_likes(uint32_t c, CompanionEvent_e e) {
	return c && c < COMPANIONS ? _says[c][e].likes : 0;
}

const char* companion_event_name(CompanionEvent_e e) {
	return _event_names[e];
}
