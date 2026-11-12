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

## The apps

Each app has a README saying what it shows, what it needs and how to use it:

| App | What it is |
|-----|------------|
| [`hstx_dvi_hello_world`](apps/hstx_dvi_hello_world/README.md) | A picture from a frame buffer: the simplest use of the library |
| [`hstx_dvi_row_fifo_test`](apps/hstx_dvi_row_fifo_test/README.md) | The screen drawn a row at a time through the row FIFO, recovering when it runs dry |
| [`hstx_dvi_text_test`](apps/hstx_dvi_text_test/README.md) | An 80x60 text grid with colours and attributes |
| [`hstx_dvi_tmt_test`](apps/hstx_dvi_tmt_test/README.md) | A terminal using libtmt |
| [`hstx_dvi_vt_test`](apps/hstx_dvi_vt_test/README.md) | A terminal on the screen for programs run on the PC (vim, htop...), with a choice of fonts |
| [`hstx_dvi_lisp_test`](apps/hstx_dvi_lisp_test/README.md) | A Lisp REPL on the terminal |
| [`hstx_dvi_cpm`](apps/hstx_dvi_cpm/README.md) | CP/M 2.2 with its drives on an SD card, a serial port, and a settings screen (**Ctrl-\\**) for the Z80's speed, registers, code and memory |
| [`hstx_dvi_sprite_test`](apps/hstx_dvi_sprite_test/README.md) | Invaders, with lots of sprites |
| [`hstx_dvi_android_test`](apps/hstx_dvi_android_test/README.md) | An android's walk cycle, in sprites taller than 16 pixels |
| [`hstx_dvi_defender`](apps/hstx_dvi_defender/README.md) | Guardian, a side scrolling shoot 'em up |
| [`hstx_dvi_wildwood`](apps/hstx_dvi_wildwood/README.md) | Wildwood, a flip screen maze adventure |
| [`hstx_dvi_manor`](apps/hstx_dvi_manor/README.md) | Moonlit Manor, a scrolling platform game in a haunted house |
| [`hstx_dvi_wireframe_test`](apps/hstx_dvi_wireframe_test/README.md) | Elite's ships in 3D, wireframe and filled, round a space station |
| [`hstx_dvi_harmless`](apps/hstx_dvi_harmless/README.md) | Harmless, a space trading and combat game in the style of Elite |

## Terminal fonts

The terminal (`src/hstx_dvi_vt.h`), used by the VT test, Lisp and CP/M, can use larger fonts than the
8x8 one. The [VT test's README](apps/hstx_dvi_vt_test/README.md#fonts-and-sizes) shows them and the
terminal sizes they give.

### Choosing fonts: quality against memory

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

