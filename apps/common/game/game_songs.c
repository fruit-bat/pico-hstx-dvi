// Copyright (c) 2024 fruit-bat.

// See game_songs.h. Every track of a song is the same length, so it loops
// cleanly. Each song keeps its own tempo macros, undefined after it.

#include "game_songs.h"
#include "game_seq_steps.h"

// Drums: a hit len ticks long, one, two or three at once
#define DR(a, v, len) ON(0, a, v), OFF(4, a), REST((len) - 4)
#define DR2(a, va, b, vb, len) ON(0, a, va), ON(0, b, vb), OFF(4, a), OFF(0, b), REST((len) - 4)
#define DR3(a, va, b, vb, c, vc, len) ON(0, a, va), ON(0, b, vb), ON(0, c, vc), OFF(4, a), OFF(0, b), OFF(0, c), REST((len) - 4)
#define KICK GM_DRUM_BASS
#define SNARE GM_DRUM_SNARE
#define HAT GM_DRUM_CLOSED_HI_HAT

// Notes b beats, or e eighths, long, of the song's beat Q and eighth E8
#define NB(n, b, v) NOTE(n, (b) * Q, v)
#define NE(n, e, v) NOTE(n, (e) * E8, v)

// The notes, C4 being middle C, 60; sharps and flats as needed
#define C1 24
#define CS1 25
#define D1 26
#define EB1 27
#define E1 28
#define F1 29
#define FS1 30
#define G1 31
#define AB1 32
#define A1 33
#define BB1 34
#define B1 35
#define GS1 AB1
#define CB2 B1
#define C2 36
#define CS2 37
#define D2 38
#define EB2 39
#define E2 40
#define F2 41
#define FS2 42
#define G2 43
#define AB2 44
#define A2 45
#define BB2 46
#define B2 47
#define GS2 AB2
#define CB3 B2
#define C3 48
#define CS3 49
#define D3 50
#define EB3 51
#define E3 52
#define F3 53
#define FS3 54
#define G3 55
#define AB3 56
#define A3 57
#define BB3 58
#define B3 59
#define GS3 AB3
#define CB4 B3
#define C4 60
#define CS4 61
#define D4 62
#define EB4 63
#define E4 64
#define F4 65
#define FS4 66
#define G4 67
#define AB4 68
#define A4 69
#define BB4 70
#define B4 71
#define GS4 AB4
#define CB5 B4
#define C5 72
#define CS5 73
#define D5 74
#define EB5 75
#define E5 76
#define F5 77
#define FS5 78
#define G5 79
#define AB5 80
#define A5 81
#define BB5 82
#define B5 83
#define GS5 AB5
#define CB6 B5
#define C6 84
#define CS6 85
#define D6 86
#define EB6 87
#define E6 88
#define F6 89
#define FS6 90
#define G6 91
#define AB6 92
#define A6 93
#define BB6 94
#define B6 95
#define GS6 AB6
#define CB7 B6

// ----------------------------------------------------------------------------
// Saucer March: eight bars, D minor, D minor, B flat, A, D minor, B flat,
// G minor, A, a beat Q ticks

#define Q 60
#define E8 (Q / 2)
#define S16 (Q / 4)
// A note b beats long
#define L(n, b) NOTE(n, (b) * Q, 100)

