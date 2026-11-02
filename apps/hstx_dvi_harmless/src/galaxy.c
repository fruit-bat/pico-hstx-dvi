// Copyright (c) 2024 fruit-bat.

// The galaxies are made as in Ian Bell's Text Elite, which follows the
// original game: three 16 bit numbers, the seed, are turned like a Fibonacci
// sequence to give each system in turn, and twisted to give the next galaxy.

#include "galaxy.h"
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

typedef struct {
	uint16_t w0, w1, w2;
} Seed_t;

// The seed of the first galaxy
static const Seed_t _first = { 0x5A4A, 0x0248, 0xB753 };

// The pairs of letters the names are made of. A dot is left out.
static const char _pairs[] = "..LEXEGEZACEBISOUSESARMAINDIREA.ERATENBERALAVETIEDORQUANTEISRION";

static void tweak(Seed_t* s) {
	const uint16_t t = (uint16_t)(s->w0 + s->w1 + s->w2);
	s->w0 = s->w1;
	s->w1 = s->w2;
	s->w2 = t;
}

// Turns each byte left one bit, for the next galaxy
static uint16_t twist(uint16_t x) {
	const uint32_t h = x >> 8, l = x & 0xff;
	return (uint16_t)((((h << 1) | (h >> 7)) & 0xff) << 8 | (((l << 1) | (l >> 7)) & 0xff));
}

static void make_system(Seed_t* s, GalaxySystem_t* sys) {
	sys->seed[0] = s->w0;
	sys->seed[1] = s->w1;
	sys->seed[2] = s->w2;
	sys->x = (uint8_t)(s->w1 >> 8);
	sys->y = (uint8_t)(s->w0 >> 8);
	sys->government = (s->w1 >> 3) & 7;
	sys->economy = (s->w0 >> 8) & 7;
	if (sys->government <= 1) sys->economy |= 2;
	uint32_t tech = ((s->w1 >> 8) & 3) + (sys->economy ^ 7) + (sys->government >> 1);
	if (sys->government & 1) ++tech;
	sys->tech = (uint8_t)(tech + 1);
	sys->population = (uint8_t)(4 * tech + sys->economy + sys->government + 1);
	sys->radius = (uint16_t)(256 * (((s->w2 >> 8) & 15) + 11) + sys->x);

	// Four pairs of letters, or three for a short name
	const bool long_name = s->w0 & 64;
	uint32_t n = 0;
	for (uint32_t i = 0; i < 4; ++i) {
		const uint32_t p = 2 * ((s->w2 >> 8) & 31);
		tweak(s);
		if (i == 3 && !long_name) continue;
		for (uint32_t j = 0; j < 2; ++j) {
			if (_pairs[p + j] != '.') sys->name[n++] = _pairs[p + j];
		}
	}
	sys->name[n] = 0;
}

void galaxy_make(uint32_t g, GalaxySystem_t systems[GALAXY_SYSTEMS]) {
	Seed_t s = _first;
	for (uint32_t i = 0; i < g % GALAXIES; ++i) {
		s.w0 = twist(s.w0);
		s.w1 = twist(s.w1);
		s.w2 = twist(s.w2);
	}
	for (uint32_t i = 0; i < GALAXY_SYSTEMS; ++i) make_system(&s, &systems[i]);
}

uint32_t galaxy_distance(const GalaxySystem_t* a, const GalaxySystem_t* b) {
	// The chart is twice as tall as it is shown
	const float dx = (float)a->x - (float)b->x;
	const float dy = ((float)a->y - (float)b->y) * 0.5f;
	return (uint32_t)(4.0f * sqrtf(dx * dx + dy * dy));
}

const char* const galaxy_economy_names[8] = {
	"Rich Industrial", "Average Industrial", "Poor Industrial", "Mainly Industrial",
	"Mainly Agricultural", "Rich Agricultural", "Average Agricultural", "Poor Agricultural"
};

const char* const galaxy_government_names[8] = {
	"Anarchy", "Feudal", "Multi-Government", "Dictatorship",
	"Communist", "Confederacy", "Democracy", "Corporate State"
};

// ----------------------------------------------------------------------------
// Who lives there, as in the BBC Micro version

