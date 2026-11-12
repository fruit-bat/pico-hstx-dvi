# Row FIFO test (hstx_dvi_row_fifo_test)

Draws the screen a row at a time, rather than from a frame buffer: core 1 fills each row with a
colour pattern, in RGB565 (`MODE_BYTES_PER_PIXEL=2`), and passes it to HSTX through the row FIFO
(`hstx_dvi_row_fifo.h`, with the rows from `hstx_dvi_row_buf.h`). Only a few rows are in RAM at once.

It also tests that the display recovers when the FIFO runs dry: for the first 200 frames, core 1 stops
for 10 ms near the bottom of the screen, so those frames are missing rows. After that the pattern
should be steady.

It prints "HSTX DVI Row FIFO Test" on the USB serial port at start up. It needs the DVI connector on
GPIOs 12 to 19, as on the [Pico DVI Sock](https://github.com/Wren6991/Pico-DVI-Sock).
