// Copyright (c) 2024 fruit-bat.

// The sounds and tunes, as steps for the sequencer (game_seq.h), in General
// MIDI's instruments, numbered from 0. Sounds bend two octaves either way,
// so a bend's step is three eighths of a semitone.

#include "el_sound.h"
#include "game_seq_steps.h"

// ----------------------------------------------------------------------------
// Sounds

static const SeqStep_t _laser[] = {
	PROG(GM_SAW_LEAD), BEND(96), ON(0, 76, 110), SWEEP(0, 24, 10), OFF(10, 76), END(2)
};
static const SeqStep_t _hit[] = {
	PROG(GM_DISTORTION_GUITAR), BEND(64), ON(0, 28, 100), ON(0, 35, 100), SWEEP(0, 40, 14), OFF(14, 28), OFF(0, 35), END(4)
};
static const SeqStep_t _explosion[] = {
	PROG(GM_GUNSHOT), BEND(64), ON(0, 30, 127), SWEEP(0, 10, 60), ON(8, 26, 110), OFF(62, 30), OFF(0, 26), END(10)
};
static const SeqStep_t _missile[] = {
	PROG(GM_SAW_LEAD), BEND(20), ON(0, 48, 100), SWEEP(0, 110, 50), OFF(50, 48), END(10)
};
static const SeqStep_t _ecm[] = {
	PROG(GM_SQUARE_LEAD), ON(0, 84, 100),
	SWEEP(0, 100, 6), SWEEP(6, 28, 6), SWEEP(6, 100, 6), SWEEP(6, 28, 6),
	SWEEP(6, 100, 6), SWEEP(6, 28, 6), SWEEP(6, 100, 6), SWEEP(6, 28, 6),
	OFF(6, 84), END(4)
};
static const SeqStep_t _bomb[] = {
	PROG(GM_REVERSE_CYMBAL), ON(0, 60, 127), OFF(150, 60), END(20)
};
static const SeqStep_t _scooped[] = {
	PROG(GM_VIBRAPHONE), ON(0, 79, 100), OFF(8, 79), ON(0, 84, 100), OFF(14, 84), END(20)
};
static const SeqStep_t _choose[] = {
	PROG(GM_GLOCKENSPIEL), ON(0, 88, 90), OFF(10, 88), END(20)
};
static const SeqStep_t _page[] = {
	PROG(GM_GLOCKENSPIEL), ON(0, 79, 90), OFF(8, 79), ON(0, 84, 90), OFF(8, 84), ON(0, 91, 90), OFF(20, 91), END(20)
};
static const SeqStep_t _count[] = {
	PROG(GM_SQUARE_LEAD), ON(0, 81, 100), OFF(10, 81), END(2)
};
static const SeqStep_t _hyperspace[] = {
	PROG(GM_SCI_FI), BEND(10), ON(0, 48, 127), SWEEP(0, 127, 180), OFF(200, 48), END(40)
};
// Engines, a low hum bent up two seconds, then dying away for a second as
// it rises a little more, turned down rather than let go, which is abrupt
static const SeqStep_t _tunnel[] = {
	PROG(GM_SYNTH_BASS_2), BEND(48), CC(11, 127), ON(0, 26, 110), ON(0, 33, 90), SWEEP(0, 96, 240),
	REST(240), SWEEP(0, 104, 120),
	CC_AFTER(10, 11, 107), CC_AFTER(10, 11, 88), CC_AFTER(10, 11, 71), CC_AFTER(10, 11, 56),
	CC_AFTER(10, 11, 43), CC_AFTER(10, 11, 32), CC_AFTER(10, 11, 22), CC_AFTER(10, 11, 14),
	CC_AFTER(10, 11, 8), CC_AFTER(10, 11, 4), CC_AFTER(10, 11, 1), CC_AFTER(10, 11, 0),
	OFF(0, 26), OFF(0, 33), END(10)
};
static const SeqStep_t _denied[] = {
	PROG(GM_SQUARE_LEAD), ON(0, 40, 90), OFF(16, 40), END(2)
};
static const SeqStep_t _dead[] = {
	PROG(GM_GUNSHOT), BEND(64), ON(0, 30, 127), SWEEP(0, 0, 120), ON(10, 26, 127), ON(14, 22, 127),
	OFF(100, 30), OFF(0, 26), OFF(0, 22), END(30)
};
static const SeqStep_t _incoming[] = {
	PROG(GM_SQUARE_LEAD), MARK, ON(0, 88, 100), OFF(12, 88), ON(6, 88, 100), OFF(12, 88), LOOP(30)
};
static const SeqStep_t _low_energy[] = {
	PROG(GM_SQUARE_LEAD), MARK, ON(0, 69, 100), OFF(30, 69), LOOP(30)
};
static const SeqStep_t _heat[] = {
	PROG(GM_SQUARE_LEAD), MARK, ON(0, 93, 80), OFF(6, 93), LOOP(54)
};
// Held on, the loop only waiting
static const SeqStep_t _scooping[] = {
	PROG(GM_ATMOSPHERE), ON(0, 52, 70), MARK, LOOP(240)
};
static const SeqStep_t _overdrive[] = {
	PROG(GM_SYNTH_BASS_2), BEND(64), ON(0, 28, 110), MARK, LOOP(240)
};