static const SeqStep_t _march_lead[] = {
	PROG(GM_WHISTLE), CC(CC_EXPRESSION, 105), CC(CC_MODULATION, 70), CC(CC_REVERB, 70),
	L(A4, 2), L(D5, 2), L(F5, 3), L(E5, 1), L(D5, 2), L(BB4, 2), L(A4, 4),
	L(A4, 1), L(D5, 1), L(F5, 1), L(A5, 1), L(BB5, 3), L(A5, 1),
	L(G5, 2), L(BB5, 1), L(G5, 1), L(CS5, 2), L(E5, 2),
	END(0)
};
// A bar of stabs on two and four
#define STABS(a, b, c) REST(Q), CHORD(a, b, c, E8, 85), REST(E8), REST(Q), CHORD(a, b, c, E8, 75), REST(E8)
#define SDM STABS(50, 53, 57)
#define SBB STABS(50, 53, 58)
#define SA STABS(49, 52, 57)
#define SGM STABS(50, 55, 58)
static const SeqStep_t _march_brass[] = {
	PROG(GM_BRASS_SECTION), CC(CC_EXPRESSION, 85),
	SDM, SDM, SBB, SA, SDM, SBB, SGM, SA,
	END(0)
};
// Root, root root, root, and a semitone below to creep back
#define TUBA(r) NOTE(r, Q, 110), NOTE(r, E8, 90), NOTE(r, E8, 90), NOTE(r, Q, 100), NOTE((r) - 1, Q, 95)
static const SeqStep_t _march_tuba[] = {
	PROG(GM_TUBA), CC(CC_EXPRESSION, 110),
	TUBA(38), TUBA(38), TUBA(34), TUBA(33), TUBA(38), TUBA(34), TUBA(31), TUBA(33),
	END(0)
};
// A bar of snare in sixteenths, accents on the beats, ghost notes between
#define ACC DR(SNARE, 100, S16)
#define GH DR(SNARE, 40, S16)
#define KACC DR2(KICK, 110, SNARE, 105, S16)
#define R16 REST(S16)
#define MBAR_REST R16, GH, GH, ACC, R16, GH, R16, KACC, R16, GH, GH, ACC, GH, GH, GH
#define MBAR KACC, MBAR_REST
static const SeqStep_t _march_drums[] = {
	DR2(GM_DRUM_CRASH, 90, KICK, 110, S16), MBAR_REST,
	MBAR, MBAR, MBAR, MBAR, MBAR, MBAR, MBAR,
	END(0)
};
const SeqTrack_t game_song_saucer_march[4] = {
	{ 0, _march_lead },
	{ 1, _march_brass },
	{ 2, _march_tuba },
	{ GM_DRUM_CHANNEL, _march_drums },
};

#undef Q
#undef E8
#undef S16
#undef L

// ----------------------------------------------------------------------------
// Atomic Surf: eight bars, D minor, C, B flat, A, twice, a beat Q ticks

#define Q 48
#define E8 (Q / 2)
#define S16 (Q / 4)

// A beat, and two, of a note picked in sixteenths
#define T1(n) ON(0, n, 95), OFF(S16 - 2, n), REST(2), ON(0, n, 75), OFF(S16 - 2, n), REST(2), \
	ON(0, n, 85), OFF(S16 - 2, n), REST(2), ON(0, n, 75), OFF(S16 - 2, n), REST(2)
#define T2(n) T1(n), T1(n)
static const SeqStep_t _surf_lead[] = {
	PROG(GM_CLEAN_GUITAR), CC(CC_EXPRESSION, 110), CC(CC_REVERB, 127),
	T1(D5), T1(F5), T2(A5),
	T1(G5), T1(E5), T2(C5),
	T1(F5), T1(D5), T2(BB4),
	T1(CS5), T1(E5), T2(A5),
	T1(A5), T1(BB5), T1(A5), T1(F5),
	T1(G5), T1(A5), T1(G5), T1(E5),
	T1(F5), T1(G5), T1(F5), T1(D5),
	T2(E5), T1(CS5), T1(A4),
	END(0)
};
// A bar of the chord on the off-beats
#define OFFB(a, b, c) REST(E8), CHORD(a, b, c, E8, 60)
#define ORG(a, b, c) OFFB(a, b, c), OFFB(a, b, c), OFFB(a, b, c), OFFB(a, b, c)
#define ODM ORG(62, 65, 69)
#define OC ORG(60, 64, 67)
#define OBB ORG(58, 62, 65)
#define OA ORG(61, 64, 69)
static const SeqStep_t _surf_organ[] = {
	PROG(GM_PERCUSSIVE_ORGAN), CC(CC_EXPRESSION, 80),
	ODM, OC, OBB, OA, ODM, OC, OBB, OA,
	END(0)
};
// A bar of the root in eighths, up to the octave and fifth at the end
#define EIGHTS(r) NOTE(r, E8, 105), NOTE(r, E8, 80), NOTE(r, E8, 95), NOTE(r, E8, 80), \
	NOTE(r, E8, 100), NOTE(r, E8, 80), NOTE((r) + 12, E8, 95), NOTE((r) + 7, E8, 85)
