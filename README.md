# pico-hstx-dvi
Just messing with HSTX

Examples for RGB332 and RGB565

## Building

HSTX is only on the RP2350, so the apps build for a Pico 2 unless another board is given:

```sh
cd apps
mkdir build_pico2 && cd build_pico2
cmake ..                       # or e.g. cmake -DPICO_BOARD=pico2_w ..
make -j
```

The games are played with NES or SNES gamepads, read with
[pico-nespad](https://github.com/fruit-bat/pico-nespad) checked out next to this project.
Both pads control the game. They share CLK on GPIO 26 and LATCH on GPIO 27, with DATA
from pad 1 on GPIO 20 and pad 2 on GPIO 21 (see `apps/common/game/CMakeLists.txt`).

Buttons on GPIOs, each to ground, can be read as well. They are off unless asked for,
leaving the GPIOs free for other things; each game's README says which GPIOs they use:

```sh
cmake -DGAME_BUTTONS=ON ..     # and -DGAME_NESPAD=OFF for buttons alone
```

The games play music and sounds as MIDI, at 31250 baud on GPIO 8 (UART1's TX), to a
General MIDI synth such as a SAM2695, which is then no button's. This can be left out with
`-DGAME_MIDI=OFF`.

## CP/M (hstx_dvi_cpm)

`hstx_dvi_cpm` runs CP/M 2.2 on the DVI screen with
[CPM-Emulator](https://github.com/fruit-bat/CPM-Emulator) (a Z80 with an emulated BDOS). It needs
CPM-Emulator (with its submodules) and [pimoroni-pico](https://github.com/pimoroni/pimoroni-pico)
(for its SD card & FatFs drivers) cloned beside this repository.

The drives are directories on an SD card, wired to SPI0 on the GPIOs the Pico DVI Sock leaves free:

| SD card | GPIO |
|---|---|
| SCK  | 2 |
| MOSI | 3 |
| MISO | 4 |
| CS   | 5 |

The card is FAT formatted, with the CCP and a directory for each drive in a `cpm` directory, as the
`run` directory of CPM-Emulator:

```
cpm/
  CCP-DR.64K
  A/   PIP.COM, STAT.COM, MBASIC.COM...
  B/
```

Without a `cpm` directory they're looked for at the top of the card. The name is `CPM_DIR` in
[`apps/hstx_dvi_cpm/CMakeLists.txt`](apps/hstx_dvi_cpm/CMakeLists.txt).

Only files with 8.3 names are seen. For now keys come from the USB serial port, and the output is
shown both on the screen and over the USB serial port, so any terminal program works:

```sh
picocom -q /dev/ttyACM0      # or screen /dev/ttyACM0, minicom -D /dev/ttyACM0...
```

Without an SD card, or if CP/M stops on an error (e.g. a Z80 `IN` or `OUT`, which aren't emulated),
it says so and restarts on a key press.

## Using the virtual terminal (hstx_dvi_vt_test)

`hstx_dvi_vt_test` turns the Pico into a terminal on the DVI screen, 80x60 with the 8x8 font, or other
sizes with bigger fonts (below). It shows what arrives on
the USB serial port and sends its replies (e.g. cursor position and size reports) back the same way.
It has no keyboard of its own, so programs run on the PC and are typed into from there.

[`pico_term.py`](apps/hstx_dvi_vt_test/pico_term.py) does this: it runs a program on a pty sized to
match the Pico (it asks with `ESC [ 18 t`), sends its output to the Pico, and passes both your keys and
the Pico's replies to it. It needs Python 3 and nothing else.

```sh
apps/hstx_dvi_vt_test/pico_term.py -- vim     # vim on the Pico screen
apps/hstx_dvi_vt_test/pico_term.py            # your shell, then try less, top, vttest...
```

* `Ctrl-]` quits.
* `-p /dev/ttyACM1` picks another port (default `/dev/ttyACM0`). You need to be in the `dialout`
  group, and nothing else (minicom, screen...) can have the port open.
* `TERM` is `xterm-256color`; set `PICO_TERM` to try another, e.g. `PICO_TERM=vt100`.

Without the script, plain commands also work, though the Pico's replies are then ignored:

```sh
stty -F /dev/ttyACM0 raw -echo           # no echo, no newline translation
stty -F /dev/ttyACM0 rows 60 cols 80
TERM=xterm-256color vim > /dev/ttyACM0   # keys from here, screen on the Pico
```

Make the port `raw -echo` first: with echo on, Linux sends the Pico's replies straight back to it and
they show up as junk on the screen.

A terminal program such as minicom or screen is fine for typing at the Pico, but it swallows the
replies (it treats them as escape sequences for its own screen). To see them, read the port directly:

```sh
python3 -c "import serial,time; s=serial.Serial('/dev/ttyACM0'); s.write(b'\x1b[6n'); time.sleep(0.3); print(repr(s.read(s.in_waiting)))"
# b'\x1b[5;1R'
```

(this one needs pyserial).

### Fonts and sizes

The terminal's font is chosen when it is built, and it can be drawn twice as wide, twice as tall or both:

```sh
cmake -DVT_FONT=terminus_8x16 ..                         # 80x30
cmake -DVT_FONT=8x8 -DVT_SCALE_X=2 -DVT_SCALE_Y=2 ..     # 40x30
```

| Font             | Size at 640x480 | Twice as big |
|------------------|-----------------|--------------|
| `8x8` (default)  | 80x60           | 40x30 (and 80x30 only twice as tall) |
| `terminus_8x14`  | 80x34           | 40x17        |
| `terminus_8x16`  | 80x30           | 40x15        |
| `vga_8x16`       | 80x30           | 40x15        |
| `terminus_12x24` | 53x20           | 26x10        |
| `terminus_16x32` | 40x15           | 20x7         |

The terminal is as big as fits, and `pico_term.py` asks the Pico for its size, so it needs no changes.

A size can also be asked for, and the Pico then picks the biggest font it fits with (Terminus, once or
twice as big, else the 8x8 font), and shows the rest of the screen blank:

```sh
apps/hstx_dvi_vt_test/pico_term.py -s 80x24 -- vim   # Terminus 8x16
apps/hstx_dvi_vt_test/pico_term.py -s 53x20          # Terminus 12x24
apps/hstx_dvi_vt_test/pico_term.py -s 20x10          # Terminus 12x24, twice as big
```

This is what any program asking with `ESC [ 8 ; rows ; cols t` gets. It can be turned off with
`-DVT_FONT_AUTO=OFF`, when the font stays as built and the size is cut down to what fits.

All but the 8x8 font have line drawing (as UTF-8 or the DEC set), double and rounded box lines, blocks,
arrows, curly quotes and dashes, and Latin-1's accented letters, so menus, `mc`, `htop` and the like
draw proper boxes. The 8x8 font, ASCII only, shows them as near as it can (`+`, `-`, `|`, `#`...).

From code, `hstx_dvi_vt_set_font()` and `hstx_dvi_vt_set_font_auto()` (in `src/hstx_dvi_vt.h`) change the
font at any time, resizing the terminal to suit.

The bigger fonts are Terminus, by Dimitar Zhekov, under the SIL Open Font License 1.1, and the IBM PC's
VGA font, both from the Linux console fonts. [`tools/psf2c.py`](tools/psf2c.py) turns such fonts into C
(see the top of `src/hstx_dvi_fonts.c` for how they were made), with the 128 extra glyphs for character
codes 128 to 255, drawing the few simple shapes (half blocks, the diamond, scan lines) a font lacks. A
font can be up to 16 pixels wide and 32 tall, and its width times its scale has to be a multiple of 4.

