// Copyright (c) 2024 fruit-bat.

// The sounds and tunes, as steps for the sequencer (game_seq.h), at 120
// ticks a second, two to a frame. Sounds bend two octaves either way, so a
// bend's step is three eighths of a semitone, 64 being none.

#include "rr_sound.h"
#include "game_seq_steps.h"
#include "game_songs.h"

// ----------------------------------------------------------------------------
// Sounds

// A quick pew, down, soft as there are a lot of them
static const SeqStep_t _shot[] = {
	PROG(GM_SQUARE_LEAD), BEND(96), ON(0, 88, 55), SWEEP(0, 48, 6), OFF(6, 88), END(1)
};
// Longer and brighter, falling further
static const SeqStep_t _laser[] = {
	PROG(GM_SAW_LEAD), BEND(124), ON(0, 91, 60), SWEEP(0, 24, 12), OFF(12, 91), END(1)
};
// The charged beam: a roar falling two octaves
static const SeqStep_t _beam[] = {
	PROG(GM_SAW_LEAD), BEND(127), ON(0, 72, 127), ON(0, 60, 127), ON(0, 48, 120), SWEEP(0, 0, 50),
	OFF(50, 72), OFF(0, 60), OFF(0, 48), END(4)
};
// A whistle, falling
static const SeqStep_t _bomb[] = {
	PROG(GM_WHISTLE), BEND(100), ON(0, 84, 60), SWEEP(0, 24, 40), OFF(40, 84), END(2)
};
static const SeqStep_t _blast[] = {
	PROG(GM_GUNSHOT), BEND(70), ON(0, 38, 120), ON(0, 31, 120), SWEEP(0, 40, 24), OFF(24, 38), OFF(0, 31), END(4)
};
// A clank off armour
static const SeqStep_t _hit[] = {
	PROG(GM_AGOGO), ON(0, 72, 90), OFF(6, 72), END(1)
};
static const SeqStep_t _explode[] = {
	PROG(GM_GUNSHOT), BEND(72), ON(0, 40, 127), ON(0, 36, 127), SWEEP(0, 40, 40), ON(4, 33, 127),
	OFF(40, 40), OFF(0, 36), OFF(0, 33), END(6)
};
static const SeqStep_t _explode_big[] = {
	PROG(GM_GUNSHOT), BEND(72), ON(0, 36, 127), ON(0, 31, 127), SWEEP(0, 16, 90), ON(6, 28, 127), ON(10, 24, 127),
	OFF(90, 36), OFF(0, 31), OFF(0, 28), OFF(0, 24), END(10)
};
// A rush, rising
static const SeqStep_t _launch[] = {
	PROG(GM_SAW_LEAD), BEND(16), ON(0, 43, 70), ON(0, 50, 60), SWEEP(0, 112, 50), OFF(50, 43), OFF(0, 50), END(2)
};
static const SeqStep_t _enemy_fire[] = {
	PROG(GM_SQUARE_LEAD), BEND(64), ON(0, 76, 55), SWEEP(0, 88, 6), OFF(6, 76), END(1)
};
static const SeqStep_t _capsule[] = {
	PROG(GM_VIBRAPHONE), ON(0, 79, 110), OFF(6, 79), ON(0, 86, 110), OFF(14, 86), END(6)
};
// Up the scale, quickly
static const SeqStep_t _power[] = {
	PROG(GM_SQUARE_LEAD), ON(0, 72, 100), OFF(4, 72), ON(0, 76, 100), OFF(4, 76), ON(0, 79, 100), OFF(4, 79),
	ON(0, 84, 100), OFF(4, 84), ON(0, 88, 100), OFF(4, 88), ON(0, 91, 110), OFF(16, 91), END(4)
};
// Bubbling, filling up
static const SeqStep_t _fuel[] = {
	PROG(GM_MARIMBA), ON(0, 60, 110), OFF(5, 60), ON(0, 64, 110), OFF(5, 64), ON(0, 67, 110), OFF(5, 67),
	ON(0, 72, 110), OFF(5, 72), ON(0, 76, 110), OFF(12, 76), END(6)
};
static const SeqStep_t _fuel_low[] = {
	PROG(GM_SQUARE_LEAD), BEND(64), ON(0, 88, 80), OFF(8, 88), ON(6, 88, 80), OFF(8, 88), END(2)
};
static const SeqStep_t _dead[] = {
	PROG(GM_GUNSHOT), BEND(64), ON(0, 30, 127), SWEEP(0, 0, 150), ON(10, 26, 127), ON(16, 22, 127),
	OFF(140, 30), OFF(0, 26), OFF(0, 22), END(20)
};
// Up as the ship comes back
static const SeqStep_t _respawn[] = {
	PROG(GM_SQUARE_LEAD), BEND(16), ON(0, 48, 70), ON(0, 55, 70), SWEEP(0, 64, 60),
	OFF(60, 48), OFF(0, 55), END(2)
};
static const SeqStep_t _extra_life[] = {
	PROG(GM_SQUARE_LEAD),
	ON(0, 74, 100), OFF(6, 74), ON(2, 78, 100), OFF(6, 78), ON(2, 81, 100), OFF(6, 81),
	ON(2, 86, 100), OFF(6, 86), ON(2, 81, 100), OFF(6, 81), ON(2, 86, 110), OFF(30, 86),
	END(4)
};
// A siren, three times
static const SeqStep_t _alarm[] = {
	PROG(GM_SQUARE_LEAD), BEND(40), ON(0, 72, 100),
	SWEEP(0, 88, 30), SWEEP(30, 40, 30), SWEEP(30, 88, 30), SWEEP(30, 40, 30), SWEEP(30, 88, 30), SWEEP(30, 40, 30),
	OFF(30, 72), END(4)
};
static const SeqStep_t _boss_hit[] = {
	PROG(GM_SYNTH_DRUM), ON(0, 45, 110), OFF(8, 45), END(1)
};
// A long rumble, falling
static const SeqStep_t _boss_die[] = {
	PROG(GM_GUNSHOT), BEND(64), ON(0, 28, 127), SWEEP(0, 0, 240), ON(20, 24, 127), ON(30, 20, 127),
	REST(200), OFF(0, 28), OFF(0, 24), OFF(0, 20), END(30)
};
static const SeqStep_t _tally[] = {
	PROG(GM_GLOCKENSPIEL), ON(0, 96, 80), OFF(6, 96), END(2)
};
// Held: a whine, bent up as the beam charges
static const SeqStep_t _charge[] = {
	PROG(GM_SAW_LEAD), BEND(0), CC(11, 90), ON(0, 72, 70), ON(0, 79, 50), MARK, LOOP(240)
};

