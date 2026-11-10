// Copyright (c) 2024 fruit-bat.

// The sounds and tunes, as steps for the sequencer (game_seq.h), at 120
// ticks a second, two to a frame. Sounds bend two octaves either way, so a
// bend's step is three eighths of a semitone, 64 being none.

#include "mm_sound.h"
#include "game_seq_steps.h"

// ----------------------------------------------------------------------------
// Sounds: spooky but friendly, celesta, harp, pizzicato and wood

// A slide whistle, up, soft as she jumps a lot
static const SeqStep_t _jump[] = {
	PROG(GM_WHISTLE), BEND(40), ON(0, 72, 55), SWEEP(0, 72, 10), OFF(10, 72), END(2)
};
// A swish up, sparkling
static const SeqStep_t _cast[] = {
	PROG(GM_CELESTA), BEND(40), ON(0, 84, 85), SWEEP(0, 88, 12), OFF(12, 84), END(6)
};
static const SeqStep_t _cheer[] = {
	PROG(GM_ORCHESTRAL_HARP),
	PLAY(0, 74, 4, 100), PLAY(0, 78, 4, 100), PLAY(0, 81, 4, 100), PLAY(0, 86, 24, 110), END(16)
};
// A creak, up
static const SeqStep_t _pull[] = {
	PROG(GM_CONTRABASS), BEND(48), ON(0, 40, 110), SWEEP(0, 72, 20), OFF(20, 40), END(4)
};
// Bumped: down the xylophone
static const SeqStep_t _hurt[] = {
	PROG(GM_XYLOPHONE), PLAY(0, 79, 4, 110), PLAY(0, 74, 4, 110), PLAY(0, 67, 10, 110), END(6)
};
// Swirling round and down, as long as she is dizzy
static const SeqStep_t _dizzy[] = {
	PROG(GM_FLUTE), BEND(64), ON(0, 84, 100),
	SWEEP(0, 48, 10), SWEEP(10, 72, 10), SWEEP(10, 40, 10), SWEEP(10, 64, 10),
	SWEEP(10, 32, 10), SWEEP(10, 52, 10), SWEEP(10, 8, 60),
	OFF(60, 84), END(6)
};
// Mew, then the harp
static const SeqStep_t _kitten[] = {
	PROG(GM_VIOLIN), BEND(56), ON(0, 84, 90), SWEEP(0, 72, 8), SWEEP(8, 44, 16), OFF(16, 84),
	PROG(GM_ORCHESTRAL_HARP), BEND(64),
	PLAY(4, 74, 5, 100), PLAY(0, 78, 5, 100), PLAY(0, 81, 5, 100), PLAY(0, 86, 30, 110), END(20)
};
static const SeqStep_t _sweet[] = {
	PROG(GM_MUSIC_BOX), PLAY(0, 96, 6, 70), END(6)
};
static const SeqStep_t _heart[] = {
	PROG(GM_MUSIC_BOX), PLAY(0, 84, 6, 100), PLAY(0, 91, 16, 100), END(10)
};
// A jingle
static const SeqStep_t _key[] = {
	PROG(GM_GLOCKENSPIEL),
	PLAY(0, 88, 4, 90), PLAY(0, 91, 4, 90), PLAY(0, 88, 4, 90), PLAY(0, 91, 4, 90), PLAY(0, 96, 20, 100), END(10)
};
// Sparkling up two octaves
static const SeqStep_t _feather[] = {
	PROG(GM_CELESTA),
	PLAY(0, 74, 4, 100), PLAY(0, 77, 4, 100), PLAY(0, 81, 4, 100), PLAY(0, 86, 4, 100),
	PLAY(0, 89, 4, 100), PLAY(0, 93, 4, 100), PLAY(0, 98, 4, 100),
	PLAY(4, 93, 4, 90), PLAY(0, 98, 30, 100), END(10)
};
// Clunk
static const SeqStep_t _lever[] = {
	PROG(GM_WOODBLOCK), PLAY(0, 60, 6, 120), PLAY(4, 52, 10, 120), END(4)
};
static const SeqStep_t _candle[] = {
	PROG(GM_VIBRAPHONE), PLAY(0, 81, 12, 90), END(10)
};
static const SeqStep_t _all_lit[] = {
	PROG(GM_VIBRAPHONE),
	PLAY(0, 81, 6, 100), PLAY(0, 84, 6, 100), PLAY(0, 88, 6, 100), PLAY(0, 93, 30, 110), END(10)
};
// A door creaking open
static const SeqStep_t _gate[] = {
	PROG(GM_CELLO), BEND(64), ON(0, 43, 100), SWEEP(0, 76, 40), OFF(40, 43), END(4)
};
// The key turning, and a chime
static const SeqStep_t _unlock[] = {
	PROG(GM_WOODBLOCK), PLAY(0, 72, 4, 110), PLAY(4, 67, 4, 110),
	PROG(GM_GLOCKENSPIEL), PLAY(4, 84, 6, 100), PLAY(0, 91, 20, 100), END(10)
};
// Rattling the door
static const SeqStep_t _locked[] = {
	PROG(GM_WOODBLOCK), PLAY(0, 64, 3, 100), PLAY(3, 64, 3, 100), PLAY(3, 64, 3, 100), END(4)
};
static const SeqStep_t _lantern[] = {
	PROG(GM_CELESTA), PLAY(0, 86, 8, 90), PLAY(0, 93, 20, 80), END(10)
};
static const SeqStep_t _extra_life[] = {
	PROG(GM_GLOCKENSPIEL),
	PLAY(0, 79, 6, 100), PLAY(2, 84, 6, 100), PLAY(2, 88, 6, 100), PLAY(2, 91, 6, 100),
	PLAY(2, 88, 6, 100), PLAY(2, 91, 6, 100), PLAY(2, 96, 30, 110), END(10)
};
static const SeqStep_t _bonus[] = {
	PROG(GM_GLOCKENSPIEL), PLAY(0, 91, 10, 90), END(10)
};
// Held: pan pipes, wavering, the loop only bending
static const SeqStep_t _float[] = {
	PROG(GM_PAN_FLUTE), ON(0, 84, 60), MARK, SWEEP(0, 68, 20), SWEEP(20, 60, 20), LOOP(20)
};

