#!/bin/sh
# Renders each of a game's sounds and tunes to a MIDI file of its own, to
# listen to on the PC, see render_sounds.c.
#
# common/tools/render_sounds.sh <game dir> [out dir] [name]
#
# The game's sounds are its src/*sound*.c, which give game_sounds. The files
# go in <game dir>/host/build/sounds unless told otherwise. With WAV=1, each
# is made into a .wav as well, with fluidsynth and the soundfont in SF2
# (FluidR3_GM by default).
set -e
T=$(cd "$(dirname "$0")" && pwd)
R=$(cd "$T/../../.." && pwd)
GAME=$(cd "$1" && pwd)
OUT=${2:-$GAME/host/build/sounds}
SF2=${SF2:-/usr/share/sounds/sf2/FluidR3_GM.sf2}
mkdir -p "$OUT"
B=$(mktemp -d)
trap 'rm -rf "$B"' EXIT
C="$R/apps/common/game"
gcc -O2 -std=gnu11 -Wall -Wno-unused-function -DGAME_HOST=1 -DMODE_BYTES_PER_PIXEL=1 \
	-I"$R/apps/common/host/stub" -I"$R/apps/common/host" -I"$R/src" -I"$C" \
	-o "$B/render_sounds" "$T/render_sounds.c" \
	"$C/game_seq.c" "$C/game_sound.c" "$C/game_synth.c" "$C/game_synth_file.c" \
	"$GAME"/src/*sound*.c
"$B/render_sounds" "$OUT" "$3" | while read -r f; do
	echo "$f"
	if [ -n "$WAV" ]; then fluidsynth -ni -F "${f%.mid}.wav" -r 44100 "$SF2" "$f" >/dev/null 2>&1; fi
done
