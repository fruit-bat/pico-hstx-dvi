# Wireframe ships

The galactic and short range charts and the data on systems from Elite, then every type of ship from Elite flying round a spinning Coriolis space station, past a sun, through a field of stars,
with missiles chasing two of them. The name of the nearest ship is shown at the top, and a dashboard after the original game's at the bottom.
Every 10 seconds it changes between solid wireframes, see-through wireframes, filled faces with dark outlines, and plain filled faces,
the filled ones shown both with and without dithering.

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
- The screen has 8 shades of red and green and 4 of blue, so the faces of a ship can only be lit in a few steps.
  Dithered, a face mixes the two nearest colours in a pattern 2 pixels by 2, for 3 more shades between each step.
  A part's colour is a word of 4 pixels for the even rows and one for the odd, so this costs the renderer nothing,
  and the pattern stays fixed to the screen as the ships move.
- An object wholly behind the outline of something solid and nearer is left out, from its circle on the screen.
- The objects are added nearest first. Lower sprite ids take priority, so the nearer ones cover the ones behind without a depth buffer.
  This goes wrong only if objects pass through each other.
- Far away an object is just a dot, and in the wireframe styles a small one is just its outline. A convex outline crosses each row twice,
  where all the edges of a ship can cross it many times, and each costs about as much on a row however short it is.
- When outlined, the dark edges are only drawn between faces we can see, on objects at least 40 pixels across,
  as the outlines cost as much as a wireframe on top of the faces, and hardly show round the outside or on small ships.
- The sun is drawn after everything else so it is behind the ships, in a few objects so no sprite looks at too many parts on a row. It is an outline with rays that turn and flicker, made of parts like the ships,
  filled in black for the solid wireframes and in yellow when filled.
- The other core turns, moves, lights and projects everything each frame, with floats, and cuts the edges and shapes to the view.
  The renderer reads the sprites of the frame it is drawing while the next is worked out, so there are two sets.

## The dashboard

- On the left, gauges for the fore and aft shields, fuel, cabin and laser temperatures and altitude.
  On the right, speed, roll and dive or climb, and the four energy banks. The values are made up, and change over time.
- In the middle, the scanner, with a blip on a stalk for each ship within range, above or below where it is round us,
  a compass with a dot towards the station, red when it is behind us, and the missiles, two of them away.
- It is drawn in outlines for the wireframe styles, and with solid bars for the filled ones, with the same lines and shapes as the ships.
  Each gauge, blip and panel is an object of its own, as an object's sprite looks at all its parts on every row it covers.

## The charts

Before the ships come the galactic chart, the short range chart and the data on a system, with the next of the eight galaxies each time round.

- The galaxies are made as the original game made them, by `src/galaxy.c`: three numbers, turned over for each system in turn,
  give where it is, its name, what it makes and the rest, and twisted give the next galaxy. So Lave is where it should be,
  with Diso 3.6 light years away.
- The galactic chart is twice as wide as it is tall, as in the original. The systems are dots, added in bands of 8 rows
  so each sprite has only a few to look at on a row. The cross is where we are, Lave in the first galaxy, inside the circle we can jump to.
- The short range chart shows the systems round us as circles as big as their planets, coloured from red for rich industrial
  to blue for poor agricultural, with their names where there is room.
- The data on a system shows us first, then the nearest systems in turn: its economy, government, tech level,
  population and who lives there, and what it is known for, made up from its numbers as in Ian Bell's Text Elite,
  so Lave is most famous for its vast rain forests and the Lavian tree grub.
- Under the data is a picture of the system: its sun, its planet turning, as big as it is in km and coloured like it is on the chart,
  and the station going round it. As in the original game, the planet has a meridian and an equator or a crater on the side facing us.
- The charts and data are always drawn as wireframes, to look like data: see-through when that is the style,
  and solid otherwise. The dashboard under them still follows the style.
- The big text, such as the titles, is the font at twice the size smoothed with Scale2x, made when the app starts:
  each pixel becomes 4, and a corner of the 4 is filled where the pixels beside it would make a step. It draws as quickly as the plain font doubled.
- A yellow cursor moves from system to system, and the name of the one it is on and how far it is are shown at the bottom.

## The meter

The line at the top shows the style, and how well the renderer is keeping up with the display.

- `AHEAD` is how far ahead of the display the rows were at the worst moment of the last frame, in rows.
  The row FIFO holds 8 rows, and is full at the top of each frame after the vertical blank,
  so a run of slow rows uses up some of the lead and the faster rows after them win it back.
  If it goes below 0 a row was late and the display drops the frame.
- `AVG` is the time spent on the whole frame.

It is also printed over USB serial each second, with the slowest single row, the slowest run of 8 rows as `load`, and the most sprite calls in a run of 8 rows. The timing is turned on by `HSTX_DVI_SPRITE_TIMING`
in `CMakeLists.txt`, and uses the SysTick of the core doing the rendering, so it only works on Arm.

## The benchmark

Building with `WIREFRAME_BENCH` set, in `CMakeLists.txt`, runs a benchmark at the start. It times the parts of the renderer's work
on their own, such as filling a pixel or drawing a run of a line, shows the results for 20 seconds, and prints them over USB serial.

## Changing the models

The ships follow the shapes of those in the original game, though not exactly.
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
