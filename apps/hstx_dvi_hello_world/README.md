# Hello world (hstx_dvi_hello_world)

The simplest use of the library: a 640x480 picture of mountains, held in RAM as RGB332 (one byte a
pixel), shown on the DVI screen. `hstx_dvi_init()` is given a function that returns where each row of
pixels is, and HSTX does the rest, with both cores left idle.

The picture is [`images/mountains_640x480_rgb332.h`](images/mountains_640x480_rgb332.h), built with
`MODE_BYTES_PER_PIXEL=1`.

It needs the DVI connector on GPIOs 12 to 19, as on the
[Pico DVI Sock](https://github.com/Wren6991/Pico-DVI-Sock).