static const SeqStep_t _surf_bass[] = {
	PROG(GM_PICK_BASS), CC(CC_EXPRESSION, 110),
	EIGHTS(38), EIGHTS(36), EIGHTS(34), EIGHTS(33), EIGHTS(38), EIGHTS(36), EIGHTS(34), EIGHTS(33),
	END(0)
};
// A beat: a kick or snare with the hat, then the hat on the and
#define SBEAT(a) DR2(a, 100, HAT, 70, E8), DR(HAT, 50, E8)
#define SURF SBEAT(KICK), SBEAT(SNARE), SBEAT(KICK), SBEAT(SNARE)
#define TOM(d) DR(d, 95, S16)
#define FILL SBEAT(KICK), SBEAT(SNARE), \
	TOM(GM_DRUM_HIGH_TOM), TOM(GM_DRUM_HIGH_TOM), TOM(GM_DRUM_HIGH_MID_TOM), TOM(GM_DRUM_HIGH_MID_TOM), \
	TOM(GM_DRUM_LOW_TOM), TOM(GM_DRUM_LOW_TOM), TOM(GM_DRUM_LOW_FLOOR_TOM), TOM(GM_DRUM_LOW_FLOOR_TOM)
static const SeqStep_t _surf_drums[] = {
	DR2(GM_DRUM_CRASH, 100, KICK, 110, E8), DR(HAT, 50, E8), SBEAT(SNARE), SBEAT(KICK), SBEAT(SNARE),
	SURF, SURF, SURF, SURF, SURF, SURF, FILL,
	END(0)
};
const SeqTrack_t game_song_atomic_surf[4] = {
	{ 0, _surf_lead },
	{ 1, _surf_organ },
	{ 2, _surf_bass },
	{ GM_DRUM_CHANNEL, _surf_drums },
};

#undef Q
#undef E8
#undef S16

// ----------------------------------------------------------------------------
// Clockwork Invention: baroque precision for the docking computer, G
// major round the circle of fifths, harpsichord sixteenths, pizzicato bass,
// a recorder, and a woodblock going tick tock, 120 a minute, 16 bars

#define Q 60
#define E8 (Q / 2)
#define S16 (Q / 4)

#define H(n, v) ON(0, n, v), OFF(S16 - 3, n), REST(3)
#define CW_B1(a, b, c) H(a, 90), H(b, 60), H(c, 70), H(b, 60)
#define CW_B2(a, b, c) H((a) + 12, 85), H(c, 60), H(b, 70), H(c, 60)
#define CW_B3(a, b, c) H(a, 85), H(b, 60), H(c, 70), H((a) + 12, 65)
#define CW_B4(a, b, c) H(c, 80), H(b, 60), H(a, 70), H(b, 60)
#define FIG(...) FIG_(__VA_ARGS__)
#define FIG_(a, b, c) CW_B1(a, b, c), CW_B2(a, b, c), CW_B3(a, b, c), CW_B4(a, b, c)
#define FIG2(...) FIG2_(__VA_ARGS__)
#define FIG2_(a, b, c, x, y, z) CW_B1(a, b, c), CW_B2(a, b, c), CW_B3(x, y, z), CW_B4(x, y, z)
#define CG G3, B3, D4
#define CMAJ G3, C4, E4
#define CFSD FS3, A3, C4
#define CBM FS3, B3, D4
#define CEM G3, B3, E4
#define CAM A3, C4, E4
#define CE7 GS3, B3, D4