const SeqSound_t rr_sound_data[RR_SFX_SOUNDS] = {
	[RR_SFX_SHOT] = { _shot, 2, SEQ_LEVEL_NONE },
	[RR_SFX_LASER] = { _laser, 2, SEQ_LEVEL_NONE },
	[RR_SFX_BEAM] = { _beam, 7, SEQ_LEVEL_NONE },
	[RR_SFX_BOMB] = { _bomb, 3, SEQ_LEVEL_NONE },
	[RR_SFX_BLAST] = { _blast, 4, SEQ_LEVEL_NONE },
	[RR_SFX_HIT] = { _hit, 3, SEQ_LEVEL_NONE },
	[RR_SFX_EXPLODE] = { _explode, 5, SEQ_LEVEL_NONE },
	[RR_SFX_EXPLODE_BIG] = { _explode_big, 7, SEQ_LEVEL_NONE },
	[RR_SFX_LAUNCH] = { _launch, 4, SEQ_LEVEL_NONE },
	[RR_SFX_ENEMY_FIRE] = { _enemy_fire, 2, SEQ_LEVEL_NONE },
	[RR_SFX_CAPSULE] = { _capsule, 7, SEQ_LEVEL_NONE },
	[RR_SFX_POWER] = { _power, 8, SEQ_LEVEL_NONE },
	[RR_SFX_FUEL] = { _fuel, 7, SEQ_LEVEL_NONE },
	[RR_SFX_FUEL_LOW] = { _fuel_low, 8, SEQ_LEVEL_NONE },
	[RR_SFX_DEAD] = { _dead, 10, SEQ_LEVEL_NONE },
	[RR_SFX_RESPAWN] = { _respawn, 7, SEQ_LEVEL_NONE },
	[RR_SFX_EXTRA_LIFE] = { _extra_life, 9, SEQ_LEVEL_NONE },
	[RR_SFX_ALARM] = { _alarm, 9, SEQ_LEVEL_NONE },
	[RR_SFX_BOSS_HIT] = { _boss_hit, 4, SEQ_LEVEL_NONE },
	[RR_SFX_BOSS_DIE] = { _boss_die, 10, SEQ_LEVEL_NONE },
	[RR_SFX_TALLY] = { _tally, 5, SEQ_LEVEL_NONE },
	[RR_SFX_CHARGE] = { _charge, 6, SEQ_LEVEL_BEND },
};

