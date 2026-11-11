# CP/M on the DVI screen (hstx_dvi_cpm)

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
[`CMakeLists.txt`](CMakeLists.txt).

Only files with 8.3 names are seen. For now keys come from the USB serial port, and the output is
shown both on the screen and over the USB serial port, so any terminal program works:

```sh
tio /dev/ttyACM0             # or picocom -q /dev/ttyACM0, screen /dev/ttyACM0...
```

The screen is 80x30, in the Terminus 8x16 font. For 80x60 in the 8x8 font, build with
`cmake -DCPM_VT_FONT=8x8 ..` (any font in [Fonts and sizes](../../README.md#fonts-and-sizes) will do).

## Settings screen

**Ctrl-\\** (or **F12** on a USB keyboard) opens the settings screen, pausing the Z80 until it is closed.
CP/M programs never see the key. Esc only closes it: CP/M programs and terminals use Esc themselves
(a serial terminal's arrow keys send `Esc [ A`...), so it can't open it. Terminals such as tio, picocom
and screen pass Ctrl-\\ on as it is (their own commands are on Ctrl-T, Ctrl-A...).

The screen is drawn on the terminal's alternate screen, so whatever CP/M had on the screen comes back as
it was. It is only on the DVI screen, not the serial port. It shows:

- **The Z80's speed.** **+** and **-** choose from unlimited (as fast as the Pico goes, the default),
  1, 2, 2.5, 4 (a Z80A), 6, 8, 10 and 20 MHz, with the speed measured while it runs. A speed can be set
  when built, e.g. `cmake -DCPM_Z80_KHZ=4000 ..`.
- **The registers:** PC, SP, AF, BC, DE, HL, IX, IY, the alternate set, I, R, the flags, the
  interrupt mode and the top of the stack.
- **The code**, disassembled: the last few instructions stepped (dim), the one at PC (marked `->`) and
  those after it. A call to the BDOS (at 0005h) or the BIOS is marked, as the emulator runs those itself.
- **Memory**, in hex and ASCII, with the byte at PC in reverse and at SP underlined. It follows PC, or
  **M** shows an address (typed in hex), **K** the stack, **Up**/**Down** (or **,** **.**) move a line and
  **PgUp**/**PgDn** (or **<** **>**) a page. **P** follows PC again.

**S** or **Space** steps the Z80 an instruction (a BDOS or BIOS call being one step) and shows it all
again, so it can be pressed again and again. Only **C**, **Esc** and **Ctrl-\\** go back to CP/M, and
**R** restarts CP/M, as a reset; other keys do nothing.

The Z80 can't be stepped while CP/M waits for input, e.g. at the `A>` prompt or BASIC's `Ok`, as it
is then in a BDOS or BIOS call. The screen says so, and **S** then sets it to stop at the first
instruction after the input (**S** again doesn't): go back to CP/M with **Esc**, type the input (a whole
line, with Enter, at a prompt), and the screen comes back there. A running program can be stepped
straight away.

At power on the SD card can take a moment to be ready, so CP/M tries it for a couple of seconds before
saying it can't read it.

Without an SD card, or if CP/M stops on an error (e.g. a Z80 `IN` or `OUT`, which aren't emulated),
it says so and restarts on a key press.
