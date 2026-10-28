#!/bin/sh
# Builds the defender app to run on the PC, for trying changes without a Pico.
#
# host/build.sh [build dir]
set -e
H=$(cd "$(dirname "$0")" && pwd)
R=$(cd "$H/../../.." && pwd)
B=${1:-$H/build}
mkdir -p "$B"
# The renderer is built from a copy, so it picks up the stub row FIFO and buffer
cp "$R/src/hstx_dvi_sprite.c" "$B/hstx_dvi_sprite.c"
FLAGS="-O2 -g -std=gnu11 -Wall -Wno-unused-function -DMODE_BYTES_PER_PIXEL=1 -I$H/stub -I$R/src"
cd "$B"
gcc $FLAGS -Wno-return-type -Dmain=main_game -c "$R"/apps/hstx_dvi_defender/src/*.c
gcc $FLAGS -c "$R/src/font_inv.c" "$B/hstx_dvi_sprite.c" "$H/harness.c"
gcc -o "$B/defender_host" "$B"/*.o -lpthread
echo "Built $B/defender_host"
