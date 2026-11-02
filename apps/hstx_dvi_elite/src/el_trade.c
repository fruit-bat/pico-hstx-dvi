// Copyright (c) 2024 fruit-bat.

// See el_trade.h. The goods and how their prices and quantities are made
// are as in Text Elite: each has a base price and quantity, moved by the
// system's economy through its gradient, and by a random number on arrival
// through its mask.

#include "el_trade.h"
#include "el_companion.h"
#include "el_draw.h"
#include "el_input.h"
#include "el_screen.h"
#include "game_pallet.h"
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

Commander_t cmdr;

typedef enum {
	UNIT_T = 0,
	UNIT_KG,
	UNIT_G
} Unit_e;

typedef struct {
	uint8_t base_price;
	int8_t gradient;
	uint8_t base_quantity;
	uint8_t mask;
	Unit_e unit;
	const char* name;
} Good_t;

static const Good_t _goods[GOODS] = {
	{ 0x13, -0x02, 0x06, 0x01, UNIT_T,  "Food" },
	{ 0x14, -0x01, 0x0A, 0x03, UNIT_T,  "Textiles" },
	{ 0x41, -0x03, 0x02, 0x07, UNIT_T,  "Radioactives" },
	{ 0x28, -0x05, 0xE2, 0x1F, UNIT_T,  "Slaves" },
	{ 0x53, -0x05, 0xFB, 0x0F, UNIT_T,  "Liquor/Wines" },
	{ 0xC4,  0x08, 0x36, 0x03, UNIT_T,  "Luxuries" },
	{ 0xEB,  0x1D, 0x08, 0x78, UNIT_T,  "Narcotics" },
	{ 0x9A,  0x0E, 0x38, 0x03, UNIT_T,  "Computers" },
	{ 0x75,  0x06, 0x28, 0x07, UNIT_T,  "Machinery" },
	{ 0x4E,  0x01, 0x11, 0x1F, UNIT_T,  "Alloys" },
	{ 0x7C,  0x0D, 0x1D, 0x07, UNIT_T,  "Firearms" },
	{ 0xB0, -0x09, 0xDC, 0x3F, UNIT_T,  "Furs" },
	{ 0x20, -0x01, 0x35, 0x03, UNIT_T,  "Minerals" },
	{ 0x61, -0x01, 0x42, 0x07, UNIT_KG, "Gold" },
	{ 0xAB, -0x02, 0x37, 0x1F, UNIT_KG, "Platinum" },
	{ 0x2D, -0x01, 0xFA, 0x0F, UNIT_G,  "Gem-Stones" },
	{ 0x35,  0x0F, 0xC0, 0x07, UNIT_T,  "Alien Items" },
};

static const char* const _unit_names[] = { "t", "kg", "g" };
#define ALIEN_ITEMS 16

// The market where we are
static uint16_t _price[GOODS]; // in tenths of a credit
static uint8_t _quantity[GOODS];
static uint32_t _good = 0;
// Holding a button down buys or sells again after a wait, then every few frames
#define REPEAT_WAIT 20
#define REPEAT_EVERY 5
static uint32_t _held_frames = 0;

// The equipment, how much, and the tech level it is sold from, as in the original
typedef enum {
	EQUIP_FUEL = 0,
	EQUIP_MISSILE,
	EQUIP_CABIN,
	EQUIP_CARGO_BAY,
	EQUIP_ECM,
	EQUIP_PULSE_LASER,
	EQUIP_BEAM_LASER,
	EQUIP_FUEL_SCOOPS,
	EQUIP_ESCAPE_POD,
	EQUIP_ENERGY_BOMB,
	EQUIP_TARGETING,
	EQUIP_DOCKING,
	EQUIP_GALACTIC,
	EQUIP_MILITARY_LASER,
	EQUIPMENT
} Equip_e;

