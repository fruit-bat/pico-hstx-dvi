#!/bin/sh
# Builds a game to run on the PC, for trying changes without a Pico.
#
# common/host/build.sh <game dir> <name> [build dir]
#
# The game dir holds src/*.c and host/scenario.c. The build goes in
# <game dir>/host/build unless told otherwise.
set -e
H=$(cd "$(dirname "$0")" && pwd)
R=$(cd "$H/../../.." && pwd)
GAME=$(cd "$1" && pwd)
NAME=$2
B=${3:-$GAME/host/build}
mkdir -p "$B"
# The renderer is built from a copy, so it picks up the stub row FIFO and buffer
cp "$R/src/hstx_dvi_sprite.c" "$B/hstx_dvi_sprite.c"
FLAGS="-O2 -g -std=gnu11 -Wall -Wno-unused-function -DMODE_BYTES_PER_PIXEL=1 -I$H/stub -I$H -I$R/src -I$R/apps/common/game"
cd "$B"
rm -f "$B"/*.o
gcc $FLAGS -Wno-return-type -Dmain=main_game -c "$GAME"/src/*.c
gcc $FLAGS -c "$R"/apps/common/game/*.c "$R/src/font_inv.c" "$B/hstx_dvi_sprite.c" "$H/host.c" "$GAME/host/scenario.c"
gcc -o "$B/$NAME" "$B"/*.o -lpthread
echo "Built $B/$NAME"
