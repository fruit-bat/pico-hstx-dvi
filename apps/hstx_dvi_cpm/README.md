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
- **The terminal** programs are set up for, changed with **T**: see [Terminal codes](#terminal-codes).
- **The registers:** PC, SP, AF, BC, DE, HL, IX, IY, the alternate set, I, R, the flags, the
  interrupt mode and the top of the stack.
- **The code**, disassembled: the last few instructions stepped (dim), the one at PC (marked `->`) and
  those after it. A call to the BDOS (at 0005h) or the BIOS is marked, as the emulator runs those itself.
- **Memory**, in hex and ASCII, with the byte at PC in reverse and at SP underlined. It follows PC, or
  **M** shows an address (typed in hex), **K** the stack, **Up**/**Down** (or **,** **.**) move a line and
  **PgUp**/**PgDn** (or **<** **>**) a page. **P** follows PC again.
- **Editing memory:** **E** puts a cursor on the first byte shown. Type two hex digits to change a byte
  (the first is shown as it is typed), or press **Tab** to type text instead, and back. The arrows (or
  WordStar's Ctrl-E, X, S, D) and **PgUp**/**PgDn** move the cursor, the view following it, and **Enter**
  or **Esc** finishes. The code shown changes with it.

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

## The serial port: RDR: and PUN:

CP/M 2.2's serial devices are the reader (`RDR:`) and punch (`PUN:`), which go to UART1, at 3.3V:

| UART1 | GPIO |
|---|---|
| TX (`PUN:`) | 8 |
| RX (`RDR:`) | 9 |

at 115200 baud, 8 bits, no parity, 1 stop bit. With a USB serial adapter to a PC:

```
A>PIP PUN:=STAT.DOC           (sends a file, with 40 NULs before and after, as for paper tape)
A>PIP B:NOTES.TXT=RDR:        (receives one: send a text file, then Ctrl-Z)
```

What arrives is kept (up to 4K) until read, so nothing is lost while CP/M writes to the SD card.
While a program waits for the reader, **Ctrl-C** gives up, as if the file had ended, and **Ctrl-\\**
still opens the settings screen. Programs using BDOS functions 3 and 4, or the BIOS's READER and PUNCH,
use it too. The pins and baud rate are `CPM_AUX_TX_PIN`, `CPM_AUX_RX_PIN` and `CPM_AUX_BAUD` in
[`CMakeLists.txt`](CMakeLists.txt), and `-DCPM_AUX=OFF` leaves the reader and punch out.

Programs that drive a serial chip themselves (modem programs such as MEX, IMP or a machine's own
Kermit) won't work, as I/O ports aren't emulated: they stop CP/M with an error. Kermit-80's "generic
CP/M 2.2" version, which uses the reader and punch, should.

## Terminal codes

CP/M programs were written for many terminals, and each is set up (installed) for one. Output goes
through CPM-Emulator's `console.h`, which turns the codes of the terminal chosen into the ANSI ones the
screen and a PC's terminal understand. **T** on the settings screen changes it, and `-DCPM_TERMINAL=`
sets it when built:

| Terminal | `CPM_TERMINAL` | |
|---|---|---|
| Kaypro + CP/M-86 (mixed) | `mixed` (default) | A Kaypro's codes and CP/M-86's (VT52-like) together; anything else, such as ANSI codes, passed on |
| Kaypro / ADM-3A | `kaypro` | A Kaypro's only: ESC = r c, attributes, insert and delete line, and the ADM-3A's control codes |
| VT52 | `vt52` | A DEC VT52's: ESC A to D, H, I, J, K and ESC Y r c |
| ANSI / VT100 | `ansi` | Passed on as they are |

The two clash in the mixed one: there ESC B n and ESC C n set and clear a Kaypro attribute (on a VT52
they move the cursor down and right), and ESC E clears the screen and ESC Y r c moves the cursor, as
CP/M-86 has them (on a Kaypro they insert a line and clear to the end of the screen). So for a program
set up for a VT52, choose `vt52`, and for a Kaypro, `kaypro`. Codes with nothing like them in ANSI
(e.g. a Kaypro's pixel graphics) are dropped, rather than passed on to do something else.

Without an SD card, or if CP/M stops on an error (e.g. a Z80 `IN` or `OUT`, which aren't emulated),
it says so and restarts on a key press.