// ----------------------------------------------------------------------------
// Tunes: our own, synth rock, at 150 beats a minute, a beat Q ticks

#define Q 48
#define E8 (Q / 2)
#define S16 (Q / 4)
#define BAR (4 * Q)

// A note e eighths long
#define L(n, e) NOTE(n, (e) * E8, 100)
// A chord held a bar, let go a little before the next
#define PAD(a, b, c) CHORD(a, b, c, BAR - 8, 70), REST(8)
// A bass in octaves, a bar of eighths
#define OCT(r) NOTE(r, E8, 105), NOTE((r) + 12, E8, 80), NOTE(r, E8, 105), NOTE((r) + 12, E8, 80), \
	NOTE(r, E8, 105), NOTE((r) + 12, E8, 80), NOTE(r, E8, 105), NOTE((r) + 12, E8, 80)
// Driving: the root on every eighth, pushing into the next bar
#define DRIVE(r) NOTE(r, E8, 110), NOTE(r, E8, 80), NOTE(r, E8, 95), NOTE(r, E8, 80), \
	NOTE(r, E8, 110), NOTE(r, E8, 80), NOTE((r) + 12, E8, 95), NOTE((r) + 10, E8, 90)

// Drums, an eighth each: one or two hits, or the hi-hat alone
#define KICK GM_DRUM_BASS
#define SNARE GM_DRUM_SNARE
#define HAT GM_DRUM_CLOSED_HI_HAT
#define HIT(a) ON(0, a, 110), ON(0, HAT, 70), OFF(4, a), OFF(0, HAT), REST(E8 - 4)
#define HH ON(0, HAT, 60), OFF(4, HAT), REST(E8 - 4)
#define BEAT HIT(KICK), HH, HIT(SNARE), HH, HIT(KICK), HIT(KICK), HIT(SNARE), HH
#define ROCK HIT(KICK), HIT(KICK), HIT(SNARE), HH, HIT(KICK), HIT(KICK), HIT(SNARE), HIT(SNARE)
#define LIGHT HIT(KICK), HH, HH, HH, HIT(KICK), HH, HH, HH
#define FILL HIT(SNARE), HIT(SNARE), HIT(SNARE), HIT(SNARE), HIT(GM_DRUM_HIGH_TOM), HIT(GM_DRUM_HIGH_TOM), \
	HIT(GM_DRUM_LOW_TOM), HIT(GM_DRUM_LOW_TOM)

// The notes
#define B1 35
#define C2 36
#define D2 38
#define E2 40
#define F2 41
#define FS2 42
#define G2 43
#define A2 45
#define BB2 46
#define A4 69
#define BB4 70
#define B4 71
#define CS5 73
#define D5 74
#define DS5 75
#define E5 76
#define F5 77
#define FS5 78
#define G5 79
#define GB5 78
#define A5 81
#define BB5 82
#define B5 83
#define C6 84
#define D6 86

