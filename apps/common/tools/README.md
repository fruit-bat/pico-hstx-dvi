# Sound tools

Tools for the games' music and sounds, run on a Linux PC. The games play them with the
sequencer in `../game/game_seq.h`, as MIDI to a General MIDI synth, through the drivers in
`../game/game_synth.h`. A game's sounds are written as steps with the macros in
`../game/game_seq_steps.h`, in its `src/*sound*.c`, which give `game_sounds` (see
`../game/game_sound.h`).

To listen to the MIDI files, e.g. with fluidsynth and its General MIDI soundfont:

```sh
fluidsynth -i /usr/share/sounds/sf2/FluidR3_GM.sf2 file.mid
```

## render_sounds.sh

Renders each of a game's sounds and tunes to a MIDI file of its own, as the sequencer plays
them, to listen to while working on them.

```sh
apps/common/tools/render_sounds.sh apps/hstx_dvi_defender            # all of them
apps/common/tools/render_sounds.sh apps/hstx_dvi_defender out LASER  # those named LASER, in out
WAV=1 apps/common/tools/render_sounds.sh apps/hstx_dvi_harmless      # and .wav files
```

They go in `<game>/host/build/sounds` unless told otherwise. A sound that loops is held for
three seconds, and a tune is played once through.

## midtool.py

```sh
apps/common/tools/midtool.py dump file.mid             # the events, a line each
apps/common/tools/midtool.py seq file.mid title --loop # a tune, as C
```

`seq` turns a MIDI file into a tune for the sequencer: a step array for each channel and a
track table, printed as C to paste into a game's sound data, with the instruments and drums
by name. It can take some of the channels (`--channels 0-3,9`), a stretch of it
(`--from 12.5 --to 40`), play it faster or slower (`--speed`), snap it to a grid of ticks
(`--grid`) and transpose it (`--transpose`). Run it with `-h` for the details.

A tune has channels 0 to 9, 9 being the drums, and each channel takes flash, so long pieces
are best cut down. A tune rendered by `render_sounds.sh` and turned back by `seq` plays
exactly as it did.