typedef struct {
	const char* name;
	uint32_t price; // in tenths of a credit; fuel is for each tenth of a light year
	uint32_t tech;
	Laser_e laser;  // which, if it is a laser
} Equip_t;

static const Equip_t _equipment[EQUIPMENT] = {
	{ "Fuel", 2, 1, LASER_NONE },
	{ "Missile", 300, 1, LASER_NONE },
	// Not in the original: room for a companion, from the hold
	{ "Passenger Cabin", 1000, 1, LASER_NONE },
	{ "Large Cargo Bay", 4000, 2, LASER_NONE },
	{ "E.C.M. System", 6000, 3, LASER_NONE },
	{ "Pulse Laser", 4000, 4, LASER_PULSE },
	{ "Beam Laser", 10000, 5, LASER_BEAM },
	{ "Fuel Scoops", 5250, 6, LASER_NONE },
	{ "Escape Pod", 10000, 6, LASER_NONE },
	{ "Energy Bomb", 9000, 7, LASER_NONE },
	// Not in the original
	{ "Targeting Computer", 20000, 8, LASER_NONE },
	{ "Docking Computer", 15000, 9, LASER_NONE },
	{ "Galactic Hyperdrive", 50000, 10, LASER_NONE },
	{ "Military Laser", 60000, 10, LASER_MILITARY },
};

#define CARGO_SMALL 20
#define CARGO_LARGE 35
// What a passenger cabin takes from the hold
#define CABIN_TONNES 5
static uint32_t _item = 0;
// Where a laser bought goes
static Mount_e _mount = MOUNT_FRONT;
static const char* const _mount_names[MOUNTS] = { "Front", "Rear", "Left", "Right" };

// The lists are centred across the screen: the market is 46 letters wide, and the equipment 35
#define MARKET_X ((GAME_SCREEN_W - 46 * 8) / 2)
#define EQUIP_X ((GAME_SCREEN_W - 35 * 8) / 2)
#define LIST_TOP (VIEW_TOP + 10)
#define LIST_LINE 12
// The inventory's list of equipment, beside the cargo
#define INVENTORY_EQUIP_X (MARKET_X + 26 * 8)
#define INVENTORY_EQUIP 14

void trade_commander_start(Commander_t* c) {
	memset(c, 0, sizeof(*c));
	c->cash = 1000;
	c->fuel = GALAXY_FUEL_MAX;
	c->missiles = 3;
	c->hold = CARGO_SMALL;
	c->laser[MOUNT_FRONT] = LASER_PULSE;
	for (uint32_t i = MOUNT_REAR; i < MOUNTS; ++i) c->laser[i] = LASER_NONE;
	story_reset(&c->story);
}

void trade_new_commander(void) {
	trade_commander_start(&cmdr);
	_good = 0;
	_item = 0;
}

void trade_escape(void) {
	cmdr.pod = false;
	memset(cmdr.cargo, 0, sizeof(cmdr.cargo));
	cmdr.fuel = GALAXY_FUEL_MAX;
}

void trade_max_equipment(void) {
	cmdr.fuel = GALAXY_FUEL_MAX;
	cmdr.missiles = MISSILES_MAX;
	cmdr.hold = CARGO_LARGE;
	cmdr.ecm = true;
	cmdr.scoops = true;
	cmdr.docking = true;
	cmdr.pod = true;
	cmdr.bomb = true;
	cmdr.galactic = true;
	cmdr.targeting = true;
	cmdr.cabin = true;
	for (uint32_t i = 0; i < MOUNTS; ++i) cmdr.laser[i] = LASER_MILITARY;
}

void trade_arrive(const GalaxySystem_t* sys) {
	const uint32_t fluct = (uint32_t)rand() & 0xff;
	for (uint32_t i = 0; i < GOODS; ++i) {
		const Good_t* g = &_goods[i];
		const int32_t product = (int32_t)sys->economy * g->gradient;
		const int32_t changing = (int32_t)(fluct & g->mask);
		int32_t q = (g->base_quantity + changing - product) & 0xff;
		if (q & 0x80) q = 0;
		_quantity[i] = (uint8_t)(q & 0x3f);
		const int32_t p = (g->base_price + changing + product) & 0xff;
		_price[i] = (uint16_t)(p * 4);
	}
	_quantity[ALIEN_ITEMS] = 0;
}