const SeqSound_t mm_sound_data[MM_SFX_SOUNDS] = {
	[MM_SFX_JUMP] = { _jump, 2, SEQ_LEVEL_NONE },
	[MM_SFX_CAST] = { _cast, 3, SEQ_LEVEL_NONE },
	[MM_SFX_CHEER] = { _cheer, 5, SEQ_LEVEL_NONE },
	[MM_SFX_PULL] = { _pull, 4, SEQ_LEVEL_NONE },
	[MM_SFX_HURT] = { _hurt, 7, SEQ_LEVEL_NONE },
	[MM_SFX_DIZZY] = { _dizzy, 9, SEQ_LEVEL_NONE },
	[MM_SFX_KITTEN] = { _kitten, 8, SEQ_LEVEL_NONE },
	[MM_SFX_SWEET] = { _sweet, 2, SEQ_LEVEL_NONE },
	[MM_SFX_HEART] = { _heart, 5, SEQ_LEVEL_NONE },
	[MM_SFX_KEY] = { _key, 6, SEQ_LEVEL_NONE },
	[MM_SFX_FEATHER] = { _feather, 8, SEQ_LEVEL_NONE },
	[MM_SFX_LEVER] = { _lever, 6, SEQ_LEVEL_NONE },
	[MM_SFX_CANDLE] = { _candle, 5, SEQ_LEVEL_NONE },
	[MM_SFX_ALL_LIT] = { _all_lit, 7, SEQ_LEVEL_NONE },
	[MM_SFX_GATE] = { _gate, 6, SEQ_LEVEL_NONE },
	[MM_SFX_UNLOCK] = { _unlock, 7, SEQ_LEVEL_NONE },
	[MM_SFX_LOCKED] = { _locked, 5, SEQ_LEVEL_NONE },
	[MM_SFX_LANTERN] = { _lantern, 5, SEQ_LEVEL_NONE },
	[MM_SFX_EXTRA_LIFE] = { _extra_life, 8, SEQ_LEVEL_NONE },
	[MM_SFX_BONUS] = { _bonus, 5, SEQ_LEVEL_NONE },
	[MM_SFX_FLOAT] = { _float, 2, SEQ_LEVEL_NONE },
};

