// Copyright (c) 2024 fruit-bat.

// The sounds and tunes, as steps for the sequencer (game_seq.h), at 120
// ticks a second, two to a frame. Sounds bend two octaves either way, so a
// bend's step is three eighths of a semitone, 64 being none.

#include "def_sound.h"
#include "game_seq_steps.h"

// ----------------------------------------------------------------------------
// Sounds

// A zap, down, short enough for the next of a burst, four frames on
static const SeqStep_t _laser[] = {
	PROG(GM_SAW_LEAD), BEND(110), ON(0, 84, 90), SWEEP(0, 40, 7), OFF(7, 84), END(1)
};
static const SeqStep_t _explode[] = {
	PROG(GM_GUNSHOT), BEND(72), ON(0, 40, 120), SWEEP(0, 40, 40), ON(4, 33, 100), OFF(40, 40), OFF(0, 33), END(6)
};
static const SeqStep_t _enemy_fire[] = {
	PROG(GM_SQUARE_LEAD), BEND(64), ON(0, 79, 55), SWEEP(0, 88, 6), OFF(6, 79), END(1)
};
// Up as long as they take to appear, 40 frames
static const SeqStep_t _spawn[] = {
	PROG(GM_SQUARE_LEAD), BEND(16), ON(0, 60, 45), SWEEP(0, 112, 80), OFF(80, 60), END(2)
};
// A warble, an alarm
static const SeqStep_t _grab[] = {
	PROG(GM_SQUARE_LEAD), BEND(64), ON(0, 76, 90),
	SWEEP(0, 88, 5), SWEEP(5, 64, 5), SWEEP(5, 88, 5), SWEEP(5, 64, 5),
	SWEEP(5, 88, 5), SWEEP(5, 64, 5), SWEEP(5, 88, 5), SWEEP(5, 64, 5),
	OFF(5, 76), END(2)
};
// A whistle, down, as it falls
static const SeqStep_t _fall[] = {
	PROG(GM_WHISTLE), BEND(112), ON(0, 84, 80), SWEEP(0, 16, 100), OFF(100, 84), END(4)
};
static const SeqStep_t _catch[] = {
	PROG(GM_VIBRAPHONE), ON(0, 79, 100), OFF(8, 79), ON(0, 84, 100), OFF(16, 84), END(10)
};
static const SeqStep_t _rescue[] = {
	PROG(GM_GLOCKENSPIEL), ON(0, 72, 90), OFF(8, 72), ON(0, 76, 90), OFF(8, 76),
	ON(0, 79, 90), OFF(8, 79), ON(0, 84, 100), OFF(24, 84), END(10)
};
static const SeqStep_t _human_die[] = {
	PROG(GM_SQUARE_LEAD), BEND(64), ON(0, 64, 90), SWEEP(0, 16, 24), OFF(24, 64), END(2)
};
// A nasty buzz, wobbling
static const SeqStep_t _mutant[] = {
	PROG(GM_SAW_LEAD), BEND(64), ON(0, 45, 110), ON(0, 46, 110),
	SWEEP(0, 56, 3), SWEEP(3, 72, 3), SWEEP(3, 56, 3), SWEEP(3, 72, 3),
	SWEEP(3, 56, 3), SWEEP(3, 72, 3), SWEEP(3, 56, 3), SWEEP(3, 64, 3),
	OFF(3, 45), OFF(0, 46), END(2)
};
// A whoop, twice
static const SeqStep_t _baiter[] = {
	PROG(GM_SAW_LEAD), BEND(32), ON(0, 60, 80), SWEEP(0, 96, 20), BEND(32),
	SWEEP(20, 96, 20), OFF(20, 60), END(2)
};
static const SeqStep_t _smart_bomb[] = {
	PROG(GM_GUNSHOT), BEND(80), ON(0, 36, 127), ON(0, 29, 127), SWEEP(0, 16, 90),
	ON(6, 24, 127), OFF(90, 36), OFF(0, 29), OFF(0, 24), END(10)
};
static const SeqStep_t _hyperspace[] = {
	PROG(GM_SCI_FI), BEND(10), ON(0, 60, 120), SWEEP(0, 127, 50), OFF(60, 60), END(10)
};
static const SeqStep_t _dead[] = {
	PROG(GM_GUNSHOT), BEND(64), ON(0, 30, 127), SWEEP(0, 0, 150), ON(10, 26, 127), ON(16, 22, 127),
	OFF(140, 30), OFF(0, 26), OFF(0, 22), END(20)
};
// Up as the ship forms, 30 frames
static const SeqStep_t _respawn[] = {
	PROG(GM_SQUARE_LEAD), BEND(16), ON(0, 48, 70), ON(0, 55, 70), SWEEP(0, 64, 60),
	OFF(60, 48), OFF(0, 55), END(2)
};
static const SeqStep_t _extra_life[] = {
	PROG(GM_SQUARE_LEAD),
	ON(0, 72, 100), OFF(6, 72), ON(2, 76, 100), OFF(6, 76), ON(2, 79, 100), OFF(6, 79),
	ON(2, 84, 100), OFF(6, 84), ON(2, 79, 100), OFF(6, 79), ON(2, 84, 110), OFF(30, 84),
	END(4)
};
static const SeqStep_t _bonus[] = {
	PROG(GM_GLOCKENSPIEL), ON(0, 91, 90), OFF(10, 91), END(10)
};
// A long rumble, falling
static const SeqStep_t _planet[] = {
	PROG(GM_GUNSHOT), BEND(64), ON(0, 28, 127), SWEEP(0, 0, 240), ON(20, 24, 127), ON(30, 20, 127),
	REST(200), OFF(0, 28), OFF(0, 24), OFF(0, 20), END(30)
};
// Held: a low rush, the loop only waiting
static const SeqStep_t _thrust[] = {
	PROG(GM_BREATH_NOISE), BEND(64), CC(11, 100), ON(0, 40, 80), MARK, LOOP(240)
};

