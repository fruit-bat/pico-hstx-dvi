# Wireframe ships

A Coriolis space station, Cobra Mk IIIs and missiles in the style of Elite, to try out drawing 3D with the sprites.
The station spins with its docking slot towards us while four ships fly round it, two of them chased by missiles.
Every 10 seconds it changes between solid wireframes, see-through wireframes, filled faces with dark outlines, and plain filled faces.

## How it works

- A sprite is just a box on the screen and a callback that draws one row of it, so each object is a sprite,
  with a list of the lines and convex shapes that make it up, its parts.
  It is drawn by `sprite_renderer_parts` in the renderer, which works out the run of each part that falls on a row,
  from the row number alone, and fills 4 pixels at a time where nothing has drawn yet.
- The shapes are cut into bands a few rows deep, and fill from the left side of the band to the right side.
- A pixel can only be drawn once, so the parts earlier in the list keep priority over the later ones, as the sprites do.
- It started with a sprite for each edge, but each call to a sprite's callback costs around 200 cycles whatever it draws,
  which was most of the time. With a sprite for each object the scene needs far fewer calls, and fewer than 50 sprites.
- The faces at the back are left out as in the original game, and an edge is only drawn if one of the two faces it joins is facing us.
  The models are convex, so the faces left never cover each other.
- For the solid wireframes each object's outline is filled in black after its edges. A convex object always has a convex outline,
  so this takes one shape. Close up, when part of an object is behind us, each face is filled instead.
- The objects are added nearest first. Lower sprite ids take priority, so the nearer ones cover the ones behind without a depth buffer.
  This goes wrong only if objects pass through each other.
- Far away an object is just a dot.
- The other core turns, moves, lights and projects everything each frame, with floats, and cuts the edges and shapes to the view.
  The renderer reads the sprites of the frame it is drawing while the next is worked out, so there are two sets.

## The meter

The bottom line shows how many parts make up the scene, and how hard the renderer is working, as a share of the time it has.

- `LOAD` is the slowest run of 8 rows in the last frame. The row FIFO holds 8 rows, so a slow row can borrow time from the rows around it,
  but if a run of 8 goes over 100% the display misses a row and drops the frame.
- `AVG` is the time spent on the whole frame.

It is also printed over USB serial each second, with the slowest single row and the most sprite calls in a run of 8 rows. The timing is turned on by `HSTX_DVI_SPRITE_TIMING`
in `CMakeLists.txt`, and uses the SysTick of the core doing the rendering, so it only works on Arm.

## The benchmark

Building with `WIREFRAME_BENCH` set, in `CMakeLists.txt`, runs a benchmark at the start. It times the parts of the renderer's work
on their own, such as filling a pixel or drawing a run of a line, shows the results for 20 seconds, and prints them over USB serial.

## Changing the models

The faces and edges are worked out from the corners of each hull by `art/ship_model.py`,
which adds details such as the engines as shapes on the faces they sit on, or as lines, like the gun and the missiles' fins.
It writes `src/ship_model.c`.

```sh
python3 art/ship_model.py
```

## Running on the PC

```sh
host/build.sh
host/build/wireframe_host attract 2400 120 frames
```

The meter is not shown on the PC. See `../common/host` for more on how this works.