static const SeqStep_t _cw_harpsichord[] = {
	PROG(GM_HARPSICHORD), CC(CC_EXPRESSION, 100), CC(CC_REVERB, 50),
	FIG(CG), FIG(CMAJ), FIG(CFSD), FIG(CBM), FIG(CEM), FIG(CAM), FIG(CFSD), FIG(CG),
	FIG(CG), FIG(CE7), FIG(CAM), FIG(CFSD), FIG(CBM), FIG(CEM), FIG2(CAM, CFSD), FIG(CG),
	END(0)
};
#define PN(n, v) NOTE(n, E8, v)
#define PZH(r, f) PN(r, 90), PN((r) + 12, 65), PN(f, 75), PN((r) + 12, 65)
#define PZ(r, f) PZH(r, f), PZH(r, f)
static const SeqStep_t _cw_bass[] = {
	PROG(GM_PIZZICATO_STRINGS), CC(CC_EXPRESSION, 110),
	PZ(G2, D3), PZ(C2, G2), PZ(FS2, C3), PZ(B1, FS2), PZ(E2, B2), PZ(A1, E2), PZ(D2, A2), PZ(G2, D3),
	PZ(G2, D3), PZ(E2, B2), PZ(A2, E3), PZ(D2, A2), PZ(B1, FS2), PZ(E2, B2), PZH(A2, E3), PZH(D2, A2), PZ(G2, D3),
	END(0)
};
#define R(n, b) NB(n, b, 75)
static const SeqStep_t _cw_recorder[] = {
	PROG(GM_RECORDER), CC(CC_EXPRESSION, 95), CC(CC_REVERB, 60),
	R(B5, 2), R(D6, 2), R(E6, 2), R(G6, 2), R(A5, 2), R(C6, 2), R(D6, 2), R(FS6, 2),
	R(G5, 2), R(B5, 2), R(C6, 2), R(E6, 2), R(A5, 2), R(C6, 2), R(B5, 4),
	R(D6, 2), R(G6, 2), R(GS6, 2), R(E6, 2), R(A6, 2), R(E6, 2), R(A6, 2), R(FS6, 2),
	R(FS6, 2), R(D6, 2), R(G6, 2), R(E6, 2), R(E6, 2), R(FS6, 2), R(G6, 4),
	END(0)
};
#define TICK DR(GM_DRUM_HIGH_WOOD_BLOCK, 40, Q), DR(GM_DRUM_LOW_WOOD_BLOCK, 35, Q)
#define TBAR TICK, TICK
#define T4 TBAR, TBAR, TBAR, TBAR
static const SeqStep_t _cw_drums[] = {
	T4, T4, T4, T4,
	END(0)
};
const SeqTrack_t game_song_clockwork_invention[4] = {
	{ 0, _cw_harpsichord }, { 1, _cw_bass }, { 2, _cw_recorder }, { GM_DRUM_CHANNEL, _cw_drums },
};
#undef Q
#undef E8
#undef S16
#undef R

// ----------------------------------------------------------------------------
// Zero-G Lullaby: a music box drifting in 6/8, F major, over a harp,
// strings and a cello, 80 dotted crotchets a minute, 16 bars

#define E8 30
#define BAR (6 * E8)

#define ZG_BARS(C) \
	C(F3, A3, C4), C(D3, F3, A3), C(BB2, D3, F3), C(C3, E3, G3), \
	C(F3, A3, C4), C(A2, C3, E3), C(BB2, D3, F3), C(C3, E3, G3), \
	C(D3, F3, A3), C(BB2, D3, F3), C(F3, A3, C4), C(C3, E3, G3), \
	C(BB2, D3, F3), C(G2, BB2, D3), C(C3, F3, G3), C(C3, E3, G3)
#define HN(n, v) NOTE(n, E8, v)
#define ZG_HARP(r, t, f) HN(r, 70), HN(f, 55), HN((r) + 12, 60), HN((t) + 12, 60), HN((f) + 12, 55), HN((t) + 12, 50)
#define ZG_STRINGS(r, t, f) CHORD((r) + 12, (t) + 12, (f) + 12, BAR, 42)
#define ZG_CELLO(r, t, f) NOTE((r) - 12, BAR, 62)
#define M(n, e) NE(n, e, 95)