// ----------------------------------------------------------------------------
// Tunes: our own, spooky waltzes in 3/4, mostly in D minor, a beat Q ticks,
// set before each tune

// A note q beats long
#define M(n, q) NOTE(n, (q) * Q, VEL)
#define R(q) REST((q) * Q)
// The waltz: the bass on the first beat, a chord on each of the other two
#define OOM(r) NOTE(r, Q, BASS_VEL), REST(2 * Q)
#define PAH(a, b, c) REST(Q), CHORD(a, b, c, Q - 6, PAH_VEL), REST(6), CHORD(a, b, c, Q - 6, PAH_VEL), REST(6)
// The chords
#define DM PAH(57, 62, 65)
#define A7 PAH(57, 61, 67)
#define GM PAH(58, 62, 67)
#define BB PAH(58, 62, 65)
#define C7 PAH(58, 64, 67)
#define FM PAH(57, 60, 65)
#define CM PAH(55, 60, 64)
// Wood blocks, a bar: low on the first beat, high on the others
#define WOOD(d, v) ON(0, d, v), OFF(4, d), REST(Q - 4)
#define CLOP WOOD(GM_DRUM_LOW_WOOD_BLOCK, 55), WOOD(GM_DRUM_HIGH_WOOD_BLOCK, 35), WOOD(GM_DRUM_HIGH_WOOD_BLOCK, 35)

// The notes
#define A4 69
#define AS4 70
#define CS5 73
#define D5 74
#define E5 76
#define F5 77
#define FS5 78
#define G5 79
#define GS5 80
#define A5 81
#define AS5 82
#define B5 83
#define C5 72
#define D6 86

// The title, sixteen bars, a harpsichord over tuba and pizzicato
#define Q 48
#define VEL 95
#define BASS_VEL 90
#define PAH_VEL 70
static const SeqStep_t _title_lead[] = {
	PROG(GM_HARPSICHORD), CC(CC_EXPRESSION, 100),
	M(D5, 1), M(F5, 1), M(A5, 1),
	M(A5, 2), M(GS5, 1),
	M(A5, 1), M(E5, 1), M(CS5, 1),
	M(D5, 2), M(A4, 1),
	M(AS4, 1), M(D5, 1), M(G5, 1),
	M(F5, 1), M(E5, 1), M(D5, 1),
	M(CS5, 1), M(E5, 1), M(G5, 1),
	M(D5, 2), R(1),
	M(F5, 1), M(AS5, 1), M(A5, 1),
	M(G5, 2), M(D5, 1),
	M(E5, 1), M(G5, 1), M(AS5, 1),
	M(A5, 2), M(F5, 1),
	M(AS4, 1), M(D5, 1), M(G5, 1),
	M(F5, 2), M(D5, 1),
	M(E5, 1), M(CS5, 1), M(A4, 1),
	M(D5, 3),
	END(0)
};
static const SeqStep_t _title_bass[] = {
	PROG(GM_TUBA), CC(CC_EXPRESSION, 100),
	OOM(38), OOM(45), OOM(45), OOM(38), OOM(43), OOM(38), OOM(45), OOM(38),
	OOM(46), OOM(43), OOM(48), OOM(41), OOM(43), OOM(38), OOM(45), OOM(38),
	END(0)
};
static const SeqStep_t _title_pah[] = {
	PROG(GM_PIZZICATO_STRINGS), CC(CC_EXPRESSION, 90),
	DM, DM, A7, DM, GM, DM, A7, DM,
	BB, GM, C7, FM, GM, DM, A7, DM,
	END(0)
};
static const SeqStep_t _title_drums[] = {
	CLOP, CLOP, CLOP, CLOP, CLOP, CLOP, CLOP, CLOP,
	CLOP, CLOP, CLOP, CLOP, CLOP, CLOP, CLOP, CLOP,
	END(0)
};
static const SeqTrack_t _title[] = {
	{ 0, _title_lead },
	{ 1, _title_pah },
	{ 2, _title_bass },
	{ GM_DRUM_CHANNEL, _title_drums },
};
#undef Q
#undef VEL
#undef BASS_VEL
#undef PAH_VEL

