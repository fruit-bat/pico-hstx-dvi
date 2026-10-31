# Wireframe ships

Cobra Mk IIIs in the style of Elite, to try out drawing 3D with the sprites.
A big one turns in the middle while two more fly round it, through a field of stars.

## How it works

- A sprite is just a box on the screen and a callback that draws one row of it, so each edge of a ship is a sprite of its own.
  Its box is the edge's bounding box and its callback draws the short run of the line that falls on that row,
  working it out from the row number alone. The renderer only calls the edges that cross each line.
- The edges at the back are hidden as in the original game. An edge is only drawn if one of the two faces it joins is facing us.
  The ships are convex, so no sorting or depth buffer is needed.
- The other core turns, moves and projects the ships each frame, with floats, and cuts the edges to the view.
  The renderer reads the edges of the frame it is drawing while the next is worked out, so there are two sets.
- The number of edges drawn is shown at the bottom. The sprites run out at 255.

## Changing the ship

The faces and edges are worked out from the corners of the hull by `art/ship_model.py`,
which adds the engines, vents and gun as extra edges on the faces they sit on. It writes `src/ship_model.c`.

```sh
python3 art/ship_model.py
```

## Running on the PC

```sh
host/build.sh
host/build/wireframe_host attract 600 60 frames
```

See `../common/host` for more on how this works.
