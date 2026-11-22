// Copyright (c) 2024 fruit-bat.

// The sounds and tunes, as steps for the sequencer (game_seq.h), at 120
// ticks a second, two to a frame. Sounds bend two octaves either way, so a
// bend's step is three eighths of a semitone, 64 being none.

#include "inv_sound.h"
#include "game_seq_steps.h"

// ----------------------------------------------------------------------------
// Sounds: an old flying saucer film, square waves, a wobbly whistle and bangs

// A pew, down, short enough not to run into the next shot
static const SeqStep_t _shoot[] = {
	PROG(GM_SQUARE_LEAD), BEND(104), ON(0, 79, 60), SWEEP(0, 40, 10), OFF(10, 79), END(1)
};
// A pop, falling
static const SeqStep_t _hit[] = {
	PROG(GM_GUNSHOT), BEND(80), ON(0, 52, 120), ON(0, 45, 110), SWEEP(0, 40, 24),
	OFF(24, 52), OFF(0, 45), END(4)
};
// A clank off the commander's armour
static const SeqStep_t _armour[] = {
	PROG(GM_AGOGO), PLAY(0, 72, 10, 120), PLAY(0, 67, 14, 110), END(4)
};
// The formation's footsteps, down through D minor
#define MARCH(n) PROG(GM_SYNTH_BASS_2), PLAY(0, n, 12, 120), END(2)
static const SeqStep_t _march_1[] = { MARCH(38) };
static const SeqStep_t _march_2[] = { MARCH(36) };
static const SeqStep_t _march_3[] = { MARCH(34) };
static const SeqStep_t _march_4[] = { MARCH(33) };
// Neeeoow, as it peels off
static const SeqStep_t _dive[] = {
	PROG(GM_SAW_LEAD), BEND(100), ON(0, 72, 60), SWEEP(0, 24, 70), OFF(70, 72), END(2)
};
// A crumble, quiet as it happens a lot
static const SeqStep_t _base[] = {
	PROG(GM_GUNSHOT), BEND(64), ON(0, 76, 45), OFF(8, 76), END(2)
};
// A bang, then a sparkle up
static const SeqStep_t _mystery_hit[] = {
	PROG(GM_GUNSHOT), BEND(72), ON(0, 45, 127), ON(0, 38, 127), SWEEP(0, 48, 30), OFF(30, 45), OFF(0, 38),
	PROG(GM_VIBRAPHONE), BEND(64),
	PLAY(0, 84, 6, 110), PLAY(0, 88, 6, 110), PLAY(0, 91, 6, 110), PLAY(0, 96, 30, 120), END(6)
};
// A big one, rumbling down, as long as the gun burns
static const SeqStep_t _dead[] = {
	PROG(GM_GUNSHOT), BEND(64), ON(0, 36, 127), SWEEP(0, 16, 90), ON(8, 31, 127), ON(12, 24, 127),
	OFF(90, 36), OFF(0, 31), OFF(0, 24), END(10)
};
// An alarm, then down and out
static const SeqStep_t _landed[] = {
	PROG(GM_SAW_LEAD), BEND(64),
	PLAY(0, 64, 14, 110), PLAY(2, 58, 14, 110), PLAY(2, 64, 14, 110), PLAY(2, 58, 14, 110),
	PLAY(2, 64, 14, 110), PLAY(2, 58, 14, 110),
	ON(2, 64, 110), SWEEP(0, 0, 80), OFF(80, 64), END(6)
};
static const SeqStep_t _extra_life[] = {
	PROG(GM_CELESTA),
	PLAY(0, 74, 6, 100), PLAY(2, 78, 6, 100), PLAY(2, 81, 6, 100), PLAY(2, 86, 6, 100),
	PLAY(2, 81, 6, 100), PLAY(2, 86, 6, 100), PLAY(2, 90, 30, 110), END(6)
};
// Held: a high warble, round and round
static const SeqStep_t _mystery[] = {
	PROG(GM_SQUARE_LEAD), BEND(64), ON(0, 86, 55), MARK, SWEEP(0, 76, 8), SWEEP(8, 52, 8), LOOP(8)
};

const SeqSound_t inv_sound_data[INV_SFX_SOUNDS] = {
	[INV_SFX_SHOOT] = { _shoot, 3, SEQ_LEVEL_NONE },
	[INV_SFX_HIT] = { _hit, 5, SEQ_LEVEL_NONE },
	[INV_SFX_ARMOUR] = { _armour, 5, SEQ_LEVEL_NONE },
	[INV_SFX_MARCH_1] = { _march_1, 4, SEQ_LEVEL_NONE },
	[INV_SFX_MARCH_2] = { _march_2, 4, SEQ_LEVEL_NONE },
	[INV_SFX_MARCH_3] = { _march_3, 4, SEQ_LEVEL_NONE },
	[INV_SFX_MARCH_4] = { _march_4, 4, SEQ_LEVEL_NONE },
	[INV_SFX_DIVE] = { _dive, 3, SEQ_LEVEL_NONE },
	[INV_SFX_BASE] = { _base, 1, SEQ_LEVEL_NONE },
	[INV_SFX_MYSTERY_HIT] = { _mystery_hit, 7, SEQ_LEVEL_NONE },
	[INV_SFX_DEAD] = { _dead, 9, SEQ_LEVEL_NONE },
	[INV_SFX_LANDED] = { _landed, 10, SEQ_LEVEL_NONE },
	[INV_SFX_EXTRA_LIFE] = { _extra_life, 8, SEQ_LEVEL_NONE },
	[INV_SFX_MYSTERY] = { _mystery, 2, SEQ_LEVEL_NONE },
};

