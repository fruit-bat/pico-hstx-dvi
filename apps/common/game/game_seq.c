// See game_seq.h.
#include "game_seq.h"
#include "game_synth.h"
#include <stddef.h>

#ifdef GAME_HOST
// On the PC the ticks come from the game, between its calls
#define LOCK()
#define UNLOCK()
#else
#include "pico/time.h"
#include "hardware/sync.h"
// On the Pico they come from a timer's interrupt
#define LOCK() const uint32_t _irq = save_and_disable_interrupts()
#define UNLOCK() restore_interrupts(_irq)
#endif

#define TRACKS 10
#define SOUNDS 6
#define SOUND_CHANNEL 10
// How far a sound bends either way, in semitones
#define SOUND_BEND_RANGE 24
#define BEND_NONE 8192
// A tune's level, while it fades
#define LEVEL_FULL 1024u
// No more steps than this in one tick, in case of a loop with no waits
#define STEPS_A_TICK 64

#define NOTE_ON 0x90
#define NOTE_OFF 0x80
#define CONTROL 0xb0
#define PROGRAM 0xc0
#define BEND 0xe0
#define CC_VOLUME 7
#define CC_EXPRESSION 11
#define CC_RESET 121
#define CC_NOTES_OFF 123

// Steps being played on a channel
typedef struct {
	const SeqStep_t* at; // the next, or NULL when done
	const SeqStep_t* mark;
	uint32_t wait;
	uint8_t channel;
	uint16_t bend, sent_bend, bend_to;
	uint32_t sweep; // ticks left bending to bend_to
} Player_t;

typedef struct {
	Player_t p;
	const SeqSound_t* s; // or NULL when the channel is free
	uint32_t id;
	uint32_t age;
	uint8_t level;
} Sound_t;

static Sound_t _sounds[SOUNDS];
static uint32_t _age;

static Player_t _tracks[TRACKS];
static const SeqTune_t* _tune;
static const SeqTune_t* _next;
static uint32_t _level = LEVEL_FULL;
// Fading out by this much a tick, or not if 0
static uint32_t _fade;
static uint8_t _sent_volume[TRACKS];

static uint8_t _music = 100, _sound = 100;

static void send(uint8_t status, uint8_t a, uint8_t b) {
	game_synth_send(status, a, b);
}

static void send_bend(Player_t* p) {
	if (p->bend == p->sent_bend) return;
	send(BEND | p->channel, p->bend & 0x7f, p->bend >> 7);
	p->sent_bend = p->bend;
}

static void player_start(Player_t* p, const SeqStep_t* steps, uint8_t channel) {
	p->at = p->mark = steps;
	p->wait = steps->wait;
	p->channel = channel;
	p->bend = BEND_NONE;
	// Sent, to be sure
	p->sent_bend = 0;
	p->sweep = 0;
	send_bend(p);
}

static void player_tick(Player_t* p) {
	if (!p->at) return;
	if (p->sweep) {
		p->bend = (uint16_t)((int32_t)p->bend + ((int32_t)p->bend_to - (int32_t)p->bend) / (int32_t)p->sweep);
		--p->sweep;
		send_bend(p);
	}
	const uint8_t c = p->channel;
	for (uint32_t n = 0; p->at && !p->wait && n < STEPS_A_TICK; ++n) {
		const SeqStep_t* s = p->at++;
		switch (s->op) {
			case SEQ_END: p->at = NULL; break;
			case SEQ_ON: send(NOTE_ON | c, s->a, s->b); break;
			case SEQ_OFF: send(NOTE_OFF | c, s->a, 0); break;
			case SEQ_PROGRAM: send(PROGRAM | c, s->a, 0); break;
			// The volume is the sequencer's, for fading
			case SEQ_CC: if (s->a != CC_VOLUME) send(CONTROL | c, s->a, s->b); break;
			case SEQ_BEND:
				p->bend = (uint16_t)(s->a << 7);
				p->sweep = 0;
				send_bend(p);
				break;
			case SEQ_SWEEP:
				p->bend_to = (uint16_t)(s->a << 7);
				p->sweep = s->b ? s->b : 1;
				break;
			case SEQ_MARK: p->mark = p->at; break;
			case SEQ_LOOP: p->at = p->mark; break;
			default: break;
		}
		if (p->at) p->wait = p->at->wait;
	}
	if (p->wait) --p->wait;
}

static void notes_off(uint8_t channel) {
	send(CONTROL | channel, CC_NOTES_OFF, 0);
}

// Sounds

static Sound_t* sound_of(uint32_t id) {
	for (uint32_t i = 0; i < SOUNDS; ++i) {
		if (_sounds[i].s && _sounds[i].id == id) return &_sounds[i];
	}
	return NULL;
}

// Its own channel, else a free one, else that of the oldest of the lowest
// priority, if no higher than its
static Sound_t* sound_channel(const SeqSound_t* s, uint32_t id) {
	Sound_t* c = sound_of(id);
	if (c) return c;
	for (uint32_t i = 0; i < SOUNDS; ++i) {
		if (!_sounds[i].s) return &_sounds[i];
	}
	for (uint32_t i = 0; i < SOUNDS; ++i) {
		Sound_t* t = &_sounds[i];
		if (t->s->priority > s->priority) continue;
		if (!c || t->s->priority < c->s->priority || (t->s->priority == c->s->priority && t->age < c->age)) c = t;
	}
	return c;
}

