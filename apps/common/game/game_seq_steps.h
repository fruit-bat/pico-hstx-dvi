#pragma once

// Writing the sequencer's steps (game_seq.h) by hand: short names for each
// step, and General MIDI's instruments and drums. Include it only where the
// steps are written, as the names are short.

#include "game_seq.h"

#define PROG(p) { 0, SEQ_PROGRAM, p, 0 }
#define CC(c, v) { 0, SEQ_CC, c, v }
#define CC_AFTER(w, c, v) { w, SEQ_CC, c, v }
#define ON(w, n, v) { w, SEQ_ON, n, v }
#define OFF(w, n) { w, SEQ_OFF, n, 0 }
#define BEND(b) { 0, SEQ_BEND, b, 0 }
#define SWEEP(w, to, t) { w, SEQ_SWEEP, to, t }
#define REST(w) { w, SEQ_REST, 0, 0 }
#define MARK { 0, SEQ_MARK, 0, 0 }
#define LOOP(w) { w, SEQ_LOOP, 0, 0 }
#define END(w) { w, SEQ_END, 0, 0 }

// A note d ticks long, let go a little before, so the same again is heard
#define NOTE(n, d, v) ON(0, n, v), OFF((d) - 4, n), REST(4)
// A note after w ticks, let go d ticks later, nothing else between
#define PLAY(w, n, d, v) ON(w, n, v), OFF(d, n)
// Three notes together, d ticks long
#define CHORD(a, b, c, d, v) ON(0, a, v), ON(0, b, v), ON(0, c, v), OFF(d, a), OFF(0, b), OFF(0, c)

// Controls
#define CC_MODULATION 1
#define CC_PAN 10
#define CC_EXPRESSION 11
#define CC_REVERB 91
#define CC_CHORUS 93

// General MIDI's instruments, from 0
#define GM_ACOUSTIC_GRAND_PIANO 0
#define GM_BRIGHT_ACOUSTIC_PIANO 1
#define GM_ELECTRIC_GRAND_PIANO 2
#define GM_HONKY_TONK_PIANO 3
#define GM_ELECTRIC_PIANO_1 4
#define GM_ELECTRIC_PIANO_2 5
#define GM_HARPSICHORD 6
#define GM_CLAVINET 7
#define GM_CELESTA 8
#define GM_GLOCKENSPIEL 9
#define GM_MUSIC_BOX 10
#define GM_VIBRAPHONE 11
#define GM_MARIMBA 12
#define GM_XYLOPHONE 13
#define GM_TUBULAR_BELLS 14
#define GM_DULCIMER 15
#define GM_DRAWBAR_ORGAN 16
#define GM_PERCUSSIVE_ORGAN 17
#define GM_ROCK_ORGAN 18
#define GM_CHURCH_ORGAN 19
#define GM_REED_ORGAN 20
#define GM_ACCORDION 21
#define GM_HARMONICA 22
#define GM_TANGO_ACCORDION 23
#define GM_NYLON_GUITAR 24
#define GM_STEEL_GUITAR 25
#define GM_JAZZ_GUITAR 26
#define GM_CLEAN_GUITAR 27
#define GM_MUTED_GUITAR 28
#define GM_OVERDRIVEN_GUITAR 29
#define GM_DISTORTION_GUITAR 30
#define GM_GUITAR_HARMONICS 31
#define GM_ACOUSTIC_BASS 32
#define GM_FINGER_BASS 33
#define GM_PICK_BASS 34
#define GM_FRETLESS_BASS 35
#define GM_SLAP_BASS_1 36
#define GM_SLAP_BASS_2 37
#define GM_SYNTH_BASS_1 38
#define GM_SYNTH_BASS_2 39
#define GM_VIOLIN 40
#define GM_VIOLA 41
#define GM_CELLO 42
#define GM_CONTRABASS 43
#define GM_TREMOLO_STRINGS 44
#define GM_PIZZICATO_STRINGS 45
#define GM_ORCHESTRAL_HARP 46
#define GM_TIMPANI 47
#define GM_STRING_ENSEMBLE_1 48
#define GM_STRING_ENSEMBLE_2 49
#define GM_SYNTH_STRINGS_1 50
#define GM_SYNTH_STRINGS_2 51
#define GM_CHOIR_AAHS 52
#define GM_VOICE_OOHS 53
#define GM_SYNTH_VOICE 54
#define GM_ORCHESTRA_HIT 55
#define GM_TRUMPET 56
#define GM_TROMBONE 57
#define GM_TUBA 58
#define GM_MUTED_TRUMPET 59
#define GM_FRENCH_HORN 60
#define GM_BRASS_SECTION 61
#define GM_SYNTH_BRASS_1 62
#define GM_SYNTH_BRASS_2 63
#define GM_SOPRANO_SAX 64
#define GM_ALTO_SAX 65
#define GM_TENOR_SAX 66
#define GM_BARITONE_SAX 67
#define GM_OBOE 68
#define GM_ENGLISH_HORN 69
#define GM_BASSOON 70
#define GM_CLARINET 71
#define GM_PICCOLO 72
#define GM_FLUTE 73
#define GM_RECORDER 74
#define GM_PAN_FLUTE 75
#define GM_BLOWN_BOTTLE 76
#define GM_SHAKUHACHI 77
#define GM_WHISTLE 78
#define GM_OCARINA 79
#define GM_SQUARE_LEAD 80
#define GM_SAW_LEAD 81
#define GM_CALLIOPE_LEAD 82
#define GM_CHIFF_LEAD 83
#define GM_CHARANG_LEAD 84
#define GM_VOICE_LEAD 85
#define GM_FIFTHS_LEAD 86
#define GM_BASS_LEAD 87
#define GM_NEW_AGE_PAD 88
#define GM_WARM_PAD 89
#define GM_POLYSYNTH_PAD 90
#define GM_CHOIR_PAD 91
#define GM_BOWED_PAD 92
#define GM_METALLIC_PAD 93
#define GM_HALO_PAD 94
#define GM_SWEEP_PAD 95
#define GM_RAIN 96
#define GM_SOUNDTRACK 97
#define GM_CRYSTAL 98
#define GM_ATMOSPHERE 99
#define GM_BRIGHTNESS 100
#define GM_GOBLINS 101
#define GM_ECHOES 102
#define GM_SCI_FI 103
#define GM_SITAR 104
#define GM_BANJO 105
#define GM_SHAMISEN 106
#define GM_KOTO 107
#define GM_KALIMBA 108
#define GM_BAGPIPE 109
#define GM_FIDDLE 110
#define GM_SHANAI 111
#define GM_TINKLE_BELL 112
#define GM_AGOGO 113
#define GM_STEEL_DRUMS 114
#define GM_WOODBLOCK 115
#define GM_TAIKO_DRUM 116
#define GM_MELODIC_TOM 117
#define GM_SYNTH_DRUM 118
#define GM_REVERSE_CYMBAL 119
#define GM_GUITAR_FRET_NOISE 120
#define GM_BREATH_NOISE 121
#define GM_SEASHORE 122
#define GM_BIRD_TWEET 123
#define GM_TELEPHONE_RING 124
#define GM_HELICOPTER 125
#define GM_APPLAUSE 126
#define GM_GUNSHOT 127