static const SeqStep_t _zg_box[] = {
	PROG(GM_MUSIC_BOX), CC(CC_EXPRESSION, 110), CC(CC_REVERB, 100),
	M(A5, 2), M(G5, 1), M(F5, 2), M(A5, 1),
	M(D6, 3), M(C6, 2), M(A5, 1),
	M(BB5, 2), M(A5, 1), M(G5, 2), M(F5, 1),
	M(E5, 3), M(G5, 3),
	M(A5, 2), M(G5, 1), M(F5, 2), M(A5, 1),
	M(C6, 3), M(A5, 3),
	M(D6, 2), M(C6, 1), M(BB5, 2), M(G5, 1),
	M(C6, 6),
	M(D6, 2), M(E6, 1), M(F6, 3),
	M(D6, 2), M(C6, 1), M(BB5, 3),
	M(A5, 2), M(BB5, 1), M(C6, 3),
	M(G5, 2), M(A5, 1), M(G5, 3),
	M(F6, 3), M(D6, 3),
	M(BB5, 2), M(A5, 1), M(G5, 3),
	M(F5, 3), M(G5, 3),
	M(E5, 3), M(G5, 3),
	END(0)
};
static const SeqStep_t _zg_harp[] = {
	PROG(GM_ORCHESTRAL_HARP), CC(CC_EXPRESSION, 95), CC(CC_REVERB, 90),
	ZG_BARS(ZG_HARP),
	END(0)
};
static const SeqStep_t _zg_strings[] = {
	PROG(GM_STRING_ENSEMBLE_2), CC(CC_EXPRESSION, 80), CC(CC_REVERB, 90),
	ZG_BARS(ZG_STRINGS),
	END(0)
};
static const SeqStep_t _zg_cello[] = {
	PROG(GM_CELLO), CC(CC_EXPRESSION, 90),
	ZG_BARS(ZG_CELLO),
	END(0)
};
const SeqTrack_t game_song_zero_g_lullaby[4] = {
	{ 0, _zg_box }, { 1, _zg_harp }, { 2, _zg_strings }, { 3, _zg_cello },
};
#undef E8
#undef BAR

// ----------------------------------------------------------------------------
// Grand Approach: stately and orchestral, E flat major, a horn tune over
// strings and double basses, a choir joining for the second half, timpani,
// 60 minims a minute, 16 bars

#define Q 60
#define S16 (Q / 4)

#define GA_BARS(C) \
	C(G3, BB3, EB4), C(AB3, C4, EB4), C(G3, BB3, EB4), C(BB3, D4, F4), \
	C(G3, C4, EB4), C(AB3, C4, EB4), C(BB3, EB4, F4), C(BB3, D4, F4), \
	C(G3, BB3, EB4), C(BB3, D4, G4), C(AB3, C4, EB4), C(AB3, C4, F4), \
	C(G3, BB3, EB4), C(AB3, D4, F4), C(AB3, CB4, EB4), C(BB3, D4, F4)
#define GA_STRINGS(a, b, c) CHORD(a, b, c, 2 * Q, 72), CHORD(a, b, c, 2 * Q, 62)
#define GA_CHOIR(a, b, c) CHORD((a) + 12, (b) + 12, (c) + 12, 4 * Q, 55)
#define GA_BASS(r) NOTE(r, 2 * Q, 95), NOTE(r, 2 * Q, 85)
#define HB(n, b) NB(n, b, 100)