// The tonnes the hold takes, less any cabin
static uint32_t hold(void) {
	return cmdr.hold - (cmdr.cabin ? CABIN_TONNES : 0);
}

// Tonnes in the hold; goods sold by the kg or g take no room
static uint32_t cargo_tonnes(void) {
	uint32_t t = 0;
	for (uint32_t i = 0; i < GOODS; ++i) {
		if (_goods[i].unit == UNIT_T) t += cmdr.cargo[i];
	}
	return t;
}

static const char* buy(uint32_t i) {
	if (!_quantity[i]) return "NONE FOR SALE";
	if (cmdr.cash < _price[i]) return "NOT ENOUGH CASH";
	if (_goods[i].unit == UNIT_T && cargo_tonnes() >= hold()) return "HOLD FULL";
	cmdr.cash -= _price[i];
	--_quantity[i];
	++cmdr.cargo[i];
	return NULL;
}

const char* trade_scoop(bool alien) {
	const uint32_t i = alien ? ALIEN_ITEMS : (uint32_t)rand() % GOODS;
	if (_goods[i].unit == UNIT_T && cargo_tonnes() >= hold()) return "HOLD FULL";
	++cmdr.cargo[i];
	// The name, in capitals, as messages are, with how much
	static char name[24];
	uint32_t k = 0;
	for (; _goods[i].name[k] && k < 15; ++k) name[k] = (char)toupper((unsigned char)_goods[i].name[k]);
	snprintf(name + k, sizeof(name) - k, " 1%s", _unit_names[_goods[i].unit]);
	for (; name[k]; ++k) name[k] = (char)toupper((unsigned char)name[k]);
	return name;
}

static const char* sell(uint32_t i) {
	if (!cmdr.cargo[i]) return "NONE IN THE HOLD";
	// Sly knows where to get a little more
	cmdr.cash += _price[i] + (companion() == COMPANION_SLY ? _price[i] / 10u : 0u);
	++_quantity[i];
	--cmdr.cargo[i];
	return NULL;
}

// Whether a button is pressed, or held long enough to go again
static bool repeat(uint8_t pressed, uint8_t held, uint8_t button) {
	if (pressed & button) {
		_held_frames = 0;
		return true;
	}
	if (!(held & button)) return false;
	++_held_frames;
	return _held_frames > REPEAT_WAIT && (_held_frames - REPEAT_WAIT) % REPEAT_EVERY == 0;
}

const char* trade_market_input(uint8_t pressed, uint8_t held) {
	if (repeat(pressed, held, EL_INP_DOWN)) _good = (_good + 1) % GOODS;
	if (repeat(pressed, held, EL_INP_UP)) _good = (_good + GOODS - 1) % GOODS;
	if (repeat(pressed, held, EL_INP_RIGHT)) return buy(_good);
	if (repeat(pressed, held, EL_INP_LEFT)) return sell(_good);
	return NULL;
}

static void cash_line(int32_t x, int32_t y) {
	char s[GAME_TEXT_MAX + 1];
	snprintf(s, sizeof(s), "Cash: %u.%u Cr", (unsigned)(cmdr.cash / 10), (unsigned)(cmdr.cash % 10));
	screen_text(x, y, s, GAME_CYAN, false);
}

