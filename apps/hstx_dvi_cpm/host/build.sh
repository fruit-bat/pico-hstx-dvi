#!/bin/sh
# Builds hstx_dvi_cpm to run on the PC, for screenshots (see cpm_host.cpp).
# It needs CPM-Emulator and pico-vt beside this repository.
#
# apps/hstx_dvi_cpm/host/build.sh [build dir]
set -e
H=$(cd "$(dirname "$0")" && pwd)
R=$(cd "$H/../../.." && pwd)
P=$(cd "$R/.." && pwd)
E=$P/CPM-Emulator
V=$P/pico-vt/src
B=${1:-$H/build}
mkdir -p "$B"
cd "$B"
rm -f ./*.o
# The renderer is built from a copy, so it picks up the stub row FIFO and buffer
cp "$R/src/hstx_dvi_vt.c" "$B/hstx_dvi_vt.c"
DEFS="-DMODE_BYTES_PER_PIXEL=1 -DVT_FONT=\"terminus_8x16\" -DCPM_Z80_KHZ=0 -DCPM_TERMINAL=CONSOLE_MIXED -DZ80_STATIC -DZ80_WITH_EXECUTE -DZ80_WITH_Q -DZ80_WITH_ZILOG_NMOS_LD_A_IR_BUG"
INC="-I$H/stub -I$R/apps/common/host/stub -I$R/src -I$V -I$R/apps/hstx_dvi_cpm/src -I$E/sources -I$E/sources/host/std -I$E/external/Z80/API -I$E/external/Zeta/API"
gcc -O2 -g -std=gnu11 -w $DEFS $INC -c "$B/hstx_dvi_vt.c" "$R/src/hstx_dvi_font.c" "$R/src/hstx_dvi_fonts.c" "$R/src/font_inv.c" \
  "$V/vt_parser.c" "$V/vt_term.c" "$V/vt_tabs.c" "$V/vt_pallet.c" "$V/vt_emu.c" "$V/vt_cell.c" "$V/vt_utf8.c" "$V/vt_charmap.c"
gcc -O2 -g -std=gnu11 -fexceptions $DEFS $INC -c "$E/external/Z80/sources/Z80.c"
g++ -O2 -g -std=gnu++17 -Wall $DEFS $INC -c "$R/apps/hstx_dvi_cpm/src/cpm_settings.cpp" "$R/apps/hstx_dvi_cpm/src/z80_disasm.cpp" "$H/cpm_host.cpp"
g++ -o "$B/cpm_host" ./*.o
echo "Built $B/cpm_host"