// ----------------------------------------------------------------------------
// Tunes: our own, in D minor, a wobbly whistle for a theremin over
// vibraphone, a synth bass and a lounge beat, at about 128 beats a minute,
// a beat Q ticks

#define Q 56
#define E8 (Q / 2)
#define S16 (Q / 4)

// A note e eighths long
#define L(n, e) NOTE(n, (e) * E8, 100)
// The vibraphone, a bar of a chord broken up and down
#define V(n) NOTE(n, E8, 65)
#define VIBES(a, b, c, d) V(a), V(b), V(c), V(d), V(c), V(b), V(a), V(b)
// The bass, a bar: root, fifth, root, octave, fifth
#define BASS(r) NOTE(r, Q, 100), NOTE((r) + 7, E8, 80), NOTE(r, E8, 90), NOTE((r) + 12, Q, 90), NOTE((r) + 7, Q, 80)

// Drums, an eighth each
#define KICK GM_DRUM_BASS
#define STICK GM_DRUM_SIDE_STICK
#define HAT GM_DRUM_CLOSED_HI_HAT
#define HIT(a) ON(0, a, 100), ON(0, HAT, 60), OFF(4, a), OFF(0, HAT), REST(E8 - 4)
#define HH ON(0, HAT, 50), OFF(4, HAT), REST(E8 - 4)
#define LOUNGE HIT(KICK), HH, HIT(STICK), HH, HH, HIT(KICK), HIT(STICK), HH

// The notes
#define D4 62
#define F4 65
#define FS4 66
#define G4 67
#define GS4 68
#define A4 69
#define BB4 70
#define C5 72
#define CS5 73
#define D5 74
#define E5 76
#define F5 77
#define G5 79
#define A5 81
#define BB5 82
#define D6 86

// The chords for the vibraphone, and the bass's roots
#define VDM VIBES(57, 62, 65, 69)
#define VBB VIBES(58, 62, 65, 70)
#define VC VIBES(55, 60, 64, 67)
#define VA7 VIBES(57, 61, 64, 67)
#define VGM VIBES(55, 58, 62, 67)

// The title, eight bars: D minor, B flat, C, A7, D minor, B flat, G minor, A7
static const SeqStep_t _title_lead[] = {
	PROG(GM_WHISTLE), CC(CC_EXPRESSION, 100), CC(CC_MODULATION, 50),
	L(D5, 3), L(A4, 1), L(D5, 2), L(F5, 2),
	L(F5, 3), L(E5, 1), L(D5, 4),
	L(E5, 3), L(C5, 1), L(E5, 2), L(G5, 2),
	L(A5, 3), L(G5, 1), L(E5, 2), L(CS5, 2),
	L(D5, 2), L(F5, 2), L(A5, 2), L(D6, 2),
	L(BB5, 3), L(A5, 1), L(F5, 2), L(D5, 2),
	L(G5, 2), L(BB5, 2), L(A5, 2), L(G5, 2),
	L(E5, 2), L(CS5, 2), L(A4, 4),
	END(0)
};
static const SeqStep_t _title_vibes[] = {
	PROG(GM_VIBRAPHONE), CC(CC_EXPRESSION, 80),
	VDM, VBB, VC, VA7, VDM, VBB, VGM, VA7,
	END(0)
};
static const SeqStep_t _title_bass[] = {
	PROG(GM_SYNTH_BASS_1), CC(CC_EXPRESSION, 100),
	BASS(38), BASS(34), BASS(36), BASS(33), BASS(38), BASS(34), BASS(31), BASS(33),
	END(0)
};
static const SeqStep_t _title_drums[] = {
	LOUNGE, LOUNGE, LOUNGE, LOUNGE, LOUNGE, LOUNGE, LOUNGE, LOUNGE,
	END(0)
};
static const SeqTrack_t _title[] = {
	{ 0, _title_lead },
	{ 1, _title_vibes },
	{ 2, _title_bass },
	{ GM_DRUM_CHANNEL, _title_drums },
};