void trade_draw_market(void) {
	int32_t y = LIST_TOP;
	screen_text(MARKET_X, y, "PRODUCT", GAME_CYAN, false);
	screen_text(MARKET_X + 22 * 8, y, "PRICE  FOR SALE  IN HOLD", GAME_CYAN, false);
	y += LIST_LINE + 4;
	char s[GAME_TEXT_MAX + 1];
	for (uint32_t i = 0; i < GOODS; ++i) {
		const uint32_t c = i == _good ? GAME_YELLOW : GAME_WHITE;
		if (i == _good) screen_select(MARKET_X, y, 46 * 8);
		snprintf(s, sizeof(s), "%-15s %s", _goods[i].name, _unit_names[_goods[i].unit]);
		screen_text(MARKET_X, y, s, c, false);
		snprintf(s, sizeof(s), "%3u.%u  %8u  %7u", (unsigned)(_price[i] / 10), (unsigned)(_price[i] % 10),
			(unsigned)_quantity[i], (unsigned)cmdr.cargo[i]);
		screen_text(MARKET_X + 22 * 8, y, s, c, false);
		y += LIST_LINE;
	}
	y += 4;
	cash_line(MARKET_X, y);
	snprintf(s, sizeof(s), "Hold: %u of %u t", (unsigned)cargo_tonnes(), (unsigned)hold());
	screen_text(MARKET_X + 30 * 8, y, s, GAME_CYAN, false);
	screen_text_centre(VIEW_BOTTOM - 12, "RIGHT BUYS  LEFT SELLS", GAME_GREY, false);
}

void trade_draw_inventory(void) {
	int32_t y = LIST_TOP;
	char s[GAME_TEXT_MAX + 1];
	snprintf(s, sizeof(s), "Fuel: %u.%u Light Years", (unsigned)(cmdr.fuel / 10), (unsigned)(cmdr.fuel % 10));
	screen_text(MARKET_X, y, s, GAME_CYAN, false);
	y += LIST_LINE;
	cash_line(MARKET_X, y);
	y += LIST_LINE;
	snprintf(s, sizeof(s), "Hold: %u of %u t", (unsigned)cargo_tonnes(), (unsigned)hold());
	screen_text(MARKET_X, y, s, GAME_CYAN, false);
	y += LIST_LINE;
	snprintf(s, sizeof(s), "Missiles: %u of %u", (unsigned)cmdr.missiles, (unsigned)MISSILES_MAX);
	screen_text(MARKET_X, y, s, GAME_CYAN, false);
	y += LIST_LINE + 8;
	screen_text(MARKET_X, y, "CARGO:", GAME_CYAN, false);
	// The equipment fitted, beside the cargo
	screen_text(INVENTORY_EQUIP_X, y, "EQUIPMENT:", GAME_CYAN, false);
	const char* fitted[INVENTORY_EQUIP];
	const uint32_t n = trade_equipment_names(fitted, INVENTORY_EQUIP);
	for (uint32_t i = 0; i < n; ++i) {
		screen_text(INVENTORY_EQUIP_X + 16, y + LIST_LINE + 4 + LIST_LINE * (int32_t)i, fitted[i], GAME_WHITE, false);
	}
	if (!n) screen_text(INVENTORY_EQUIP_X + 16, y + LIST_LINE + 4, "None", GAME_GREY, false);
	y += LIST_LINE + 4;
	bool any = false;
	for (uint32_t i = 0; i < GOODS; ++i) {
		if (!cmdr.cargo[i]) continue;
		snprintf(s, sizeof(s), "%-15s %4u %s", _goods[i].name, (unsigned)cmdr.cargo[i], _unit_names[_goods[i].unit]);
		screen_text(MARKET_X + 16, y, s, GAME_WHITE, false);
		y += LIST_LINE;
		any = true;
	}
	if (!any) screen_text(MARKET_X + 16, y, "Empty", GAME_GREY, false);
	if (!any) y += LIST_LINE;
	// Who and what the stories have put aboard
	const char* items[STORY_ITEMS];
	const uint32_t k = story_item_names(items, STORY_ITEMS);
	if (!k && !companion_name() && !pet_name()) return;
	y += 8;
	screen_text(MARKET_X, y, "ABOARD:", GAME_CYAN, false);
	y += LIST_LINE + 4;
	if (companion_name()) {
		snprintf(s, sizeof(s), "%s (%s)", companion_name(), companion_mood_name());
		screen_text(MARKET_X + 16, y, s, GAME_GREEN, false);
		y += LIST_LINE;
	}
	if (pet_name()) {
		snprintf(s, sizeof(s), "A %s", pet_name());
		screen_text(MARKET_X + 16, y, s, GAME_GREEN, false);
		y += LIST_LINE;
	}
	for (uint32_t i = 0; i < k && y < VIEW_BOTTOM - 12; ++i) {
		screen_text(MARKET_X + 16, y, items[i], GAME_YELLOW, false);
		y += LIST_LINE;
	}
}