const SeqSound_t def_sound_data[DEF_SFX_SOUNDS] = {
	[DEF_SFX_LASER] = { _laser, 3, SEQ_LEVEL_NONE },
	[DEF_SFX_EXPLODE] = { _explode, 5, SEQ_LEVEL_NONE },
	[DEF_SFX_ENEMY_FIRE] = { _enemy_fire, 2, SEQ_LEVEL_NONE },
	[DEF_SFX_SPAWN] = { _spawn, 1, SEQ_LEVEL_NONE },
	[DEF_SFX_GRAB] = { _grab, 6, SEQ_LEVEL_NONE },
	[DEF_SFX_FALL] = { _fall, 5, SEQ_LEVEL_NONE },
	[DEF_SFX_CATCH] = { _catch, 6, SEQ_LEVEL_NONE },
	[DEF_SFX_RESCUE] = { _rescue, 6, SEQ_LEVEL_NONE },
	[DEF_SFX_HUMAN_DIE] = { _human_die, 6, SEQ_LEVEL_NONE },
	[DEF_SFX_MUTANT] = { _mutant, 6, SEQ_LEVEL_NONE },
	[DEF_SFX_BAITER] = { _baiter, 6, SEQ_LEVEL_NONE },
	[DEF_SFX_SMART_BOMB] = { _smart_bomb, 9, SEQ_LEVEL_NONE },
	[DEF_SFX_HYPERSPACE] = { _hyperspace, 8, SEQ_LEVEL_NONE },
	[DEF_SFX_DEAD] = { _dead, 10, SEQ_LEVEL_NONE },
	[DEF_SFX_RESPAWN] = { _respawn, 7, SEQ_LEVEL_NONE },
	[DEF_SFX_EXTRA_LIFE] = { _extra_life, 8, SEQ_LEVEL_NONE },
	[DEF_SFX_BONUS] = { _bonus, 5, SEQ_LEVEL_NONE },
	[DEF_SFX_PLANET] = { _planet, 10, SEQ_LEVEL_NONE },
	[DEF_SFX_THRUST] = { _thrust, 1, SEQ_LEVEL_NONE },
};

// ----------------------------------------------------------------------------
// Tunes: our own, arcade synth, at 150 beats a minute, a beat Q ticks

#define Q 48
#define E8 (Q / 2)
#define S16 (Q / 4)
#define BAR (4 * Q)

