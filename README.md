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

## Using the virtual terminal (hstx_dvi_vt_test)

`hstx_dvi_vt_test` turns the Pico into an 80x60 terminal on the DVI screen. It shows what arrives on
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