static const char* const _sizes[3] = { "Large", "Fierce", "Small" };
static const char* const _colours[6] = { "Green", "Red", "Yellow", "Blue", "Black", "Harmless" };
static const char* const _looks[6] = { "Slimy", "Bug-Eyed", "Horned", "Bony", "Fat", "Furry" };
static const char* const _kinds[8] = { "Rodents", "Frogs", "Lizards", "Lobsters", "Birds", "Humanoids", "Felines", "Insects" };

// Adds a word to a string of n bytes, with a space before it if needed
static void add_word(char* out, size_t n, const char* w) {
	const size_t l = strlen(out);
	snprintf(out + l, n - l, "%s%s", l ? " " : "", w);
}

void galaxy_species(const GalaxySystem_t* sys, char* out, size_t n) {
	if (!n) return;
	out[0] = 0;
	if (!(sys->seed[2] & 0x80)) {
		snprintf(out, n, "Human Colonials");
		return;
	}
	const uint32_t s2 = sys->seed[2] >> 8;
	const uint32_t size = (s2 >> 2) & 7;
	if (size < 3) add_word(out, n, _sizes[size]);
	const uint32_t colour = s2 >> 5;
	if (colour < 6) add_word(out, n, _colours[colour]);
	const uint32_t look = ((sys->seed[0] >> 8) ^ (sys->seed[1] >> 8)) & 7;
	if (look < 6) add_word(out, n, _looks[look]);
	add_word(out, n, _kinds[((s2 & 3) + look) & 7]);
}

// ----------------------------------------------------------------------------
// What the system is known for, as in Text Elite. A byte from 0x81 is one of
// five choices from the table below, picked by a random number made from the
// system's seed; 0xB0 is the name of the system, 0xB1 the name made into
// "-ian", and 0xB2 a made-up name.

static const char* const _choices[][5] = {
	/* 81 */ { "fabled", "notable", "well known", "famous", "noted" },
	/* 82 */ { "very", "mildly", "most", "reasonably", "" },
	/* 83 */ { "ancient", "\x95", "great", "vast", "pink" },
	/* 84 */ { "\x9E \x9D plantations", "mountains", "\x9C", "\x94 forests", "oceans" },
	/* 85 */ { "shyness", "silliness", "mating traditions", "loathing of \x86", "love for \x86" },
	/* 86 */ { "food blenders", "tourists", "poetry", "discos", "\x8E" },
	/* 87 */ { "talking tree", "crab", "bat", "lobst", "\xB2" },
	/* 88 */ { "beset", "plagued", "ravaged", "cursed", "scourged" },
	/* 89 */ { "\x96 civil war", "\x9B \x98 \x99s", "a \x9B disease", "\x96 earthquakes", "\x96 solar activity" },
	/* 8A */ { "its \x83 \x84", "the \xB1 \x98 \x99", "its inhabitants' \x9A \x85", "\xA1", "its \x8D \x8E" },
	/* 8B */ { "juice", "brandy", "water", "brew", "gargle blasters" },
	/* 8C */ { "\xB2", "\xB1 \x99", "\xB1 \xB2", "\xB1 \x9B", "\x9B \xB2" },
	/* 8D */ { "fabulous", "exotic", "hoopy", "unusual", "exciting" },
	/* 8E */ { "cuisine", "night life", "casinos", "sit coms", " \xA1 " },
	/* 8F */ { "\xB0", "The planet \xB0", "The world \xB0", "This planet", "This world" },
	/* 90 */ { "n unremarkable", " boring", " dull", " tedious", " revolting" },
	/* 91 */ { "planet", "world", "place", "little planet", "dump" },
	/* 92 */ { "wasp", "moth", "grub", "ant", "\xB2" },
	/* 93 */ { "poet", "arts graduate", "yak", "snail", "slug" },
	/* 94 */ { "tropical", "dense", "rain", "impenetrable", "exuberant" },
	/* 95 */ { "funny", "wierd", "unusual", "strange", "peculiar" },
	/* 96 */ { "frequent", "occasional", "unpredictable", "dreadful", "deadly" },
	/* 97 */ { "\x82 \x81 for \x8A", "\x82 \x81 for \x8A and \x8A", "\x88 by \x89", "\x82 \x81 for \x8A but \x88 by \x89", "a\x90 \x91" },
	/* 98 */ { "\x9B", "mountain", "edible", "tree", "spotted" },
	/* 99 */ { "\x9F", "\xA0", "\x87oid", "\x93", "\x92" },
	/* 9A */ { "ancient", "exceptional", "eccentric", "ingrained", "\x95" },
	/* 9B */ { "killer", "deadly", "evil", "lethal", "vicious" },
	/* 9C */ { "parking meters", "dust clouds", "ice bergs", "rock formations", "volcanoes" },
	/* 9D */ { "plant", "tulip", "banana", "corn", "\xB2weed" },
	/* 9E */ { "\xB2", "\xB1 \xB2", "\xB1 \x9B", "inhabitant", "\xB1 \xB2" },
	/* 9F */ { "shrew", "beast", "bison", "snake", "wolf" },
	/* A0 */ { "leopard", "cat", "monkey", "goat", "fish" },
	/* A1 */ { "\x8C \x8B", "\xB1 \x9F \xA2", "its \x8D \xA0 \xA2", "\xA3 \xA4", "\x8C \x8B" },
	/* A2 */ { "meat", "cutlet", "steak", "burgers", "soup" },
	/* A3 */ { "ice", "mud", "Zero-G", "vacuum", "\xB1 ultra" },
	/* A4 */ { "hockey", "cricket", "karate", "polo", "tennis" },
};

