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

`hstx_dvi_cpm` runs CP/M 2.2 on the DVI screen, with its drives on an SD card, and a settings screen
(**Ctrl-\\**) for the Z80's speed, its registers and memory, and stepping it. See
[`apps/hstx_dvi_cpm/README.md`](apps/hstx_dvi_cpm/README.md) for what it needs, the SD card's wiring and
layout, and the keys.

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

#### Choosing fonts: quality against memory

Each larger font can be left out of the build, to save flash and RAM. They are all in by default:

| CMake option (default `ON`)    | Font             | Flash   | Gives                          |
|--------------------------------|------------------|---------|--------------------------------|
| `HSTX_DVI_FONT_TERMINUS_8X14`  | `terminus_8x14`  | 3.0 KB  | 80x34                          |
| `HSTX_DVI_FONT_TERMINUS_8X16`  | `terminus_8x16`  | 3.5 KB  | 80x30, 80x24                   |
| `HSTX_DVI_FONT_VGA_8X16`       | `vga_8x16`       | 3.5 KB  | 80x30, in the PC's look        |
| `HSTX_DVI_FONT_TERMINUS_12X24` | `terminus_12x24` | 10.5 KB | 53x20, and 26x10 twice as big  |
| `HSTX_DVI_FONT_TERMINUS_16X32` | `terminus_16x32` | 13.9 KB | 40x15, and 20x7 twice as big   |

The 8x8 font is always there (0.8 KB, ASCII only), so with every option `OFF` the terminal is as it was
before the larger fonts came along. The terminal also keeps a copy of the font in use in RAM, sized for
the biggest font built:

| Biggest font built          | RAM for the font |
|-----------------------------|------------------|
| `terminus_16x32`            | 13.9 KB          |
| `terminus_12x24`            | 10.5 KB          |
| an 8x16                     | 3.5 KB           |
| `terminus_8x14`             | 3.0 KB           |
| only the 8x8 font           | 0.7 KB           |

For example:

```sh
cmake -DHSTX_DVI_FONT_TERMINUS_16X32=OFF ..         # all but the biggest: saves 14 KB flash, 3.5 KB RAM
cmake -DHSTX_DVI_FONT_TERMINUS_12X24=OFF -DHSTX_DVI_FONT_TERMINUS_16X32=OFF \
      -DHSTX_DVI_FONT_VGA_8X16=OFF ..               # 80-column line drawing only: 6.5 KB flash, 3.5 KB RAM
```

The options apply to every app using the terminal (the VT test, CP/M and Lisp). CP/M and Lisp use
`terminus_8x16` (80x30), set by `CPM_VT_FONT` and `LISP_VT_FONT`; if it isn't built they use the 8x8 font. The automatic choice
only picks from the fonts built (using `vga_8x16` for 8x16 if `terminus_8x16` is left out), and
`hstx_dvi_font_find()` doesn't find the others. Choosing one that isn't built for `VT_FONT` stops CMake
with an error. Outside CMake, define `HSTX_DVI_FONT_TERMINUS_16X32=0` and the like (see
`src/hstx_dvi_font.h`).

An app can give the terminal an alternate screen with `hstx_dvi_vt_enable_alt_screen()`, as full screen
programs (`vim`, `less`...) switch to with `ESC [ ? 1049 h`, leaving the normal screen as it was. It costs
another screen of cells in RAM (19 KB at 640x480), so is only there if asked for; CP/M uses it.

From code, `hstx_dvi_vt_set_font()` and `hstx_dvi_vt_set_font_auto()` (in `src/hstx_dvi_vt.h`) change the
font at any time, resizing the terminal to suit.

The bigger fonts are Terminus, by Dimitar Zhekov, under the SIL Open Font License 1.1, and the IBM PC's
VGA font, both from the Linux console fonts. [`tools/psf2c.py`](tools/psf2c.py) turns such fonts into C
(see the top of `src/hstx_dvi_fonts.c` for how they were made), with the 128 extra glyphs for character
codes 128 to 255, drawing the few simple shapes (half blocks, the diamond, scan lines) a font lacks. A
font can be up to 16 pixels wide and 32 tall, and its width times its scale has to be a multiple of 4.