// A note e eighths long
#define L(n, e) NOTE(n, (e) * E8, 100)
// A chord held a bar, let go a little before the next
#define PAD(a, b, c) CHORD(a, b, c, BAR - 8, 70), REST(8)
// A bass in octaves, a bar of eighths
#define OCT(r) NOTE(r, E8, 100), NOTE((r) + 12, E8, 80), NOTE(r, E8, 100), NOTE((r) + 12, E8, 80), \
	NOTE(r, E8, 100), NOTE((r) + 12, E8, 80), NOTE(r, E8, 100), NOTE((r) + 12, E8, 80)

// Drums, an eighth each: one or two hits, or the hi-hat alone
#define KICK GM_DRUM_BASS
#define SNARE GM_DRUM_SNARE
#define HAT GM_DRUM_CLOSED_HI_HAT
#define HIT(a) ON(0, a, 110), ON(0, HAT, 70), OFF(4, a), OFF(0, HAT), REST(E8 - 4)
#define HH ON(0, HAT, 60), OFF(4, HAT), REST(E8 - 4)
#define BEAT HIT(KICK), HH, HIT(SNARE), HH, HIT(KICK), HIT(KICK), HIT(SNARE), HH
// Lighter: the kick on the beat, the hi-hat between
#define LIGHT HIT(KICK), HH, HH, HH, HIT(KICK), HH, HH, HH

// The notes
#define E4 64
#define GS4 68
#define A4 69
#define B4 71
#define C5 72
#define CS5 73
#define D5 74
#define E5 76
#define F5 77
#define G5 79
#define GS5 80
#define A5 81
#define B5 83
#define C6 84

// The title, in A minor: A minor, F, C, G, then E to come round again
static const SeqStep_t _title_lead[] = {
	PROG(GM_SQUARE_LEAD), CC(CC_EXPRESSION, 100),
	L(A4, 2), L(C5, 1), L(E5, 2), L(D5, 1), L(C5, 1), L(B4, 1),
	L(A4, 3), L(E4, 1), L(A4, 2), L(B4, 1), L(C5, 1),
	L(C5, 2), L(F5, 1), L(A5, 2), L(G5, 1), L(F5, 1), L(E5, 1),
	L(F5, 3), L(C5, 1), L(F5, 2), L(E5, 1), L(D5, 1),
	L(E5, 2), L(G5, 1), L(C6, 2), L(B5, 1), L(A5, 1), L(G5, 1),
	L(E5, 3), L(C5, 1), L(E5, 2), L(F5, 1), L(G5, 1),
	L(D5, 2), L(G5, 1), L(B5, 2), L(A5, 1), L(G5, 1), L(F5, 1),
	L(GS4, 1), L(B4, 1), L(E5, 2), L(GS5, 2), L(B5, 2),
	END(0)
};
static const SeqStep_t _title_pad[] = {
	PROG(GM_POLYSYNTH_PAD), CC(CC_EXPRESSION, 80),
	PAD(57, 60, 64), PAD(57, 60, 64), PAD(53, 57, 60), PAD(53, 57, 60),
	PAD(55, 60, 64), PAD(55, 60, 64), PAD(55, 59, 62), PAD(56, 59, 64),
	END(0)
};
static const SeqStep_t _title_bass[] = {
	PROG(GM_SYNTH_BASS_1), CC(CC_EXPRESSION, 100),
	OCT(33), OCT(33), OCT(29), OCT(29), OCT(36), OCT(36), OCT(31), OCT(28),
	END(0)
};
static const SeqStep_t _title_drums[] = {
	BEAT, BEAT, BEAT, BEAT, BEAT, BEAT, BEAT, BEAT,
	END(0)
};
static const SeqTrack_t _title[] = {
	{ 0, _title_lead },
	{ 1, _title_pad },
	{ 2, _title_bass },
	{ GM_DRUM_CHANNEL, _title_drums },
};

