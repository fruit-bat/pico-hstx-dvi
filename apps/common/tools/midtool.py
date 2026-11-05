#!/usr/bin/env python3
"""MIDI files and the games' sequencer (apps/common/game/game_seq.h).

  midtool.py dump <file.mid> [--channels 0-9]
      Lists the events, a line each, with the time in seconds and channels
      from 1, as a synth would show them.

  midtool.py seq <file.mid> <name> [options]
      Turns a MIDI file into a tune for the sequencer: C, a SeqStep_t array a
      channel, written with game_seq_steps.h's macros, and a SeqTrack_t table,
      at the sequencer's 120 ticks a second.

      --channels 0-9,12   only these channels, from 0 (the default: all)
      --from S --to S     only from and to these seconds; the silence before
                          the first note is left out unless --from is given,
                          and the instruments and controls set in it are set
                          at the first note instead
      --speed X           faster (2) or slower (0.5)
      --grid N            times snapped to every N ticks (e.g. 6, a 32nd at
                          150 beats a minute), else to the nearest tick
      --transpose N       semitones up, or down; not the drums
      --loop              the tune starts again when it ends (the default
                          is to play it once)

      Volume (control 7) and reset all controllers (121) are left out, as the
      sequencer sends its own. Channels above 9 are moved to free ones below,
      as tunes have 0 to 9, 9 being the drums. A note still held at the end
      is let go there.
"""

import argparse
import struct
import sys

SEQ_HZ = 120
MAX_WAIT = 255
TRACKS = 10
DRUMS = 9

GM = """ACOUSTIC_GRAND_PIANO BRIGHT_ACOUSTIC_PIANO ELECTRIC_GRAND_PIANO HONKY_TONK_PIANO
ELECTRIC_PIANO_1 ELECTRIC_PIANO_2 HARPSICHORD CLAVINET CELESTA GLOCKENSPIEL MUSIC_BOX
VIBRAPHONE MARIMBA XYLOPHONE TUBULAR_BELLS DULCIMER DRAWBAR_ORGAN PERCUSSIVE_ORGAN
ROCK_ORGAN CHURCH_ORGAN REED_ORGAN ACCORDION HARMONICA TANGO_ACCORDION NYLON_GUITAR
STEEL_GUITAR JAZZ_GUITAR CLEAN_GUITAR MUTED_GUITAR OVERDRIVEN_GUITAR DISTORTION_GUITAR
GUITAR_HARMONICS ACOUSTIC_BASS FINGER_BASS PICK_BASS FRETLESS_BASS SLAP_BASS_1 SLAP_BASS_2
SYNTH_BASS_1 SYNTH_BASS_2 VIOLIN VIOLA CELLO CONTRABASS TREMOLO_STRINGS PIZZICATO_STRINGS
ORCHESTRAL_HARP TIMPANI STRING_ENSEMBLE_1 STRING_ENSEMBLE_2 SYNTH_STRINGS_1 SYNTH_STRINGS_2
CHOIR_AAHS VOICE_OOHS SYNTH_VOICE ORCHESTRA_HIT TRUMPET TROMBONE TUBA MUTED_TRUMPET
FRENCH_HORN BRASS_SECTION SYNTH_BRASS_1 SYNTH_BRASS_2 SOPRANO_SAX ALTO_SAX TENOR_SAX
BARITONE_SAX OBOE ENGLISH_HORN BASSOON CLARINET PICCOLO FLUTE RECORDER PAN_FLUTE
BLOWN_BOTTLE SHAKUHACHI WHISTLE OCARINA SQUARE_LEAD SAW_LEAD CALLIOPE_LEAD CHIFF_LEAD
CHARANG_LEAD VOICE_LEAD FIFTHS_LEAD BASS_LEAD NEW_AGE_PAD WARM_PAD POLYSYNTH_PAD CHOIR_PAD
BOWED_PAD METALLIC_PAD HALO_PAD SWEEP_PAD RAIN SOUNDTRACK CRYSTAL ATMOSPHERE BRIGHTNESS
GOBLINS ECHOES SCI_FI SITAR BANJO SHAMISEN KOTO KALIMBA BAGPIPE FIDDLE SHANAI TINKLE_BELL
AGOGO STEEL_DRUMS WOODBLOCK TAIKO_DRUM MELODIC_TOM SYNTH_DRUM REVERSE_CYMBAL
GUITAR_FRET_NOISE BREATH_NOISE SEASHORE BIRD_TWEET TELEPHONE_RING HELICOPTER APPLAUSE
GUNSHOT""".split()
assert len(GM) == 128