// The pairs of letters the made-up names are made of
static const char _name_pairs[] = "ABOUSEITILETSTONLONUTHNOALLEXEGEZACEBISOUSESARMAINDIREA.ERATENBERALAVETIEDORQUANTEISRION";

typedef struct {
	uint8_t a, b, c, d;
	char* out;
	size_t n, len;
} Soup_t;

static uint32_t soup_random(Soup_t* r) {
	uint32_t x = (r->a * 2) & 0xff;
	uint32_t a = x + r->c;
	if (r->a > 127) ++a;
	r->a = a & 0xff;
	r->c = (uint8_t)x;
	a >>= 8;
	x = r->b;
	a = (a + x + r->d) & 0xff;
	r->b = (uint8_t)a;
	r->d = (uint8_t)x;
	return a;
}

static void soup_char(Soup_t* r, char c) {
	if (r->len + 1 < r->n) {
		r->out[r->len++] = c;
		r->out[r->len] = 0;
	}
}

static char lower(char c) {
	return c >= 'A' && c <= 'Z' ? (char)(c - 'A' + 'a') : c;
}

static void soup(Soup_t* r, const GalaxySystem_t* sys, const char* source) {
	for (; *source; ++source) {
		const uint8_t c = (uint8_t)*source;
		if (c < 0x80) {
			soup_char(r, (char)c);
		}
		else if (c <= 0xA4) {
			const uint32_t k = soup_random(r);
			soup(r, sys, _choices[c - 0x81][(k >= 0x33) + (k >= 0x66) + (k >= 0x99) + (k >= 0xCC)]);
		}
		else if (c == 0xB0 || c == 0xB1) {
			const char* name = sys->name;
			soup_char(r, name[0]);
			for (uint32_t i = 1; name[i]; ++i) {
				// "-ian" takes the place of a last E or I
				if (c == 0xB1 && !name[i + 1] && (name[i] == 'E' || name[i] == 'I')) continue;
				soup_char(r, lower(name[i]));
			}
			if (c == 0xB1) {
				soup_char(r, 'i');
				soup_char(r, 'a');
				soup_char(r, 'n');
			}
		}
		else if (c == 0xB2) {
			const uint32_t len = soup_random(r) & 3;
			for (uint32_t i = 0; i <= len; ++i) {
				const uint32_t x = soup_random(r) & 0x3e;
				soup_char(r, i ? lower(_name_pairs[x]) : _name_pairs[x]);
				soup_char(r, lower(_name_pairs[x + 1]));
			}
		}
	}
}

void galaxy_description(const GalaxySystem_t* sys, char* out, size_t n) {
	if (!n) return;
	out[0] = 0;
	Soup_t r = {
		(uint8_t)(sys->seed[1] & 0xff), (uint8_t)(sys->seed[1] >> 8),
		(uint8_t)(sys->seed[2] & 0xff), (uint8_t)(sys->seed[2] >> 8),
		out, n, 0
	};
	soup(&r, sys, "\x8F is \x97.");
}