// The start of a wave: three quick notes, up, and A major, held
static const SeqStep_t _wave_lead[] = {
	PROG(GM_SYNTH_BRASS_1), CC(CC_EXPRESSION, 110),
	NOTE(E5, S16, 110), NOTE(E5, S16, 110), NOTE(E5, S16, 110), NOTE(A5, 3 * S16, 110),
	NOTE(GS5, S16, 110), NOTE(A5, S16, 110), NOTE(CS5 + 12, 8 * S16, 120),
	END(0)
};
static const SeqStep_t _wave_chord[] = {
	PROG(GM_SYNTH_BRASS_1), CC(CC_EXPRESSION, 90),
	REST(8 * S16), CHORD(57, 61, 64, 8 * S16, 90),
	END(0)
};
static const SeqStep_t _wave_bass[] = {
	PROG(GM_SYNTH_BASS_1), CC(CC_EXPRESSION, 100),
	NOTE(40, 3 * S16, 100), NOTE(45, 5 * S16, 100), NOTE(33, 8 * S16, 110),
	END(0)
};
static const SeqStep_t _wave_drums[] = {
	ON(0, SNARE, 90), OFF(4, SNARE), ON(S16 - 4, SNARE, 100), OFF(4, SNARE), ON(S16 - 4, SNARE, 110), OFF(4, SNARE),
	ON(S16 - 4, KICK, 110), OFF(4, KICK), ON(3 * S16 - 4, KICK, 110), OFF(4, KICK),
	ON(2 * S16 - 4, KICK, 120), ON(0, GM_DRUM_CRASH, 110), OFF(4, KICK), OFF(8 * S16 - 4, GM_DRUM_CRASH),
	END(0)
};
static const SeqTrack_t _wave[] = {
	{ 0, _wave_lead },
	{ 1, _wave_chord },
	{ 2, _wave_bass },
	{ GM_DRUM_CHANNEL, _wave_drums },
};

// Game over: slower, down through A minor, G, F and E to A minor
#define GQ 60
static const SeqStep_t _over_lead[] = {
	PROG(GM_SQUARE_LEAD), CC(CC_EXPRESSION, 100),
	NOTE(E5, GQ, 100), NOTE(D5, GQ, 100), NOTE(C5, GQ, 100), NOTE(B4, GQ, 100), NOTE(A4, 3 * GQ, 100),
	END(0)
};
static const SeqStep_t _over_pad[] = {
	PROG(GM_POLYSYNTH_PAD), CC(CC_EXPRESSION, 80),
	CHORD(57, 60, 64, GQ, 70), CHORD(55, 59, 62, GQ, 70), CHORD(53, 57, 60, GQ, 70),
	CHORD(52, 56, 59, GQ, 70), CHORD(57, 60, 64, 3 * GQ, 70),
	END(0)
};
static const SeqStep_t _over_bass[] = {
	PROG(GM_SYNTH_BASS_1), CC(CC_EXPRESSION, 100),
	NOTE(45, GQ, 100), NOTE(43, GQ, 100), NOTE(41, GQ, 100), NOTE(40, GQ, 100), NOTE(33, 3 * GQ, 110),
	END(0)
};
static const SeqTrack_t _over[] = {
	{ 0, _over_lead },
	{ 1, _over_pad },
	{ 2, _over_bass },
};

// A new high score: lighter, a square arpeggio in sixteenths over C, A minor,
// F and G
#define A16(n) ON(0, n, 70), OFF(S16 - 2, n), REST(2)
#define ARP(a, b, c, d) A16(a), A16(b), A16(c), A16(d), A16(c), A16(b), A16(a), A16(b), \
	A16(a), A16(b), A16(c), A16(d), A16(c), A16(b), A16(a), A16(b)
static const SeqStep_t _hiscore_arp[] = {
	PROG(GM_SQUARE_LEAD), CC(CC_EXPRESSION, 80),
	ARP(60, 64, 67, 72), ARP(57, 60, 64, 69), ARP(53, 57, 60, 65), ARP(55, 59, 62, 67),
	END(0)
};
static const SeqStep_t _hiscore_bass[] = {
	PROG(GM_SYNTH_BASS_1), CC(CC_EXPRESSION, 90),
	OCT(36), OCT(33), OCT(29), OCT(31),
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

const SeqTune_t def_tune_data[DEF_TUNES] = {
	[DEF_TUNE_TITLE] = { _title, 4, true },
	[DEF_TUNE_WAVE] = { _wave, 4, false },
	[DEF_TUNE_GAME_OVER] = { _over, 3, false },
	[DEF_TUNE_HISCORE] = { _hiscore, 3, true },
};