// The title, heroic, in E minor: E minor, C, G, D, E minor, C, D, B
static const SeqStep_t _title_lead[] = {
	PROG(GM_SYNTH_BRASS_1), CC(CC_EXPRESSION, 105),
	L(E5, 4), L(B4, 2), L(E5, 2),
	L(G5, 6), L(E5, 2),
	L(D5, 4), L(B4, 2), L(D5, 2),
	L(FS5, 6), L(D5, 2),
	L(E5, 2), L(G5, 2), L(B5, 4),
	L(C6, 4), L(B5, 2), L(G5, 2),
	L(A5, 4), L(FS5, 2), L(D5, 2),
	L(DS5, 4), L(FS5, 2), L(B5, 2),
	END(0)
};
static const SeqStep_t _title_pad[] = {
	PROG(GM_POLYSYNTH_PAD), CC(CC_EXPRESSION, 80),
	PAD(52, 55, 59), PAD(48, 52, 55), PAD(50, 55, 59), PAD(50, 54, 57),
	PAD(52, 55, 59), PAD(48, 52, 55), PAD(50, 54, 57), PAD(51, 54, 59),
	END(0)
};
static const SeqStep_t _title_bass[] = {
	PROG(GM_SYNTH_BASS_1), CC(CC_EXPRESSION, 100),
	OCT(E2), OCT(C2), OCT(G2), OCT(D2), OCT(E2), OCT(C2), OCT(D2), OCT(B1),
	END(0)
};
static const SeqStep_t _title_drums[] = {
	BEAT, BEAT, BEAT, BEAT, BEAT, BEAT, BEAT, FILL,
	END(0)
};
static const SeqTrack_t _title[] = {
	{ 0, _title_lead },
	{ 1, _title_pad },
	{ 2, _title_bass },
	{ GM_DRUM_CHANNEL, _title_drums },
};

// Flying, driving, in D minor: D minor, B flat, C, D minor, then again to A
static const SeqStep_t _stage_lead[] = {
	PROG(GM_SAW_LEAD), CC(CC_EXPRESSION, 90),
	L(D5, 2), L(F5, 1), L(A5, 2), L(G5, 1), L(F5, 1), L(E5, 1),
	L(D5, 3), L(BB4, 1), L(D5, 2), L(F5, 2),
	L(E5, 2), L(G5, 1), L(C6, 2), L(BB5, 1), L(A5, 1), L(G5, 1),
	L(A5, 4), L(F5, 2), L(D5, 2),
	L(D5, 1), L(D5, 1), L(F5, 1), L(A5, 1), L(D6, 2), L(C6, 1), L(A5, 1),
	L(BB5, 2), L(A5, 1), L(G5, 1), L(F5, 2), L(D5, 2),
	L(E5, 1), L(F5, 1), L(G5, 1), L(A5, 1), L(BB5, 1), L(C6, 1), L(BB5, 1), L(G5, 1),
	L(A5, 2), L(E5, 1), L(CS5, 1), L(E5, 2), L(A4, 2),
	END(0)
};
static const SeqStep_t _stage_pad[] = {
	PROG(GM_SYNTH_STRINGS_1), CC(CC_EXPRESSION, 70),
	PAD(50, 53, 57), PAD(46, 50, 53), PAD(48, 52, 55), PAD(50, 53, 57),
	PAD(50, 53, 57), PAD(46, 50, 53), PAD(48, 52, 55), PAD(45, 49, 52),
	END(0)
};
static const SeqStep_t _stage_bass[] = {
	PROG(GM_SYNTH_BASS_2), CC(CC_EXPRESSION, 105),
	DRIVE(D2), DRIVE(BB2 - 12), DRIVE(C2), DRIVE(D2),
	DRIVE(D2), DRIVE(BB2 - 12), DRIVE(C2), DRIVE(A2 - 12),
	END(0)
};
static const SeqStep_t _stage_drums[] = {
	ROCK, BEAT, ROCK, BEAT, ROCK, BEAT, ROCK, FILL,
	END(0)
};
static const SeqTrack_t _stage[] = {
	{ 0, _stage_lead },
	{ 1, _stage_pad },
	{ 2, _stage_bass },
	{ GM_DRUM_CHANNEL, _stage_drums },
};

