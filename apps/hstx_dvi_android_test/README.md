# Android walk cycle

An android with flowing hair walking up and down, to try out sprites taller than 16 pixels.
All 8 frames of the walk are laid out above her.

## How she is drawn

- She is 32x48, drawn with `sprite_renderer_sprite_32xn_p1`, which takes a 32 bit row for each line of the sprite, however tall it is.
- Each colour is a sprite of its own: 6 for her body and 2 for her hair, so 8 in all.
- Her hair is a separate set of sprites with its own 8 frames, going at a different rate to the walk, so it keeps flowing.
  It is added before her body, so it is drawn over it, and moves down with her body as she steps.
- Turning round mirrors the art when the app starts.

## Changing her

The art is made by `art/android_art.py`, which poses her from a simple skeleton for each frame,
lowering her body so a foot is always on the ground, and works out how far she moves on each frame so her feet do not slide.
It writes `src/android_art.c` as rows of characters, a character for each colour, like the art in the games.

```sh
python3 art/android_art.py
```

## Running on the PC

```sh
host/build.sh
host/build/android_host attract 600 60 frames
```

See `../common/host` for more on how this works.