const SeqSound_t sound_data[SFX_SOUNDS] = {
	[SFX_LASER] = { _laser, 4, SEQ_LEVEL_NONE },
	[SFX_HIT] = { _hit, 7, SEQ_LEVEL_NONE },
	[SFX_EXPLOSION] = { _explosion, 6, SEQ_LEVEL_NONE },
	[SFX_MISSILE] = { _missile, 6, SEQ_LEVEL_NONE },
	[SFX_ECM] = { _ecm, 7, SEQ_LEVEL_NONE },
	[SFX_BOMB] = { _bomb, 8, SEQ_LEVEL_NONE },
	[SFX_SCOOPED] = { _scooped, 5, SEQ_LEVEL_NONE },
	[SFX_CHOOSE] = { _choose, 3, SEQ_LEVEL_NONE },
	[SFX_PAGE] = { _page, 5, SEQ_LEVEL_NONE },
	[SFX_COUNT] = { _count, 8, SEQ_LEVEL_NONE },
	[SFX_HYPERSPACE] = { _hyperspace, 8, SEQ_LEVEL_NONE },
	[SFX_TUNNEL] = { _tunnel, 8, SEQ_LEVEL_NONE },
	[SFX_DENIED] = { _denied, 3, SEQ_LEVEL_NONE },
	[SFX_DEAD] = { _dead, 10, SEQ_LEVEL_NONE },
	[SFX_INCOMING] = { _incoming, 9, SEQ_LEVEL_NONE },
	[SFX_LOW_ENERGY] = { _low_energy, 9, SEQ_LEVEL_NONE },
	[SFX_HEAT] = { _heat, 9, SEQ_LEVEL_NONE },
	[SFX_SCOOPING] = { _scooping, 2, SEQ_LEVEL_NONE },
	[SFX_OVERDRIVE] = { _overdrive, 2, SEQ_LEVEL_BEND },
};

// ----------------------------------------------------------------------------
// Tunes

// The docking computer's waltz: our own, in D major, three beats of B ticks
// a bar, 32 bars. A flute, doubled an octave up on the crystal, over an
// electric piano's two chords to a bar, a soft pad and the bass on the
// first beat. The first half goes from D to A and back, the second wanders
// through G and B minor before coming home.
#define B 44

#define W_G4 67
#define W_A4 69
#define W_B4 71
#define W_C5 72
#define W_CS5 73
#define W_D5 74
#define W_E5 76
#define W_FS5 78
#define W_G5 79
#define W_A5 81
#define W_B5 83
#define W_C6 84
#define W_D6 86

// The tune, a note at a time, for N(note, beats)
#define W_MELODY(N) \
	N(W_A4, 1), N(W_D5, 1), N(W_FS5, 1),  N(W_A5, 2), N(W_FS5, 1), \
	N(W_G5, 1), N(W_FS5, 1), N(W_E5, 1),  N(W_E5, 3), \
	N(W_G4, 1), N(W_CS5, 1), N(W_E5, 1),  N(W_G5, 2), N(W_E5, 1), \
	N(W_FS5, 1), N(W_E5, 1), N(W_D5, 1),  N(W_D5, 3), \
	N(W_A4, 1), N(W_D5, 1), N(W_FS5, 1),  N(W_A5, 2), N(W_D6, 1), \
	N(W_D6, 1), N(W_B5, 1), N(W_G5, 1),   N(W_B5, 3), \
	N(W_A5, 1), N(W_FS5, 1), N(W_D5, 1),  N(W_E5, 2), N(W_D5, 1), \
	N(W_E5, 1), N(W_CS5, 1), N(W_A4, 1),  N(W_D5, 3), \
	N(W_D5, 2), N(W_B4, 1),               N(W_G5, 3), \
	N(W_E5, 2), N(W_C5, 1),               N(W_A5, 2), N(W_FS5, 1), \
	N(W_FS5, 1), N(W_A5, 1), N(W_C6, 1),  N(W_B5, 3), \
	N(W_G5, 1), N(W_E5, 1), N(W_B4, 1),   N(W_CS5, 2), N(W_E5, 1), \
	N(W_FS5, 2), N(W_A5, 1),              N(W_D6, 2), N(W_B5, 1), \
	N(W_G5, 2), N(W_E5, 1),               N(W_A5, 1), N(W_G5, 1), N(W_E5, 1), \
	N(W_FS5, 1), N(W_A5, 1), N(W_D6, 1),  N(W_D6, 2), N(W_B5, 1), \
	N(W_A5, 1), N(W_G5, 1), N(W_CS5, 1),  N(W_D5, 3)

#define W_LEAD(n, q) NOTE(n, (q) * B, 95)
#define W_HIGH(n, q) NOTE((n) + 12, (q) * B, 50)