void game_seq_sound(const SeqSound_t* s, uint32_t id) {
	LOCK();
	Sound_t* c = sound_channel(s, id);
	if (c) {
		const uint8_t channel = (uint8_t)(SOUND_CHANNEL + (c - _sounds));
		notes_off(channel);
		send(CONTROL | channel, CC_EXPRESSION, 127);
		c->s = s;
		c->id = id;
		c->age = ++_age;
		c->level = 255;
		player_start(&c->p, s->steps, channel);
	}
	UNLOCK();
}

void game_seq_sound_stop(uint32_t id) {
	LOCK();
	Sound_t* c = sound_of(id);
	if (c) {
		notes_off(c->p.channel);
		c->s = NULL;
	}
	UNLOCK();
}

bool game_seq_sound_playing(uint32_t id) {
	LOCK();
	const bool playing = sound_of(id) != NULL;
	UNLOCK();
	return playing;
}

void game_seq_sound_level(uint32_t id, uint8_t level) {
	LOCK();
	Sound_t* c = sound_of(id);
	if (c && level != c->level) {
		c->level = level;
		if (c->s->level == SEQ_LEVEL_BEND) {
			c->p.bend = (uint16_t)(level << 7);
			c->p.sweep = 0;
			send_bend(&c->p);
		}
		else if (c->s->level == SEQ_LEVEL_VOLUME) {
			send(CONTROL | c->p.channel, CC_EXPRESSION, level);
		}
	}
	UNLOCK();
}

// Tunes

static void send_volumes(void) {
	if (!_tune) return;
	const uint8_t v = (uint8_t)(_music * _level / LEVEL_FULL);
	for (uint32_t i = 0; i < _tune->n; ++i) {
		const uint8_t c = _tune->tracks[i].channel;
		if (_sent_volume[c] == v) continue;
		send(CONTROL | c, CC_VOLUME, v);
		_sent_volume[c] = v;
	}
}

static void tune_begin(const SeqTune_t* t) {
	_tune = t;
	_fade = 0;
	_level = LEVEL_FULL;
	for (uint32_t i = 0; i < t->n; ++i) {
		const uint8_t c = t->tracks[i].channel;
		send(CONTROL | c, CC_RESET, 0);
		_sent_volume[c] = 255;
		player_start(&_tracks[i], t->tracks[i].steps, c);
	}
	send_volumes();
}

static void tune_end(void) {
	if (!_tune) return;
	for (uint32_t i = 0; i < _tune->n; ++i) notes_off(_tune->tracks[i].channel);
	_tune = NULL;
	_fade = 0;
	if (_next) {
		const SeqTune_t* t = _next;
		_next = NULL;
		tune_begin(t);
	}
}

static void fade_out(uint32_t fade) {
	if (!fade) {
		tune_end();
		return;
	}
	_fade = LEVEL_FULL / fade;
	if (!_fade) _fade = 1;
}

void game_seq_tune(const SeqTune_t* t, uint32_t fade) {
	LOCK();
	if (_tune == t) {
		// Back, if fading
		_next = NULL;
		_fade = 0;
		_level = LEVEL_FULL;
		send_volumes();
	}
	else if (!_tune) {
		tune_begin(t);
	}
	else {
		_next = t;
		if (!_fade) fade_out(fade);
	}
	UNLOCK();
}

void game_seq_tune_stop(uint32_t fade) {
	LOCK();
	_next = NULL;
	if (_tune && !_fade) fade_out(fade);
	UNLOCK();
}

void game_seq_volume(uint8_t music, uint8_t sounds) {
	LOCK();
	_music = music;
	_sound = sounds;
	send_volumes();
	for (uint32_t i = 0; i < SOUNDS; ++i) send(CONTROL | (SOUND_CHANNEL + i), CC_VOLUME, _sound);
	UNLOCK();
}

void game_seq_tick(void) {
	game_synth_tick();
	for (uint32_t i = 0; i < SOUNDS; ++i) {
		Sound_t* c = &_sounds[i];
		if (!c->s) continue;
		player_tick(&c->p);
		if (!c->p.at) {
			notes_off(c->p.channel);
			c->s = NULL;
		}
	}
	if (!_tune) return;
	if (_fade) {
		_level = _level > _fade ? _level - _fade : 0;
		send_volumes();
		if (!_level) {
			tune_end();
			return;
		}
	}
	bool playing = false;
	for (uint32_t i = 0; i < _tune->n; ++i) {
		player_tick(&_tracks[i]);
		playing |= _tracks[i].at != NULL;
	}
	if (playing) return;
	if (_tune->loop) tune_begin(_tune);
	else tune_end();
}

#ifndef GAME_HOST
static repeating_timer_t _timer;

static bool on_timer(repeating_timer_t* t) {
	(void)t;
	game_seq_tick();
	return true;
}
#endif

void game_seq_init(void) {
	// General MIDI on, which resets everything
	static const uint8_t gm_on[] = { 0xf0, 0x7e, 0x7f, 0x09, 0x01, 0xf7 };
	game_synth_sysex(gm_on, sizeof(gm_on));
	for (uint32_t i = 0; i < SOUNDS; ++i) {
		const uint8_t c = (uint8_t)(SOUND_CHANNEL + i);
		// The bend range, by registered parameter 0, then no parameter
		send(CONTROL | c, 101, 0);
		send(CONTROL | c, 100, 0);
		send(CONTROL | c, 6, SOUND_BEND_RANGE);
		send(CONTROL | c, 38, 0);
		send(CONTROL | c, 101, 127);
		send(CONTROL | c, 100, 127);
		send(CONTROL | c, CC_VOLUME, _sound);
	}
#ifndef GAME_HOST
	add_repeating_timer_us(-(1000000 / SEQ_HZ), on_timer, NULL, &_timer);
#endif
}