// The drums, notes on channel 10 (9 from 0)
#define GM_DRUM_CHANNEL 9
#define GM_DRUM_ACOUSTIC_BASS 35
#define GM_DRUM_BASS 36
#define GM_DRUM_SIDE_STICK 37
#define GM_DRUM_SNARE 38
#define GM_DRUM_HAND_CLAP 39
#define GM_DRUM_ELECTRIC_SNARE 40
#define GM_DRUM_LOW_FLOOR_TOM 41
#define GM_DRUM_CLOSED_HI_HAT 42
#define GM_DRUM_HIGH_FLOOR_TOM 43
#define GM_DRUM_PEDAL_HI_HAT 44
#define GM_DRUM_LOW_TOM 45
#define GM_DRUM_OPEN_HI_HAT 46
#define GM_DRUM_LOW_MID_TOM 47
#define GM_DRUM_HIGH_MID_TOM 48
#define GM_DRUM_CRASH 49
#define GM_DRUM_HIGH_TOM 50
#define GM_DRUM_RIDE 51
#define GM_DRUM_CHINESE_CYMBAL 52
#define GM_DRUM_RIDE_BELL 53
#define GM_DRUM_TAMBOURINE 54
#define GM_DRUM_SPLASH 55
#define GM_DRUM_COWBELL 56
#define GM_DRUM_CRASH_2 57
#define GM_DRUM_VIBRASLAP 58
#define GM_DRUM_RIDE_2 59
#define GM_DRUM_HIGH_BONGO 60
#define GM_DRUM_LOW_BONGO 61
#define GM_DRUM_MUTE_HIGH_CONGA 62
#define GM_DRUM_OPEN_HIGH_CONGA 63
#define GM_DRUM_LOW_CONGA 64
#define GM_DRUM_HIGH_TIMBALE 65
#define GM_DRUM_LOW_TIMBALE 66
#define GM_DRUM_HIGH_AGOGO 67
#define GM_DRUM_LOW_AGOGO 68
#define GM_DRUM_CABASA 69
#define GM_DRUM_MARACAS 70
#define GM_DRUM_SHORT_WHISTLE 71
#define GM_DRUM_LONG_WHISTLE 72
#define GM_DRUM_SHORT_GUIRO 73
#define GM_DRUM_LONG_GUIRO 74
#define GM_DRUM_CLAVES 75
#define GM_DRUM_HIGH_WOOD_BLOCK 76
#define GM_DRUM_LOW_WOOD_BLOCK 77
#define GM_DRUM_MUTE_CUICA 78
#define GM_DRUM_OPEN_CUICA 79
#define GM_DRUM_MUTE_TRIANGLE 80
#define GM_DRUM_OPEN_TRIANGLE 81