// A bar of each chord, for C(a, b, c), and the bass's note for it
#define W_BARS(C) \
	C(62, 66, 69), C(62, 66, 69), C(61, 64, 67), C(61, 64, 67), \
	C(61, 64, 67), C(61, 64, 67), C(62, 66, 69), C(62, 66, 69), \
	C(62, 66, 69), C(62, 66, 69), C(62, 67, 71), C(62, 67, 71), \
	C(62, 66, 69), C(62, 64, 68), C(61, 64, 67), C(62, 66, 69), \
	C(62, 67, 71), C(62, 67, 71), C(60, 64, 67), C(60, 66, 69), \
	C(60, 66, 69), C(62, 67, 71), C(64, 67, 71), C(61, 64, 67), \
	C(62, 66, 69), C(62, 66, 71), C(64, 67, 71), C(61, 64, 67), \
	C(62, 66, 69), C(62, 67, 71), C(61, 64, 67), C(62, 66, 69)
// The root, or the fifth when a chord goes on
#define W_ROOTS(R) \
	R(38), R(45), R(45), R(40), R(45), R(40), R(38), R(45), \
	R(38), R(45), R(43), R(38), R(38), R(40), R(45), R(38), \
	R(43), R(38), R(48), R(38), R(45), R(43), R(40), R(45), \
	R(38), R(47), R(40), R(45), R(38), R(43), R(45), R(38)

// Oom, pah, pah
#define W_PAH(a, b, c) REST(B), CHORD(a, b, c, B - 6, 60), REST(6), CHORD(a, b, c, B - 6, 55), REST(6)
#define W_PAD(a, b, c) CHORD(a, b, c, 3 * B - 8, 45), REST(8)
#define W_OOM(r) NOTE(r, B, 95), REST(2 * B)

static const SeqStep_t _docking_lead[] = {
	PROG(GM_FLUTE), CC(11, 105),
	W_MELODY(W_LEAD),
	END(0)
};

static const SeqStep_t _docking_high[] = {
	PROG(GM_CRYSTAL), CC(11, 80),
	W_MELODY(W_HIGH),
	END(0)
};

static const SeqStep_t _docking_pah[] = {
	PROG(GM_ELECTRIC_PIANO_2), CC(11, 90),
	W_BARS(W_PAH),
	END(0)
};

static const SeqStep_t _docking_pad[] = {
	PROG(GM_WARM_PAD), CC(11, 75),
	W_BARS(W_PAD),
	END(0)
};

static const SeqStep_t _docking_bass[] = {
	PROG(GM_ACOUSTIC_BASS), CC(11, 105),
	W_ROOTS(W_OOM),
	END(0)
};

static const SeqTrack_t _docking[] = {
	{ 0, _docking_lead },
	{ 1, _docking_high },
	{ 2, _docking_pah },
	{ 3, _docking_pad },
	{ 4, _docking_bass },
};

// The title: our own, slow, in D minor, four beats of Q ticks a bar, a pad,
// a crystal arpeggio in eighths, and a bass
#define Q 48
#define E8(n) ON(0, n, 80), OFF(Q / 2 - 4, n), REST(4)
#define ARP(a, b, c, d, e, f, g, h) E8(a), E8(b), E8(c), E8(d), E8(e), E8(f), E8(g), E8(h)
#define PAD(a, b, c) CHORD(a, b, c, 4 * Q - 8, 70), REST(8)
#define ROOT(n) NOTE(n, 2 * Q, 90), NOTE(n, 2 * Q, 80)

static const SeqStep_t _title_pad[] = {
	PROG(GM_WARM_PAD), CC(11, 100),
	PAD(50, 53, 57), PAD(46, 50, 53), PAD(53, 57, 60), PAD(48, 52, 55),
	PAD(50, 53, 57), PAD(46, 50, 53), PAD(53, 57, 60), PAD(48, 52, 55),
	END(0)
};

static const SeqStep_t _title_arp[] = {
	PROG(GM_CRYSTAL), CC(11, 90),
	ARP(62, 65, 69, 74, 69, 65, 62, 65),
	ARP(58, 62, 65, 70, 65, 62, 58, 62),
	ARP(60, 65, 69, 72, 69, 65, 60, 65),
	ARP(60, 64, 67, 72, 67, 64, 60, 64),
	ARP(74, 69, 65, 62, 65, 69, 74, 77),
	ARP(70, 65, 62, 58, 62, 65, 70, 74),
	ARP(72, 69, 65, 60, 65, 69, 72, 77),
	ARP(72, 67, 64, 60, 64, 67, 72, 76),
	END(0)
};

static const SeqStep_t _title_bass[] = {
	PROG(GM_SYNTH_BASS_1), CC(11, 100),
	ROOT(38), ROOT(34), ROOT(41), ROOT(36),
	ROOT(38), ROOT(34), ROOT(41), ROOT(36),
	END(0)
};

static const SeqTrack_t _title[] = {
	{ 0, _title_pad },
	{ 1, _title_arp },
	{ 2, _title_bass },
};

const SeqTune_t tune_data[TUNES] = {
	[TUNE_TITLE] = { _title, 3, true },
	[TUNE_DOCKING] = { _docking, 5, true },
};