// The warship: E, a semitone up and back, quicker, strings stabbing
#define BQ 40
#define BE (BQ / 2)
#define BL(n, e) NOTE(n, (e) * BE, 100)
#define BB(r, s, t) NOTE(r, BE, 115), NOTE(r, BE, 85), NOTE(s, BE, 105), NOTE(r, BE, 85), \
	NOTE(r, BE, 115), NOTE(t, BE, 100), NOTE(s, BE, 100), NOTE(r, BE, 90)
#define STAB(a, b, c) CHORD(a, b, c, BE, 105), REST(BE), REST(BE), CHORD(a, b, c, BE, 95), REST(BE), REST(BE), \
	CHORD(a, b, c, BE, 105), REST(BE)
#define BHIT(a) ON(0, a, 115), ON(0, HAT, 70), OFF(4, a), OFF(0, HAT), REST(BE - 4)
#define BHH ON(0, HAT, 60), OFF(4, HAT), REST(BE - 4)
#define BBEAT BHIT(KICK), BHH, BHIT(SNARE), BHIT(KICK), BHIT(KICK), BHH, BHIT(SNARE), BHH
static const SeqStep_t _boss_lead[] = {
	PROG(GM_SAW_LEAD), CC(CC_EXPRESSION, 85),
	BL(E5, 6), BL(F5, 2),
	BL(E5, 4), BL(G5, 2), BL(F5, 2),
	BL(F5, 6), BL(GB5, 2),
	BL(E5, 8),
	END(0)
};
static const SeqStep_t _boss_stabs[] = {
	PROG(GM_STRING_ENSEMBLE_1), CC(CC_EXPRESSION, 95),
	STAB(52, 55, 58), STAB(52, 55, 58), STAB(53, 56, 59), STAB(52, 55, 58),
	END(0)
};
static const SeqStep_t _boss_bass[] = {
	PROG(GM_SYNTH_BASS_1), CC(CC_EXPRESSION, 110),
	BB(E2, F2, G2), BB(E2, F2, G2), BB(F2, FS2, A2 - 1), BB(E2, F2, G2),
	END(0)
};
static const SeqStep_t _boss_drums[] = {
	BBEAT, BBEAT, BBEAT, BBEAT,
	END(0)
};
static const SeqTrack_t _boss[] = {
	{ 0, _boss_lead },
	{ 1, _boss_stabs },
	{ 2, _boss_bass },
	{ GM_DRUM_CHANNEL, _boss_drums },
};

// The warship beaten: a fanfare, up to D major, held
static const SeqStep_t _clear_lead[] = {
	PROG(GM_SYNTH_BRASS_1), CC(CC_EXPRESSION, 110),
	NOTE(D5, S16, 110), NOTE(D5, S16, 110), NOTE(D5, S16, 110), NOTE(A5, 3 * S16, 110),
	NOTE(G5, S16, 110), NOTE(A5, S16, 110), NOTE(FS5, 2 * S16, 110), NOTE(A5, 2 * S16, 110),
	NOTE(D6, 12 * S16, 120),
	END(0)
};
static const SeqStep_t _clear_chord[] = {
	PROG(GM_SYNTH_BRASS_1), CC(CC_EXPRESSION, 90),
	REST(12 * S16), CHORD(62, 66, 69, 12 * S16, 95),
	END(0)
};
static const SeqStep_t _clear_bass[] = {
	PROG(GM_SYNTH_BASS_1), CC(CC_EXPRESSION, 100),
	NOTE(A2, 4 * S16, 100), NOTE(A2 + 12, 4 * S16, 100), NOTE(A2, 4 * S16, 100), NOTE(D2, 12 * S16, 115),
	END(0)
};
static const SeqStep_t _clear_drums[] = {
	ON(0, SNARE, 90), OFF(4, SNARE), ON(S16 - 4, SNARE, 100), OFF(4, SNARE), ON(S16 - 4, SNARE, 110), OFF(4, SNARE),
	ON(S16 - 4, KICK, 110), OFF(4, KICK), ON(3 * S16 - 4, KICK, 110), OFF(4, KICK),
	ON(4 * S16 - 4, KICK, 110), OFF(4, KICK), ON(4 * S16 - 4, KICK, 110), OFF(4, KICK),
	ON(2 * S16 - 4, KICK, 120), ON(0, GM_DRUM_CRASH, 110), OFF(4, KICK), OFF(12 * S16 - 4, GM_DRUM_CRASH),
	END(0)
};
static const SeqTrack_t _clear[] = {
	{ 0, _clear_lead },
	{ 1, _clear_chord },
	{ 2, _clear_bass },
	{ GM_DRUM_CHANNEL, _clear_drums },
};

