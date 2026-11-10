// Copyright (c) 2024 fruit-bat.

// The sounds and tunes, as steps for the sequencer (game_seq.h), at 120
// ticks a second, two to a frame. Sounds bend two octaves either way, so a
// bend's step is three eighths of a semitone, 64 being none.

#include "ww_sound.h"
#include "game_seq_steps.h"

// ----------------------------------------------------------------------------
// Sounds: a storybook wood, harp, celesta and flutes

// A swish up, sparkling
static const SeqStep_t _cast[] = {
	PROG(GM_CELESTA), BEND(40), ON(0, 84, 85), SWEEP(0, 88, 12), OFF(12, 84), END(6)
};
static const SeqStep_t _friend[] = {
	PROG(GM_GLOCKENSPIEL), PLAY(0, 88, 6, 70), PLAY(0, 93, 10, 70), END(10)
};
static const SeqStep_t _better[] = {
	PROG(GM_VIBRAPHONE), PLAY(0, 76, 8, 90), PLAY(0, 79, 16, 90), END(10)
};
// A harp, up through D major
static const SeqStep_t _happy[] = {
	PROG(GM_ORCHESTRAL_HARP),
	PLAY(0, 62, 5, 100), PLAY(0, 66, 5, 100), PLAY(0, 69, 5, 100), PLAY(0, 74, 5, 100),
	PLAY(0, 78, 5, 100), PLAY(0, 81, 5, 100), PLAY(0, 86, 30, 110), END(20)
};
// Hoo hoo, up
static const SeqStep_t _found[] = {
	PROG(GM_OCARINA), PLAY(0, 81, 10, 90), PLAY(4, 86, 16, 100), END(6)
};
// The harp, then a flute held over it
static const SeqStep_t _reunite[] = {
	PROG(GM_ORCHESTRAL_HARP),
	PLAY(0, 62, 5, 100), PLAY(0, 66, 5, 100), PLAY(0, 69, 5, 100), PLAY(0, 74, 5, 100),
	PLAY(0, 78, 5, 100), PLAY(0, 81, 5, 100), PLAY(0, 86, 5, 100),
	PROG(GM_FLUTE), PLAY(0, 78, 20, 100), PLAY(0, 81, 20, 100), PLAY(0, 86, 60, 110), END(10)
};
// A growl, rising
static const SeqStep_t _wake[] = {
	PROG(GM_TROMBONE), BEND(48), ON(0, 40, 120), SWEEP(0, 68, 30), OFF(30, 40), END(6)
};
static const SeqStep_t _yelp[] = {
	PROG(GM_WHISTLE), BEND(96), ON(0, 84, 100), SWEEP(0, 64, 10), OFF(10, 84), END(2)
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
// Sparkling up two octaves
static const SeqStep_t _piece[] = {
	PROG(GM_CELESTA),
	PLAY(0, 74, 4, 100), PLAY(0, 78, 4, 100), PLAY(0, 81, 4, 100), PLAY(0, 86, 4, 100),
	PLAY(0, 90, 4, 100), PLAY(0, 93, 4, 100), PLAY(0, 98, 4, 100),
	PLAY(4, 93, 4, 90), PLAY(0, 98, 30, 100), END(10)
};
// Bells, the whole charm
static const SeqStep_t _charm[] = {
	PROG(GM_TUBULAR_BELLS), ON(0, 74, 110), ON(10, 78, 100), ON(10, 81, 100), ON(10, 86, 120),
	OFF(120, 74), OFF(0, 78), OFF(0, 81), OFF(0, 86), END(30)
};
static const SeqStep_t _flower[] = {
	PROG(GM_MUSIC_BOX), PLAY(0, 84, 6, 100), PLAY(0, 88, 6, 100), PLAY(0, 91, 16, 100), END(10)
};
// Wobbling
static const SeqStep_t _muddle[] = {
	PROG(GM_VIBRAPHONE), BEND(64), ON(0, 79, 100),
	SWEEP(0, 80, 6), SWEEP(6, 48, 6), SWEEP(6, 80, 6), SWEEP(6, 48, 6), SWEEP(6, 64, 6),
	OFF(6, 79), END(10)
};
// Awooo: up a little, then a long way down
static const SeqStep_t _howl[] = {
	PROG(GM_SYNTH_VOICE), BEND(52), ON(0, 69, 110), SWEEP(0, 72, 30), SWEEP(60, 40, 90),
	OFF(90, 69), END(10)
};
// Low tremolo strings, rubbing
static const SeqStep_t _wolf[] = {
	PROG(GM_TREMOLO_STRINGS), ON(0, 38, 110), ON(0, 39, 100), OFF(90, 38), OFF(0, 39), END(20)
};
// Tiptoeing up the pizzicato
static const SeqStep_t _sneak[] = {
	PROG(GM_PIZZICATO_STRINGS),
	PLAY(0, 67, 4, 80), PLAY(6, 71, 4, 80), PLAY(6, 74, 4, 80), PLAY(6, 79, 8, 100), END(10)
};
static const SeqStep_t _extra_life[] = {
	PROG(GM_GLOCKENSPIEL),
	PLAY(0, 79, 6, 100), PLAY(2, 84, 6, 100), PLAY(2, 88, 6, 100), PLAY(2, 91, 6, 100),
	PLAY(2, 88, 6, 100), PLAY(2, 91, 6, 100), PLAY(2, 96, 30, 110), END(10)
};
static const SeqStep_t _bonus[] = {
	PROG(GM_GLOCKENSPIEL), PLAY(0, 91, 10, 90), END(10)
};

const SeqSound_t ww_sound_data[WW_SFX_SOUNDS] = {
	[WW_SFX_CAST] = { _cast, 3, SEQ_LEVEL_NONE },
	[WW_SFX_FRIEND] = { _friend, 4, SEQ_LEVEL_NONE },
	[WW_SFX_BETTER] = { _better, 5, SEQ_LEVEL_NONE },
	[WW_SFX_HAPPY] = { _happy, 6, SEQ_LEVEL_NONE },
	[WW_SFX_FOUND] = { _found, 6, SEQ_LEVEL_NONE },
	[WW_SFX_REUNITE] = { _reunite, 7, SEQ_LEVEL_NONE },
	[WW_SFX_WAKE] = { _wake, 6, SEQ_LEVEL_NONE },
	[WW_SFX_YELP] = { _yelp, 6, SEQ_LEVEL_NONE },
	[WW_SFX_HURT] = { _hurt, 7, SEQ_LEVEL_NONE },
	[WW_SFX_DIZZY] = { _dizzy, 9, SEQ_LEVEL_NONE },
	[WW_SFX_PIECE] = { _piece, 8, SEQ_LEVEL_NONE },
	[WW_SFX_CHARM] = { _charm, 9, SEQ_LEVEL_NONE },
	[WW_SFX_FLOWER] = { _flower, 5, SEQ_LEVEL_NONE },
	[WW_SFX_MUDDLE] = { _muddle, 5, SEQ_LEVEL_NONE },
	[WW_SFX_HOWL] = { _howl, 8, SEQ_LEVEL_NONE },
	[WW_SFX_WOLF] = { _wolf, 7, SEQ_LEVEL_NONE },
	[WW_SFX_SNEAK] = { _sneak, 5, SEQ_LEVEL_NONE },
	[WW_SFX_EXTRA_LIFE] = { _extra_life, 8, SEQ_LEVEL_NONE },
	[WW_SFX_BONUS] = { _bonus, 5, SEQ_LEVEL_NONE },
};

// ----------------------------------------------------------------------------
// Tunes: our own, folk in 6/8, a flute or recorder over a harp, pizzicato
// strings for the bass

// A note e eighths long, the eighth E ticks, set before each tune
#define L(n, e) NOTE(n, (e) * E, VEL)
#define R(e) REST((e) * E)
// The harp, a bar of a chord broken up and down
#define H(n) NOTE(n, E, HARP_VEL)
#define HARP(a, b, c, d) H(a), H(b), H(c), H(d), H(c), H(b)
// The bass, a bar of the root then the fifth
#define BASS(r) NOTE(r, 3 * E, VEL), NOTE((r) + 7, 3 * E, VEL - 10)
// Percussion, a bar: a bongo on the first beat, tambourine on the second,
// maracas throughout
#define SHAKE ON(0, GM_DRUM_MARACAS, 40), OFF(4, GM_DRUM_MARACAS), REST(E - 4)
#define SHAKE_WITH(d, v) ON(0, d, v), ON(0, GM_DRUM_MARACAS, 40), OFF(4, d), OFF(0, GM_DRUM_MARACAS), REST(E - 4)
#define FOLK SHAKE_WITH(GM_DRUM_LOW_BONGO, 60), SHAKE, SHAKE, SHAKE_WITH(GM_DRUM_TAMBOURINE, 55), SHAKE, SHAKE

// The notes
#define D4 62
#define E4 64
#define FS4 66
#define G4 67
#define A4 69
#define B4 71
#define C5 72
#define CS5 73
#define D5 74
#define DS5 75
#define E5 76
#define FS5 78
#define G5 79
#define A5 81
#define B5 83
#define D6 86

// The chords for the harp, and the bass's roots
#define HD HARP(50, 57, 62, 66)
#define HG HARP(43, 50, 55, 59)
#define HA HARP(45, 52, 57, 61)
#define HBM HARP(47, 54, 59, 62)
#define HEM HARP(52, 59, 64, 67)
#define HC HARP(48, 55, 60, 64)
#define HAM HARP(45, 52, 57, 60)
#define HB HARP(47, 54, 59, 63)

// The title, in D major, eight bars: D, G, A, D, B minor, G, A, D
#define E 30
#define VEL 95
#define HARP_VEL 75
static const SeqStep_t _title_lead[] = {
	PROG(GM_FLUTE), CC(CC_EXPRESSION, 100),
	L(A4, 2), L(D5, 1), L(FS5, 2), L(E5, 1),
	L(D5, 2), L(B4, 1), L(G4, 3),
	L(A4, 2), L(CS5, 1), L(E5, 2), L(D5, 1),
	L(D5, 3), L(A4, 3),
	L(B4, 2), L(D5, 1), L(FS5, 2), L(A5, 1),
	L(G5, 2), L(FS5, 1), L(E5, 2), L(D5, 1),
	L(E5, 2), L(FS5, 1), L(E5, 2), L(CS5, 1),
	L(D5, 6),
	END(0)
};
static const SeqStep_t _title_harp[] = {
	PROG(GM_ORCHESTRAL_HARP), CC(CC_EXPRESSION, 85),
	HD, HG, HA, HD, HBM, HG, HA, HD,
	END(0)
};
static const SeqStep_t _title_bass[] = {
	PROG(GM_PIZZICATO_STRINGS), CC(CC_EXPRESSION, 100),
	BASS(38), BASS(43), BASS(45), BASS(38), BASS(47), BASS(43), BASS(45), BASS(38),
	END(0)
};
static const SeqStep_t _title_drums[] = {
	FOLK, FOLK, FOLK, FOLK, FOLK, FOLK, FOLK, FOLK,
	END(0)
};
static const SeqTrack_t _title[] = {
	{ 0, _title_lead },
	{ 1, _title_harp },
	{ 2, _title_bass },
	{ GM_DRUM_CHANNEL, _title_drums },
};
#undef E
#undef VEL
#undef HARP_VEL

// While exploring: slower and quiet, in E minor, sixteen bars, a recorder
// over the harp
#define E 36
#define VEL 75
#define HARP_VEL 60
static const SeqStep_t _play_lead[] = {
	PROG(GM_RECORDER), CC(CC_EXPRESSION, 70),
	L(B4, 3), L(E5, 2), L(D5, 1),
	L(C5, 2), L(B4, 1), L(G4, 3),
	L(A4, 2), L(FS4, 1), L(D5, 3),
	L(B4, 3), R(3),
	L(E5, 2), L(G5, 1), L(FS5, 2), L(E5, 1),
	L(E5, 2), L(D5, 1), L(C5, 3),
	L(C5, 2), L(B4, 1), L(A4, 2), L(C5, 1),
	L(B4, 3), L(FS4, 3),
	L(G4, 2), L(A4, 1), L(C5, 2), L(E5, 1),
	L(D5, 3), L(B4, 3),
	L(A4, 2), L(FS4, 1), L(A4, 2), L(D5, 1),
	L(E5, 3), R(3),
	L(G5, 2), L(E5, 1), L(C5, 2), L(E5, 1),
	L(D5, 2), L(B4, 1), L(G4, 3),
	L(A4, 2), L(C5, 1), L(E5, 2), L(D5, 1),
	L(DS5, 3), L(B4, 3),
	END(0)
};
static const SeqStep_t _play_harp[] = {
	PROG(GM_ORCHESTRAL_HARP), CC(CC_EXPRESSION, 70),
	HEM, HC, HD, HEM, HEM, HC, HAM, HB,
	HC, HG, HD, HEM, HC, HG, HAM, HB,
	END(0)
};
static const SeqStep_t _play_bass[] = {
	PROG(GM_PIZZICATO_STRINGS), CC(CC_EXPRESSION, 70),
	BASS(40), BASS(36), BASS(38), BASS(40), BASS(40), BASS(36), BASS(45), BASS(47),
	BASS(36), BASS(43), BASS(38), BASS(40), BASS(36), BASS(43), BASS(45), BASS(47),
	END(0)
};
static const SeqTrack_t _play[] = {
	{ 0, _play_lead },
	{ 1, _play_harp },
	{ 2, _play_bass },
};
#undef E
#undef VEL
#undef HARP_VEL

// A night starting: the harp up two octaves, a flute and strings
static const SeqStep_t _night_harp[] = {
	PROG(GM_ORCHESTRAL_HARP), CC(CC_EXPRESSION, 100),
	NOTE(D4, 8, 90), NOTE(FS4, 8, 90), NOTE(A4, 8, 90), NOTE(D5, 8, 95),
	NOTE(FS5, 8, 95), NOTE(A5, 8, 100), NOTE(D6, 8, 100),
	CHORD(D4, FS4, A4, 200, 80),
	END(0)
};
static const SeqStep_t _night_flute[] = {
	PROG(GM_FLUTE), CC(CC_EXPRESSION, 100),
	REST(56), NOTE(FS5, 30, 90), NOTE(A5, 30, 95), NOTE(D6, 150, 100),
	END(0)
};
static const SeqStep_t _night_strings[] = {
	PROG(GM_STRING_ENSEMBLE_1), CC(CC_EXPRESSION, 80),
	REST(56), CHORD(50, 57, 62, 200, 70),
	END(0)
};
static const SeqTrack_t _night[] = {
	{ 0, _night_flute },
	{ 1, _night_harp },
	{ 3, _night_strings },
};

// The wolf calmed: four bars in D, a little quicker
#define E 24
#define VEL 100
#define HARP_VEL 80
static const SeqStep_t _end_lead[] = {
	PROG(GM_FLUTE), CC(CC_EXPRESSION, 100),
	L(FS5, 2), L(E5, 1), L(D5, 2), L(A4, 1),
	L(B4, 2), L(D5, 1), L(G5, 2), L(FS5, 1),
	L(E5, 2), L(CS5, 1), L(A4, 2), L(CS5, 1),
	L(D5, 6),
	END(0)
};
static const SeqStep_t _end_harp[] = {
	PROG(GM_ORCHESTRAL_HARP), CC(CC_EXPRESSION, 85),
	HD, HG, HA, HD,
	END(0)
};
static const SeqStep_t _end_bass[] = {
	PROG(GM_PIZZICATO_STRINGS), CC(CC_EXPRESSION, 100),
	BASS(38), BASS(43), BASS(45), NOTE(38, 6 * E, VEL),
	END(0)
};
static const SeqStep_t _end_drums[] = {
	FOLK, FOLK, FOLK, SHAKE_WITH(GM_DRUM_TAMBOURINE, 70),
	END(0)
};
static const SeqTrack_t _night_end[] = {
	{ 0, _end_lead },
	{ 1, _end_harp },
	{ 2, _end_bass },
	{ GM_DRUM_CHANNEL, _end_drums },
};
#undef E
#undef VEL
#undef HARP_VEL

// Game over, sweet dreams: a music box lullaby, down to D, over strings
#define E 25
#define VEL 90
static const SeqStep_t _over_lead[] = {
	PROG(GM_MUSIC_BOX), CC(CC_EXPRESSION, 100),
	L(A5, 2), L(FS5, 1), L(G5, 2), L(E5, 1), L(FS5, 2), L(D5, 1), L(E5, 2), L(CS5, 1),
	L(D5, 6),
	END(0)
};
static const SeqStep_t _over_pad[] = {
	PROG(GM_STRING_ENSEMBLE_1), CC(CC_EXPRESSION, 70),
	CHORD(62, 66, 69, 3 * E, 60), CHORD(61, 64, 67, 3 * E, 60), CHORD(62, 66, 69, 3 * E, 60),
	CHORD(61, 64, 69, 3 * E, 60), CHORD(62, 66, 69, 6 * E, 60),
	END(0)
};
static const SeqStep_t _over_bass[] = {
	PROG(GM_PIZZICATO_STRINGS), CC(CC_EXPRESSION, 90),
	NOTE(38, 3 * E, 90), NOTE(45, 3 * E, 90), NOTE(38, 3 * E, 90), NOTE(45, 3 * E, 90), NOTE(38, 6 * E, 90),
	END(0)
};
static const SeqTrack_t _over[] = {
	{ 0, _over_lead },
	{ 1, _over_pad },
	{ 2, _over_bass },
};
#undef E
#undef VEL

// A new high score: a jig on the fiddle, with an accordion
#define E 20
#define VEL 95
#define J(n) NOTE(n, E, VEL)
#define SQUEEZE(a, b, c) CHORD(a, b, c, 3 * E - 4, 70), REST(4)
static const SeqStep_t _hiscore_lead[] = {
	PROG(GM_FIDDLE), CC(CC_EXPRESSION, 95),
	J(A4), J(D5), J(FS5), J(A5), J(FS5), J(D5),
	J(B4), J(D5), J(G5), J(B5), J(G5), J(D5),
	J(CS5), J(E5), J(A5), J(G5), J(E5), J(CS5),
	L(D5, 2), L(A4, 1), L(D5, 3),
	END(0)
};
static const SeqStep_t _hiscore_chords[] = {
	PROG(GM_ACCORDION), CC(CC_EXPRESSION, 75),
	SQUEEZE(62, 66, 69), SQUEEZE(62, 66, 69), SQUEEZE(62, 67, 71), SQUEEZE(62, 67, 71),
	SQUEEZE(61, 64, 69), SQUEEZE(61, 64, 69), SQUEEZE(62, 66, 69), SQUEEZE(62, 66, 69),
	END(0)
};
static const SeqStep_t _hiscore_bass[] = {
	PROG(GM_PIZZICATO_STRINGS), CC(CC_EXPRESSION, 95),
	BASS(38), BASS(43), BASS(45), BASS(38),
	END(0)
};
static const SeqStep_t _hiscore_drums[] = {
	FOLK, FOLK, FOLK, FOLK,
	END(0)
};
static const SeqTrack_t _hiscore[] = {
	{ 0, _hiscore_lead },
	{ 1, _hiscore_chords },
	{ 2, _hiscore_bass },
	{ GM_DRUM_CHANNEL, _hiscore_drums },
};
#undef E
#undef VEL

const SeqTune_t ww_tune_data[WW_TUNES] = {
	[WW_TUNE_TITLE] = { _title, 4, true },
	[WW_TUNE_PLAY] = { _play, 3, true },
	[WW_TUNE_NIGHT] = { _night, 3, false },
	[WW_TUNE_NIGHT_END] = { _night_end, 4, false },
	[WW_TUNE_GAME_OVER] = { _over, 3, false },
	[WW_TUNE_HISCORE] = { _hiscore, 4, true },
};
