# Text test (hstx_dvi_text_test)

An 80x60 grid of text in the 8x8 font, from `hstx_dvi_grid.h`, drawn a row at a time by core 1. It
shows:

- a palette of colours for each character's foreground and background,
- the attributes: normal, dim, underlined, reversed and blinking,
- the first lines of *Kubla Khan*, and a character in each corner,
- a count going up ten times a second, to show the screen being changed while it is shown.

It prints "HSTX DVI Text Test" on the USB serial port at start up. It needs the DVI connector on
GPIOs 12 to 19, as on the [Pico DVI Sock](https://github.com/Wren6991/Pico-DVI-Sock).

For a full terminal, with escape sequences, see the [VT test](../hstx_dvi_vt_test/README.md).