// The equipment sold at a system, by its tech level
static uint32_t equipment_sold(const GalaxySystem_t* sys) {
	uint32_t n = 0;
	while (n < EQUIPMENT && _equipment[n].tech <= sys->tech) ++n;
	return n;
}

static const char* fit(Equip_e e) {
	uint32_t price = _equipment[e].price;
	if (e == EQUIP_FUEL) price *= GALAXY_FUEL_MAX - cmdr.fuel;
	switch (e) {
		case EQUIP_FUEL:
			if (cmdr.fuel >= GALAXY_FUEL_MAX) return "FULL";
			break;
		case EQUIP_MISSILE:
			if (cmdr.missiles >= MISSILES_MAX) return "FULL";
			break;
		case EQUIP_CABIN:
			if (cmdr.cabin) return "FITTED";
			break;
		case EQUIP_CARGO_BAY:
			if (cmdr.hold >= CARGO_LARGE) return "FITTED";
			break;
		case EQUIP_ECM:
			if (cmdr.ecm) return "FITTED";
			break;
		case EQUIP_FUEL_SCOOPS:
			if (cmdr.scoops) return "FITTED";
			break;
		case EQUIP_ESCAPE_POD:
			if (cmdr.pod) return "FITTED";
			break;
		case EQUIP_ENERGY_BOMB:
			if (cmdr.bomb) return "FITTED";
			break;
		case EQUIP_DOCKING:
			if (cmdr.docking) return "FITTED";
			break;
		case EQUIP_TARGETING:
			if (cmdr.targeting) return "FITTED";
			break;
		case EQUIP_GALACTIC:
			if (cmdr.galactic) return "FITTED";
			break;
		default: {
			// Only a better laser than the one there, if any
			const Laser_e has = cmdr.laser[_mount];
			if (has != LASER_NONE && has >= _equipment[e].laser) return "FITTED";
			break;
		}
	}
	if (cmdr.cash < price) return "NOT ENOUGH CASH";
	cmdr.cash -= price;
	switch (e) {
		case EQUIP_FUEL: cmdr.fuel = GALAXY_FUEL_MAX; break;
		case EQUIP_MISSILE: ++cmdr.missiles; break;
		case EQUIP_CABIN: cmdr.cabin = true; break;
		case EQUIP_CARGO_BAY: cmdr.hold = CARGO_LARGE; break;
		case EQUIP_ECM: cmdr.ecm = true; break;
		case EQUIP_FUEL_SCOOPS: cmdr.scoops = true; break;
		case EQUIP_ESCAPE_POD: cmdr.pod = true; break;
		case EQUIP_ENERGY_BOMB: cmdr.bomb = true; break;
		case EQUIP_DOCKING: cmdr.docking = true; break;
		case EQUIP_TARGETING: cmdr.targeting = true; break;
		case EQUIP_GALACTIC: cmdr.galactic = true; break;
		default: cmdr.laser[_mount] = _equipment[e].laser; break;
	}
	return NULL;
}