static const SeqStep_t _ga_horn[] = {
	PROG(GM_FRENCH_HORN), CC(CC_EXPRESSION, 110), CC(CC_REVERB, 80),
	HB(G4, 2), HB(BB4, 2), HB(C5, 3), HB(BB4, 1), HB(G4, 2), HB(EB4, 2), HB(F4, 4),
	HB(G4, 2), HB(C5, 2), HB(EB5, 3), HB(C5, 1), HB(BB4, 2), HB(EB5, 2), HB(D5, 4),
	HB(EB5, 2), HB(BB4, 2), HB(D5, 3), HB(BB4, 1), HB(C5, 2), HB(EB5, 2), HB(F5, 2), HB(C5, 2),
	HB(EB5, 2), HB(G5, 2), HB(F5, 3), HB(D5, 1), HB(EB5, 2), HB(CB5, 2), HB(BB4, 4),
	END(0)
};
static const SeqStep_t _ga_strings[] = {
	PROG(GM_STRING_ENSEMBLE_1), CC(CC_EXPRESSION, 95), CC(CC_REVERB, 80),
	GA_BARS(GA_STRINGS),
	END(0)
};
static const SeqStep_t _ga_bass[] = {
	PROG(GM_CONTRABASS), CC(CC_EXPRESSION, 105),
	GA_BASS(EB2), GA_BASS(EB2), GA_BASS(EB2), GA_BASS(D2), GA_BASS(C2), GA_BASS(AB1), GA_BASS(BB1), GA_BASS(BB1),
	GA_BASS(EB2), GA_BASS(D2), GA_BASS(AB2), GA_BASS(F2), GA_BASS(BB1), GA_BASS(BB1), GA_BASS(AB1), GA_BASS(BB1),
	END(0)
};
static const SeqStep_t _ga_choir[] = {
	PROG(GM_CHOIR_AAHS), CC(CC_EXPRESSION, 90), CC(CC_REVERB, 100),
	REST(4 * Q), REST(4 * Q), REST(4 * Q), REST(4 * Q), REST(4 * Q), REST(4 * Q), REST(4 * Q), REST(4 * Q),
	GA_CHOIR(G3, BB3, EB4), GA_CHOIR(BB3, D4, G4), GA_CHOIR(AB3, C4, EB4), GA_CHOIR(AB3, C4, F4),
	GA_CHOIR(G3, BB3, EB4), GA_CHOIR(AB3, D4, F4), GA_CHOIR(AB3, CB4, EB4), GA_CHOIR(BB3, D4, F4),
	END(0)
};
#define TIM(n) NOTE(n, Q, 100), REST(3 * Q)
#define TR(n, v) ON(0, n, v), OFF(S16 - 3, n), REST(3)
#define TROLL(n) NOTE(n, Q, 95), REST(Q), TR(n, 50), TR(n, 58), TR(n, 66), TR(n, 74), TR(n, 82), TR(n, 90), TR(n, 100), TR(n, 110)
static const SeqStep_t _ga_timpani[] = {
	PROG(GM_TIMPANI), CC(CC_EXPRESSION, 110),
	TIM(EB3), TIM(EB3), TIM(EB3), TIM(D3), TIM(C3), TIM(AB2), TIM(BB2), TROLL(BB2),
	TIM(EB3), TIM(D3), TIM(AB2), TIM(F3), TIM(BB2), TIM(BB2), TIM(AB2), TROLL(BB2),
	END(0)
};
#define BARREST REST(4 * Q)
static const SeqStep_t _ga_drums[] = {
	DR(GM_DRUM_CRASH, 60, 4 * Q), BARREST, BARREST, BARREST, BARREST, BARREST, BARREST, BARREST,
	DR(GM_DRUM_CRASH, 100, 4 * Q), BARREST, BARREST, BARREST, DR(GM_DRUM_CRASH, 85, 4 * Q), BARREST, BARREST, BARREST,
	END(0)
};
const SeqTrack_t game_song_grand_approach[6] = {
	{ 0, _ga_horn }, { 1, _ga_strings }, { 2, _ga_bass }, { 3, _ga_choir }, { 4, _ga_timpani }, { GM_DRUM_CHANNEL, _ga_drums },
};
#undef Q
#undef S16

// ----------------------------------------------------------------------------
// Night Orbit: 80s synthwave, A minor, pumping octave bass, a polysynth
// pad and arpeggio, a saw lead from bar 5, gated snare, 129 a minute, 16 bars

#define Q 56
#define E8 (Q / 2)
#define S16 (Q / 4)

#define NO_BARS(C) \
	C(A3, C4, E4, A1), C(A3, C4, F4, F1), C(G3, C4, E4, C2), C(G3, B3, D4, G1), \
	C(A3, C4, E4, A1), C(A3, C4, F4, F1), C(G3, C4, E4, C2), C(G3, B3, D4, G1), \
	C(A3, C4, F4, F1), C(G3, B3, D4, G1), C(G3, B3, E4, E1), C(A3, C4, E4, A1), \
	C(A3, D4, F4, D2), C(A3, C4, F4, F1), C(G3, B3, D4, G1), C(GS3, B3, E4, E1)