// While exploring: slower and quiet, a celesta over creeping pizzicato and
// a choir, sixteen bars
#define Q 56
#define VEL 70
#define CREEP(a, b, c) NOTE(a, Q, 70), NOTE(b, Q, 60), NOTE(c, Q, 60)
#define CREEP8 CREEP(50, 53, 57), CREEP(43, 46, 50), CREEP(45, 49, 52), CREEP(50, 45, 50), \
	CREEP(46, 50, 53), CREEP(43, 46, 50), CREEP(45, 49, 55), CREEP(50, 57, 50)
#define OOH(a, b, c) CHORD(a, b, c, 3 * Q - 8, 50), REST(8)
#define OOH8 OOH(62, 65, 69), OOH(62, 67, 70), OOH(61, 64, 67), OOH(62, 65, 69), \
	OOH(62, 65, 70), OOH(62, 67, 70), OOH(61, 64, 67), OOH(62, 65, 69)
static const SeqStep_t _play_lead[] = {
	PROG(GM_CELESTA), CC(CC_EXPRESSION, 75),
	R(1), M(D5, 1), M(F5, 1),
	M(G5, 2), M(F5, 1),
	M(E5, 2), M(CS5, 1),
	M(D5, 2), R(1),
	R(1), M(F5, 1), M(AS5, 1),
	M(A5, 2), M(G5, 1),
	M(G5, 1), M(E5, 1), M(CS5, 1),
	M(D5, 2), R(1),
	R(3),
	R(1), M(D5, 1), M(G5, 1),
	R(1), M(CS5, 1), M(E5, 1),
	M(F5, 2), R(1),
	R(1), M(D5, 1), M(F5, 1),
	M(AS5, 2), M(A5, 1),
	M(G5, 1), M(E5, 1), M(CS5, 1),
	M(D5, 2), R(1),
	END(0)
};
static const SeqStep_t _play_bass[] = {
	PROG(GM_PIZZICATO_STRINGS), CC(CC_EXPRESSION, 70),
	CREEP8, CREEP8,
	END(0)
};
static const SeqStep_t _play_choir[] = {
	PROG(GM_CHOIR_AAHS), CC(CC_EXPRESSION, 45),
	OOH8, OOH8,
	END(0)
};
static const SeqTrack_t _play[] = {
	{ 0, _play_lead },
	{ 1, _play_bass },
	{ 3, _play_choir },
};
#undef Q
#undef VEL

// A night starting: the celesta up, a choir, and the timpani rolling
static const SeqStep_t _night_celesta[] = {
	PROG(GM_CELESTA), CC(CC_EXPRESSION, 100),
	NOTE(74, 8, 90), NOTE(77, 8, 90), NOTE(81, 8, 95), NOTE(86, 8, 95), NOTE(89, 8, 100), NOTE(93, 120, 100),
	END(0)
};
static const SeqStep_t _night_choir[] = {
	PROG(GM_CHOIR_AAHS), CC(CC_EXPRESSION, 80),
	REST(40), CHORD(62, 65, 69, 200, 80),
	END(0)
};
static const SeqStep_t _night_timpani[] = {
	PROG(GM_TIMPANI), CC(CC_EXPRESSION, 100),
	NOTE(38, 6, 50), NOTE(38, 6, 60), NOTE(38, 6, 70), NOTE(38, 6, 80),
	NOTE(38, 6, 90), NOTE(38, 6, 100), NOTE(38, 120, 120),
	END(0)
};
static const SeqTrack_t _night[] = {
	{ 0, _night_celesta },
	{ 3, _night_choir },
	{ 4, _night_timpani },
};