DRUM_NAMES = dict(enumerate("""ACOUSTIC_BASS BASS SIDE_STICK SNARE HAND_CLAP ELECTRIC_SNARE
LOW_FLOOR_TOM CLOSED_HI_HAT HIGH_FLOOR_TOM PEDAL_HI_HAT LOW_TOM OPEN_HI_HAT LOW_MID_TOM
HIGH_MID_TOM CRASH HIGH_TOM RIDE CHINESE_CYMBAL RIDE_BELL TAMBOURINE SPLASH COWBELL CRASH_2
VIBRASLAP RIDE_2 HIGH_BONGO LOW_BONGO MUTE_HIGH_CONGA OPEN_HIGH_CONGA LOW_CONGA
HIGH_TIMBALE LOW_TIMBALE HIGH_AGOGO LOW_AGOGO CABASA MARACAS SHORT_WHISTLE LONG_WHISTLE
SHORT_GUIRO LONG_GUIRO CLAVES HIGH_WOOD_BLOCK LOW_WOOD_BLOCK MUTE_CUICA OPEN_CUICA
MUTE_TRIANGLE OPEN_TRIANGLE""".split(), 35))

CC_NAMES = {1: "CC_MODULATION", 10: "CC_PAN", 11: "CC_EXPRESSION", 91: "CC_REVERB", 93: "CC_CHORUS"}
NOTE_NAMES = ["C", "CS", "D", "DS", "E", "F", "FS", "G", "GS", "A", "AS", "B"]


# ----------------------------------------------------------------------------
# Reading

class Event:
    """A channel message at a time in seconds: kind is on, off, prog, cc or bend"""

    def __init__(self, t, channel, kind, a=0, b=0):
        self.t, self.channel, self.kind, self.a, self.b = t, channel, kind, a, b


def read_number(d, i):
    v = 0
    while True:
        c = d[i]
        i += 1
        v = (v << 7) | (c & 0x7F)
        if not c & 0x80:
            return v, i


def read_midi(path):
    """The channel messages, in time order, and the end in seconds"""
    d = open(path, "rb").read()
    if d[:4] != b"MThd":
        sys.exit(f"{path}: not a MIDI file")
    hlen = struct.unpack(">I", d[4:8])[0]
    _fmt, ntracks, division = struct.unpack(">HHH", d[8:14])
    if division & 0x8000:
        sys.exit(f"{path}: SMPTE timing isn't handled")
    i = 8 + hlen
    raw = []  # (tick, order, channel message or tempo)
    order = 0
    for _ in range(ntracks):
        if d[i:i + 4] != b"MTrk":
            break
        tlen = struct.unpack(">I", d[i + 4:i + 8])[0]
        j, end = i + 8, i + 8 + tlen
        tick, status = 0, 0
        while j < end:
            delta, j = read_number(d, j)
            tick += delta
            c = d[j]
            if c == 0xFF:
                kind = d[j + 1]
                n, j = read_number(d, j + 2)
                if kind == 0x51:
                    raw.append((tick, order, ("tempo", int.from_bytes(d[j:j + 3], "big"))))
                elif kind == 0x2F:
                    raw.append((tick, order, ("end",)))
                j += n
            elif c in (0xF0, 0xF7):
                n, j = read_number(d, j + 1)
                j += n
            else:
                if c & 0x80:
                    status = c
                    j += 1
                k = status & 0xF0
                a = d[j]
                b = d[j + 1] if k not in (0xC0, 0xD0) else 0
                j += 1 if k in (0xC0, 0xD0) else 2
                raw.append((tick, order, ("msg", status, a, b)))
            order += 1
        i = end
    raw.sort(key=lambda r: (r[0], r[1]))
    # Ticks to seconds, through the tempo changes
    tempo, last_tick, secs = 500000, 0, 0.0
    events, the_end = [], 0.0
    for tick, _, r in raw:
        secs += (tick - last_tick) * tempo / 1e6 / division
        last_tick = tick
        if r[0] == "tempo":
            tempo = r[1]
        elif r[0] == "end":
            the_end = max(the_end, secs)
        else:
            _, status, a, b = r
            k, ch = status & 0xF0, status & 0x0F
            if k == 0x90 and b:
                events.append(Event(secs, ch, "on", a, b))
            elif k in (0x80, 0x90):
                events.append(Event(secs, ch, "off", a))
            elif k == 0xC0:
                events.append(Event(secs, ch, "prog", a))
            elif k == 0xB0:
                events.append(Event(secs, ch, "cc", a, b))
            elif k == 0xE0:
                events.append(Event(secs, ch, "bend", b, a))
    the_end = max([the_end] + [e.t for e in events])
    return events, the_end


