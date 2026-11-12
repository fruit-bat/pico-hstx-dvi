# Virtual terminal (hstx_dvi_vt_test)

`hstx_dvi_vt_test` turns the Pico into a terminal on the DVI screen, 80x60 with the 8x8 font, or other
sizes with bigger fonts ([below](#fonts-and-sizes)). It shows what arrives on
the USB serial port and sends its replies (e.g. cursor position and size reports) back the same way.
It has no keyboard of its own, so programs run on the PC and are typed into from there.

[`pico_term.py`](pico_term.py) does this: it runs a program on a pty sized to
match the Pico (it asks with `ESC [ 18 t`), sends its output to the Pico, and passes both your keys and
the Pico's replies to it. It needs Python 3 and nothing else.

```sh
cd apps/hstx_dvi_vt_test
./pico_term.py -- vim     # vim on the Pico screen
./pico_term.py            # your shell, then try less, top, vttest...
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

## Fonts and sizes

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
./pico_term.py -s 80x24 -- vim   # Terminus 8x16
./pico_term.py -s 53x20          # Terminus 12x24
./pico_term.py -s 20x10          # Terminus 12x24, twice as big
```

This is what any program asking with `ESC [ 8 ; rows ; cols t` gets. It can be turned off with
`-DVT_FONT_AUTO=OFF`, when the font stays as built and the size is cut down to what fits.

All but the 8x8 font have line drawing (as UTF-8 or the DEC set), double and rounded box lines, blocks,
arrows, curly quotes and dashes, and Latin-1's accented letters, so menus, `mc`, `htop` and the like
draw proper boxes. The 8x8 font, ASCII only, shows them as near as it can (`+`, `-`, `|`, `#`...).

Which fonts are built at all, to save flash and RAM, is set for the whole project: see
[Choosing fonts](../../README.md#choosing-fonts-quality-against-memory) in the main README.

It needs the DVI connector on GPIOs 12 to 19, as on the
[Pico DVI Sock](https://github.com/Wren6991/Pico-DVI-Sock).