const char* trade_equipment_input(uint8_t pressed, const GalaxySystem_t* sys) {
	const uint32_t n = equipment_sold(sys);
	if (_item >= n) _item = 0;
	if (pressed & EL_INP_DOWN) _item = (_item + 1) % n;
	if (pressed & EL_INP_UP) _item = (_item + n - 1) % n;
	// Left and right choose where a laser goes
	if (_equipment[_item].laser != LASER_NONE) {
		if (pressed & EL_INP_RIGHT) _mount = (Mount_e)((_mount + 1) % MOUNTS);
		if (pressed & EL_INP_LEFT) _mount = (Mount_e)((_mount + MOUNTS - 1) % MOUNTS);
	}
	if (pressed & EL_INP_A) return fit((Equip_e)_item);
	return NULL;
}

void trade_draw_equipment(const GalaxySystem_t* sys) {
	const uint32_t n = equipment_sold(sys);
	int32_t y = LIST_TOP;
	screen_text(EQUIP_X, y, "EQUIPMENT", GAME_CYAN, false);
	screen_text(EQUIP_X + 30 * 8, y, "PRICE", GAME_CYAN, false);
	y += LIST_LINE + 4;
	char s[GAME_TEXT_MAX + 1];
	for (uint32_t i = 0; i < n; ++i) {
		const uint32_t c = i == _item ? GAME_YELLOW : GAME_WHITE;
		if (i == _item) screen_select(EQUIP_X, y, 35 * 8);
		screen_text(EQUIP_X, y, _equipment[i].name, c, false);
		if (i == _item && _equipment[i].laser != LASER_NONE) {
			snprintf(s, sizeof(s), "<%s>", _mount_names[_mount]);
			screen_text(EQUIP_X + 16 * 8, y, s, c, false);
		}
		uint32_t price = _equipment[i].price;
		if (i == EQUIP_FUEL) price *= GALAXY_FUEL_MAX - cmdr.fuel;
		snprintf(s, sizeof(s), "%5u.%u", (unsigned)(price / 10), (unsigned)(price % 10));
		screen_text(EQUIP_X + 28 * 8, y, s, c, false);
		y += LIST_LINE + 3;
	}
	y += 8;
	cash_line(EQUIP_X, y);
	screen_text_centre(VIEW_BOTTOM - 12, _equipment[_item].laser != LASER_NONE ? "FIRE BUYS  LEFT AND RIGHT CHOOSE WHERE" : "FIRE BUYS", GAME_GREY, false);
}

uint32_t trade_equipment_names(const char** names, uint32_t max) {
	static const char* const lasers[MOUNTS][LASER_NONE] = {
		{ "Front Pulse Laser", "Front Beam Laser", "Front Military Laser" },
		{ "Rear Pulse Laser", "Rear Beam Laser", "Rear Military Laser" },
		{ "Left Pulse Laser", "Left Beam Laser", "Left Military Laser" },
		{ "Right Pulse Laser", "Right Beam Laser", "Right Military Laser" },
	};
	uint32_t n = 0;
	for (uint32_t i = 0; i < MOUNTS; ++i) {
		if (n < max && cmdr.laser[i] != LASER_NONE) names[n++] = lasers[i][cmdr.laser[i]];
	}
	if (n < max && cmdr.hold >= CARGO_LARGE) names[n++] = "Large Cargo Bay";
	if (n < max && cmdr.cabin) names[n++] = "Passenger Cabin";
	if (n < max && cmdr.ecm) names[n++] = "E.C.M. System";
	if (n < max && cmdr.scoops) names[n++] = "Fuel Scoops";
	if (n < max && cmdr.pod) names[n++] = "Escape Pod";
	if (n < max && cmdr.bomb) names[n++] = "Energy Bomb";
	if (n < max && cmdr.targeting) names[n++] = "Targeting Computer";
	if (n < max && cmdr.docking) names[n++] = "Docking Computer";
	if (n < max && cmdr.galactic) names[n++] = "Galactic Hyperdrive";
	return n;
}