def parse_channels(s):
    if not s:
        return None
    out = set()
    for part in s.split(","):
        if "-" in part:
            lo, hi = part.split("-")
            out.update(range(int(lo), int(hi) + 1))
        else:
            out.add(int(part))
    return out


# ----------------------------------------------------------------------------
# dump

def dump(args):
    events, the_end = read_midi(args.file)
    chans = parse_channels(args.channels)
    for e in events:
        if chans is not None and e.channel not in chans:
            continue
        print(f"{e.t:8.3f} ch{e.channel + 1:02d} {e.kind:4s} {e.a:3d} {e.b:3d}")
    print(f"{the_end:8.3f} end")


# ----------------------------------------------------------------------------
# seq

def note_name(n):
    return f"{NOTE_NAMES[n % 12]}{n // 12 - 1}"


class Writer:
    """A channel's steps, as C, with the wait carried to the next step"""

    def __init__(self):
        self.steps = []
        self.wait = 0

    def gap(self, w):
        self.wait += w

    def take_wait(self):
        """The wait for a step that can have one, after any REST it needs"""
        while self.wait > MAX_WAIT:
            self.steps.append(f"REST({MAX_WAIT})")
            self.wait -= MAX_WAIT
        w, self.wait = self.wait, 0
        return w

    def no_wait(self):
        """Waits out the wait, for a step that has none"""
        w = self.take_wait()
        if w:
            self.steps.append(f"REST({w})")

    def add(self, s):
        self.steps.append(s)