// A wave starting: up the toms to a brass call, and D minor, held
#define T(d) ON(0, d, 100), OFF(4, d), REST(S16 - 4)
static const SeqStep_t _wave_lead[] = {
	PROG(GM_SYNTH_BRASS_1), CC(CC_EXPRESSION, 110),
	NOTE(A4, S16, 110), NOTE(D5, S16, 110), NOTE(F5, S16, 110), NOTE(A5, 3 * S16, 115),
	NOTE(G5, S16, 110), NOTE(F5, S16, 110), NOTE(E5, 2 * S16, 110), NOTE(D5, 8 * S16, 120),
	END(0)
};
static const SeqStep_t _wave_chord[] = {
	PROG(GM_SYNTH_BRASS_1), CC(CC_EXPRESSION, 90),
	REST(10 * S16), CHORD(62, 65, 69, 8 * S16, 90),
	END(0)
};
static const SeqStep_t _wave_bass[] = {
	PROG(GM_SYNTH_BASS_1), CC(CC_EXPRESSION, 100),
	NOTE(38, 2 * S16, 100), NOTE(38, 2 * S16, 100), NOTE(36, 2 * S16, 100), NOTE(33, 4 * S16, 100),
	NOTE(38, 8 * S16, 110),
	END(0)
};
static const SeqStep_t _wave_drums[] = {
	T(GM_DRUM_LOW_FLOOR_TOM), T(GM_DRUM_HIGH_FLOOR_TOM), T(GM_DRUM_LOW_TOM), T(GM_DRUM_LOW_MID_TOM),
	T(GM_DRUM_HIGH_MID_TOM), T(GM_DRUM_HIGH_TOM),
	T(GM_DRUM_SNARE), T(GM_DRUM_SNARE), T(GM_DRUM_SNARE), T(GM_DRUM_SNARE),
	ON(0, KICK, 120), ON(0, GM_DRUM_CRASH, 110), OFF(4, KICK), OFF(8 * S16 - 4, GM_DRUM_CRASH),
	END(0)
};
static const SeqTrack_t _wave[] = {
	{ 0, _wave_lead },
	{ 1, _wave_chord },
	{ 2, _wave_bass },
	{ GM_DRUM_CHANNEL, _wave_drums },
};

// Game over: the whistle creeping down from A to D, over D minor and B flat
#define GQ 60
static const SeqStep_t _over_lead[] = {
	PROG(GM_WHISTLE), CC(CC_EXPRESSION, 100), CC(CC_MODULATION, 70),
	NOTE(A4, GQ, 100), NOTE(GS4, GQ, 100), NOTE(G4, GQ, 100), NOTE(FS4, GQ, 100),
	NOTE(F4, 2 * GQ, 100), NOTE(D4, 4 * GQ, 100),
	END(0)
};
static const SeqStep_t _over_pad[] = {
	PROG(GM_WARM_PAD), CC(CC_EXPRESSION, 80),
	CHORD(50, 53, 57, 4 * GQ, 70), CHORD(46, 50, 53, 2 * GQ, 70), CHORD(50, 53, 57, 4 * GQ, 70),
	END(0)
};
static const SeqStep_t _over_bass[] = {
	PROG(GM_SYNTH_BASS_1), CC(CC_EXPRESSION, 100),
	NOTE(38, 4 * GQ, 100), NOTE(34, 2 * GQ, 100), NOTE(26, 4 * GQ, 110),
	END(0)
};
static const SeqTrack_t _over[] = {
	{ 0, _over_lead },
	{ 1, _over_pad },
	{ 2, _over_bass },
};

// A new high score: a marimba bubbling in sixteenths over F, D minor,
// B flat and C
#define A16(n) ON(0, n, 75), OFF(S16 - 2, n), REST(2)
#define ARP(a, b, c, d) A16(a), A16(b), A16(c), A16(d), A16(c), A16(b), A16(a), A16(b), \
	A16(a), A16(b), A16(c), A16(d), A16(c), A16(b), A16(a), A16(b)
static const SeqStep_t _hiscore_arp[] = {
	PROG(GM_MARIMBA), CC(CC_EXPRESSION, 90),
	ARP(65, 69, 72, 77), ARP(62, 65, 69, 74), ARP(58, 62, 65, 70), ARP(60, 64, 67, 72),
	END(0)
};
static const SeqStep_t _hiscore_bass[] = {
	PROG(GM_SYNTH_BASS_1), CC(CC_EXPRESSION, 90),
	BASS(41), BASS(38), BASS(34), BASS(36),
	END(0)
};
static const SeqStep_t _hiscore_drums[] = {
	LOUNGE, LOUNGE, LOUNGE, LOUNGE,
	END(0)
};
static const SeqTrack_t _hiscore[] = {
	{ 0, _hiscore_arp },
	{ 2, _hiscore_bass },
	{ GM_DRUM_CHANNEL, _hiscore_drums },
};

const SeqTune_t inv_tune_data[INV_TUNES] = {
	[INV_TUNE_TITLE] = { _title, 4, true },
	[INV_TUNE_WAVE] = { _wave, 4, false },
	[INV_TUNE_GAME_OVER] = { _over, 3, false },
	[INV_TUNE_HISCORE] = { _hiscore, 3, true },
};