#define H16(n, v) ON(0, n, v), OFF(S16 - 3, n), REST(3)
#define NO_B4(r) H16(r, 100), H16(r, 65), H16((r) + 12, 85), H16(r, 65)
#define NO_BASS(a, b, c, r) NO_B4(r), NO_B4(r), NO_B4(r), NO_B4(r)
#define NO_PAD(a, b, c, r) CHORD(a, b, c, 4 * Q, 52)
#define H8(n, v) ON(0, n, v), OFF(E8 - 4, n), REST(4)
#define NO_ARP(a, b, c, r) H8((a) + 12, 50), H8((b) + 12, 40), H8((c) + 12, 45), H8((a) + 24, 50), \
	H8((c) + 12, 40), H8((b) + 12, 45), H8((a) + 12, 40), H8((b) + 12, 45)
#define LD(n, e) NE(n, e, 92)

static const SeqStep_t _no_lead[] = {
	PROG(GM_SAW_LEAD), CC(CC_EXPRESSION, 75), CC(CC_REVERB, 100), CC(CC_MODULATION, 20),
	REST(4 * Q), REST(4 * Q), REST(4 * Q), REST(4 * Q),
	LD(E5, 3), LD(D5, 1), LD(C5, 2), LD(A4, 2),
	LD(C5, 3), LD(A4, 1), LD(C5, 2), LD(F5, 2),
	LD(E5, 3), LD(D5, 1), LD(C5, 2), LD(G5, 2),
	LD(D5, 6), LD(B4, 2),
	LD(A5, 3), LD(G5, 1), LD(F5, 2), LD(E5, 2),
	LD(D5, 3), LD(E5, 1), LD(G5, 4),
	LD(G5, 3), LD(E5, 1), LD(B4, 2), LD(E5, 2),
	LD(A5, 6), LD(G5, 2),
	LD(F5, 3), LD(E5, 1), LD(D5, 2), LD(A5, 2),
	LD(C6, 4), LD(A5, 2), LD(F5, 2),
	LD(B5, 3), LD(A5, 1), LD(G5, 2), LD(D5, 2),
	LD(GS5, 4), LD(E5, 4),
	END(0)
};
static const SeqStep_t _no_pad[] = {
	PROG(GM_POLYSYNTH_PAD), CC(CC_EXPRESSION, 65), CC(CC_REVERB, 90),
	NO_BARS(NO_PAD),
	END(0)
};
static const SeqStep_t _no_arp[] = {
	PROG(GM_SQUARE_LEAD), CC(CC_EXPRESSION, 65), CC(CC_REVERB, 100),
	NO_BARS(NO_ARP),
	END(0)
};
static const SeqStep_t _no_bass[] = {
	PROG(GM_SYNTH_BASS_1), CC(CC_EXPRESSION, 80),
	NO_BARS(NO_BASS),
	END(0)
};
#define SN GM_DRUM_ELECTRIC_SNARE
#define CLAP GM_DRUM_HAND_CLAP
#define KB DR2(KICK, 105, HAT, 60, E8), DR(HAT, 40, E8)
#define SB DR3(SN, 100, CLAP, 70, HAT, 60, E8), DR(HAT, 40, E8)
#define LIGHT KB, DR(HAT, 55, E8), DR(HAT, 40, E8), KB, DR(HAT, 55, E8), DR(HAT, 40, E8)
#define FULL KB, SB, KB, SB
#define CRASHED DR2(KICK, 110, GM_DRUM_CRASH, 90, E8), DR(HAT, 40, E8), SB, KB, SB
#define NO_FILL KB, SB, KB, DR(SN, 60, S16), DR(SN, 75, S16), DR(SN, 90, S16), DR(SN, 110, S16)
static const SeqStep_t _no_drums[] = {
	CC(CC_EXPRESSION, 80),
	LIGHT, LIGHT, LIGHT, LIGHT,
	CRASHED, FULL, FULL, FULL, CRASHED, FULL, FULL, FULL, FULL, FULL, FULL, NO_FILL,
	END(0)
};
const SeqTrack_t game_song_night_orbit[5] = {
	{ 0, _no_lead }, { 1, _no_pad }, { 2, _no_arp }, { 3, _no_bass }, { GM_DRUM_CHANNEL, _no_drums },
};