def seq(args):
    events, the_end = read_midi(args.file)
    chans = parse_channels(args.channels)
    if chans is not None:
        events = [e for e in events if e.channel in chans]
    if args.start is not None:
        start = args.start
    else:
        ons = [e.t for e in events if e.kind == "on"]
        start = min(ons) if ons else 0.0
    stop = args.stop if args.stop is not None else the_end
    grid = max(1, args.grid)

    def ticks(t):
        v = (t - start) / args.speed * SEQ_HZ
        return int(round(v / grid)) * grid

    # What is set before the start is set at it
    for e in events:
        if e.t < start and e.kind in ("prog", "cc", "bend"):
            e.t = start
    events = [e for e in events if start <= e.t <= stop]
    if not events:
        sys.exit("No events to turn into steps")
    end_tick = ticks(stop)

    # Channels above 9, but the drums, onto free ones below
    used = sorted({e.channel for e in events})
    free = [c for c in range(TRACKS) if c != DRUMS and c not in used]
    move = {}
    for c in used:
        if c >= TRACKS:
            if not free:
                sys.exit(f"Too many channels: tunes have {TRACKS}")
            move[c] = free.pop(0)
            print(f"// channel {c} moved to {move[c]}", file=sys.stderr)
    by_channel = {}
    for e in events:
        by_channel.setdefault(move.get(e.channel, e.channel), []).append(e)

    out = [f"// From {args.file}, by apps/common/tools/midtool.py seq\n"]
    tracks = []
    for ch in sorted(by_channel):
        drums = ch == DRUMS
        evs = []
        bent = False
        for e in by_channel[ch]:
            if e.kind == "cc" and e.a in (7, 121):
                continue
            # The sequencer starts with no bend
            if e.kind == "bend":
                if e.a == 64 and not bent:
                    continue
                bent = True
            n = e.a
            if e.kind in ("on", "off") and not drums:
                n = min(127, max(0, n + args.transpose))
            # Let go before taking up, so the same note again is heard
            rank = {"prog": 0, "cc": 1, "bend": 2, "off": 3, "on": 4}[e.kind]
            evs.append((ticks(e.t), rank, e.kind, n, e.b))
        evs.sort(key=lambda x: (x[0], x[1]))
        # Notes on at the end let go there
        held = {}
        for t, _, k, n, _ in evs:
            if k == "on":
                held[n] = held.get(n, 0) + 1
            elif k == "off" and held.get(n):
                held[n] -= 1
        for n, count in held.items():
            evs.extend([(end_tick, 3, "off", n, 0)] * count)
        evs.sort(key=lambda x: (x[0], x[1]))

        def note(n):
            if drums and n in DRUM_NAMES:
                return f"GM_DRUM_{DRUM_NAMES[n]}"
            return str(n)

        w = Writer()
        now = 0
        i = 0
        while i < len(evs):
            t, _, k, n, b = evs[i]
            w.gap(t - now)
            now = t
            if k == "on":
                # A note with nothing else on the channel until it is let go
                nxt = evs[i + 1] if i + 1 < len(evs) else None
                if nxt and nxt[2] == "off" and nxt[3] == n and 0 < nxt[0] - t <= MAX_WAIT:
                    w.add(f"PLAY({w.take_wait()}, {note(n)}, {nxt[0] - t}, {b})")
                    now = nxt[0]
                    i += 2
                    continue
                w.add(f"ON({w.take_wait()}, {note(n)}, {b})")
            elif k == "off":
                w.add(f"OFF({w.take_wait()}, {note(n)})")
            elif k == "prog":
                w.no_wait()
                w.add(f"PROG(GM_{GM[n]})")
            elif k == "cc":
                w.add(f"CC_AFTER({w.take_wait()}, {CC_NAMES.get(n, n)}, {b})")
            elif k == "bend":
                w.no_wait()
                w.add(f"BEND({n})")
            i += 1
        w.gap(end_tick - now)
        w.add(f"END({w.take_wait()})")

        arr = f"_{args.name}_{ch}"
        tracks.append((ch, arr))
        progs = sorted({GM[n] for _, _, k, n, _ in evs if k == "prog"})
        what = "drums" if drums else ", ".join(p.lower().replace("_", " ") for p in progs) or "no instrument given"
        lo = [n for _, _, k, n, _ in evs if k == "on"]
        span = f", {note_name(min(lo))} to {note_name(max(lo))}" if lo and not drums else ""
        out.append(f"// Channel {ch}: {what}{span}")
        out.append(f"static const SeqStep_t {arr}[] = {{")
        line = "\t"
        for s in w.steps:
            piece = s + ", "
            if len(line) + len(piece) > 100:
                out.append(line.rstrip())
                line = "\t"
            line += piece
        out.append(line.rstrip().rstrip(","))
        out.append("};\n")
    out.append(f"static const SeqTrack_t _{args.name}[] = {{")
    for ch, arr in tracks:
        out.append(f"\t{{ {'GM_DRUM_CHANNEL' if ch == DRUMS else ch}, {arr} }},")
    out.append("};")
    loop = "true" if args.loop else "false"
    out.append(f"// For the tune table: {{ _{args.name}, {len(tracks)}, {loop} }}, "
               f"{end_tick / SEQ_HZ:.1f} seconds")
    print("\n".join(out))


def main():
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = p.add_subparsers(dest="cmd", required=True)
    d = sub.add_parser("dump", help="list a MIDI file's events")
    d.add_argument("file")
    d.add_argument("--channels")
    d.set_defaults(func=dump)
    s = sub.add_parser("seq", help="turn a MIDI file into sequencer steps")
    s.add_argument("file")
    s.add_argument("name", help="what the arrays are called after, e.g. title")
    s.add_argument("--channels")
    s.add_argument("--from", dest="start", type=float)
    s.add_argument("--to", dest="stop", type=float)
    s.add_argument("--speed", type=float, default=1.0)
    s.add_argument("--grid", type=int, default=1)
    s.add_argument("--transpose", type=int, default=0)
    s.add_argument("--loop", action="store_true")
    s.set_defaults(func=seq)
    a = p.parse_args()
    a.func(a)


if __name__ == "__main__":
    main()