// Game over: slower, down through D minor, C, B flat and A to D minor
#define GQ 60
static const SeqStep_t _over_lead[] = {
	PROG(GM_SYNTH_BRASS_1), CC(CC_EXPRESSION, 100),
	NOTE(A5, GQ, 100), NOTE(G5, GQ, 100), NOTE(F5, GQ, 100), NOTE(E5, GQ, 100), NOTE(D5, 3 * GQ, 100),
	END(0)
};
static const SeqStep_t _over_pad[] = {
	PROG(GM_POLYSYNTH_PAD), CC(CC_EXPRESSION, 80),
	CHORD(50, 53, 57, GQ, 70), CHORD(48, 52, 55, GQ, 70), CHORD(46, 50, 53, GQ, 70),
	CHORD(45, 49, 52, GQ, 70), CHORD(50, 53, 57, 3 * GQ, 70),
	END(0)
};
static const SeqStep_t _over_bass[] = {
	PROG(GM_SYNTH_BASS_1), CC(CC_EXPRESSION, 100),
	NOTE(D2 + 12, GQ, 100), NOTE(C2 + 12, GQ, 100), NOTE(BB2, GQ, 100), NOTE(A2, GQ, 100), NOTE(D2, 3 * GQ, 110),
	END(0)
};
static const SeqTrack_t _over[] = {
	{ 0, _over_lead },
	{ 1, _over_pad },
	{ 2, _over_bass },
};

// A new high score: a square arpeggio in sixteenths over D, B minor, G and A
#define A16(n) ON(0, n, 70), OFF(S16 - 2, n), REST(2)
#define ARP(a, b, c, d) A16(a), A16(b), A16(c), A16(d), A16(c), A16(b), A16(a), A16(b), \
	A16(a), A16(b), A16(c), A16(d), A16(c), A16(b), A16(a), A16(b)
static const SeqStep_t _hiscore_arp[] = {
	PROG(GM_SQUARE_LEAD), CC(CC_EXPRESSION, 80),
	ARP(62, 66, 69, 74), ARP(59, 62, 66, 71), ARP(55, 59, 62, 67), ARP(57, 61, 64, 69),
	END(0)
};
static const SeqStep_t _hiscore_bass[] = {
	PROG(GM_SYNTH_BASS_1), CC(CC_EXPRESSION, 90),
	OCT(D2), OCT(B1), OCT(G2 - 12), OCT(A2 - 12),
	END(0)
};
static const SeqStep_t _hiscore_drums[] = {
	LIGHT, LIGHT, LIGHT, LIGHT,
	END(0)
};
static const SeqTrack_t _hiscore[] = {
	{ 0, _hiscore_arp },
	{ 2, _hiscore_bass },
	{ GM_DRUM_CHANNEL, _hiscore_drums },
};

const SeqTune_t rr_tune_data[RR_TUNES] = {
	[RR_TUNE_TITLE] = { _title, 4, true },
	[RR_TUNE_STAGE] = { _stage, 4, true },
	[RR_TUNE_BOSS] = { _boss, 4, true },
	[RR_TUNE_CLEAR] = { _clear, 4, false },
	[RR_TUNE_GAME_OVER] = { _over, 3, false },
	[RR_TUNE_HISCORE] = { _hiscore, 3, true },
	[RR_TUNE_NIGHT_ORBIT] = GAME_SONG_NIGHT_ORBIT,
	[RR_TUNE_ATOMIC_SURF] = GAME_SONG_ATOMIC_SURF,
	[RR_TUNE_CLOCKWORK] = GAME_SONG_CLOCKWORK_INVENTION,
};