// All the kittens safe: four bars, in D major for once
#define Q 40
#define VEL 100
#define BASS_VEL 95
#define PAH_VEL 75
static const SeqStep_t _end_lead[] = {
	PROG(GM_HARPSICHORD), CC(CC_EXPRESSION, 100),
	M(D5, 1), M(FS5, 1), M(A5, 1),
	M(B5, 1), M(A5, 1), M(FS5, 1),
	M(G5, 1), M(E5, 1), M(CS5, 1),
	M(D5, 3),
	END(0)
};
static const SeqStep_t _end_bass[] = {
	PROG(GM_TUBA), CC(CC_EXPRESSION, 100),
	OOM(38), OOM(43), OOM(45), NOTE(38, 3 * Q, BASS_VEL),
	END(0)
};
static const SeqStep_t _end_pah[] = {
	PROG(GM_PIZZICATO_STRINGS), CC(CC_EXPRESSION, 90),
	PAH(57, 62, 66), PAH(59, 62, 67), PAH(57, 61, 67), CHORD(57, 62, 66, 3 * Q, PAH_VEL),
	END(0)
};
static const SeqTrack_t _night_end[] = {
	{ 0, _end_lead },
	{ 1, _end_pah },
	{ 2, _end_bass },
};
#undef Q
#undef VEL
#undef BASS_VEL
#undef PAH_VEL

// Game over: a sad trombone, wobbling at the end
static const SeqStep_t _over_trombone[] = {
	PROG(GM_TROMBONE), CC(CC_EXPRESSION, 110), CC(CC_MODULATION, 0),
	NOTE(55, 50, 100), NOTE(54, 50, 100), NOTE(53, 50, 100),
	ON(0, 52, 105), CC_AFTER(30, CC_MODULATION, 110), OFF(170, 52), CC(CC_MODULATION, 0),
	END(0)
};
static const SeqStep_t _over_tuba[] = {
	PROG(GM_TUBA), CC(CC_EXPRESSION, 90),
	NOTE(43, 50, 80), NOTE(42, 50, 80), NOTE(41, 50, 80), NOTE(40, 200, 90),
	END(0)
};
static const SeqTrack_t _over[] = {
	{ 0, _over_trombone },
	{ 2, _over_tuba },
};

// A new high score: a skeleton's dance on the xylophone, quicker
#define Q 36
#define VEL 100
#define BASS_VEL 95
#define PAH_VEL 70
static const SeqStep_t _hiscore_lead[] = {
	PROG(GM_XYLOPHONE), CC(CC_EXPRESSION, 100),
	M(D5, 1), M(F5, 1), M(A5, 1),
	M(G5, 1), M(E5, 1), M(C5, 1),
	M(F5, 1), M(D5, 1), M(AS4, 1),
	M(CS5, 1), M(E5, 1), M(A5, 1),
	M(D6, 1), M(A5, 1), M(F5, 1),
	M(G5, 1), M(AS5, 1), M(D5, 1),
	M(E5, 1), M(CS5, 1), M(A4, 1),
	M(D5, 3),
	END(0)
};
static const SeqStep_t _hiscore_bass[] = {
	PROG(GM_TUBA), CC(CC_EXPRESSION, 100),
	OOM(38), OOM(48), OOM(46), OOM(45), OOM(38), OOM(43), OOM(45), OOM(38),
	END(0)
};
static const SeqStep_t _hiscore_pah[] = {
	PROG(GM_PIZZICATO_STRINGS), CC(CC_EXPRESSION, 90),
	DM, CM, BB, A7, DM, GM, A7, DM,
	END(0)
};
static const SeqStep_t _hiscore_drums[] = {
	CLOP, CLOP, CLOP, CLOP, CLOP, CLOP, CLOP, CLOP,
	END(0)
};
static const SeqTrack_t _hiscore[] = {
	{ 0, _hiscore_lead },
	{ 1, _hiscore_pah },
	{ 2, _hiscore_bass },
	{ GM_DRUM_CHANNEL, _hiscore_drums },
};
#undef Q
#undef VEL
#undef BASS_VEL
#undef PAH_VEL

const SeqTune_t mm_tune_data[MM_TUNES] = {
	[MM_TUNE_TITLE] = { _title, 4, true },
	[MM_TUNE_PLAY] = { _play, 3, true },
	[MM_TUNE_NIGHT] = { _night, 3, false },
	[MM_TUNE_NIGHT_END] = { _night_end, 3, false },
	[MM_TUNE_GAME_OVER] = { _over, 2, false },
	[MM_TUNE_HISCORE] = { _hiscore, 4, true },
};
