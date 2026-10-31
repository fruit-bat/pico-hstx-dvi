# Moonlit Manor

A scrolling platform game in a haunted house, using sprites.
Hazel the witch has followed five lost kittens into the old manor, and gets past its spooks by cheering them up with spells rather than fighting them.

## Controls

Buttons connect the GPIO to ground (internal pull ups are used).

| GPIO | Button |
|------|--------|
| 6    | Left   |
| 7    | Right  |
| 8    | Up     |
| 9    | Down   |
| 10   | Cast   |
| 11   | Jump   |

- Hold jump for a full jump, or tap it for a little hop.
- Cast sends a spell the way Hazel is facing. Hold up to cast straight up.
- Down and jump together drops through a shelf.
- Jump or cast starts a game.

GPIOs 2 to 5 are left free for the SD card, wired as in the hstx_dvi_cpm app.
The first button GPIO can be changed with `MM_INP_GPIO_START` in CMakeLists.txt.

## The game

- The house is a cut away doll's house of a garden, hall, library, kitchen, cellar, landing, bedroom, gallery and attic, and the view scrolls smoothly round it after Hazel.
- Find all five kittens to end the night. Each is behind a puzzle:
  - **The hall**: a kitten on a high ledge. Cheer the ghost when it floats by and use it as a step.
  - **The library and kitchen**: climb the shelves to cast at the lever and open the way to the kitchen, where three candles open the trapdoor. One is lit from the floor, one at the top of a jump and one from the top shelf.
  - **The cellar**: push the crate into the hole by the gate to hold the plate down. A spell pulls a crate a tile towards Hazel, should it end up somewhere awkward. The key is down here too.
  - **The bedroom and gallery**: the key opens the bedroom, where the magic feather is up on the wardrobe. It lets Hazel float over the gallery's broken floor by holding jump.
  - **The attic**: two more candles open the way to the last kitten, one of them over a hole only a float will cross.
- The spooks are grumpy and bump into Hazel. A spell cheers them up for a while:
  - happy ghosts and pumpkins hold still and can be stood on,
  - bats go to sleep,
  - spiders climb back up their threads.

  They flash before getting grumpy again, but wait for Hazel to get off first.
- Hazel has hearts, lost when a grumpy spook bumps into her. Lose them all and she is dizzy and loses a life, and her hats are her lives.
- Lanterns light up as Hazel passes them, and she starts again at the last one lit after losing a life.
- Sweets, hearts, the key and the feather can be picked up, and there is a bonus at the end of each night. The next night spooks stay cheerful for less time.
- An extra life every 10000 points.

## Between games

The title with a tour of the house, who lives there and what to do about them, a demo game played by the Pico,
and the high score table with Hazel and the kittens walking along the bottom.

## How it is drawn

Everything is drawn twice its size, as in the old home computer games, so Hazel is 64 pixels tall.
The pictures have up to 15 colours at 4 bits a pixel and each is a single sprite, drawn straight into the row by a renderer of its own.
They can be flipped, made half see through (the grumpy ghosts) or drawn four times their size (Hazel on the title).
They don't take part in sprite priority, so they are added back to front.

The house is drawn by another renderer that copies rows of 32 pixel tiles into the row at any position, so the view can scroll
a pixel at a time each way. The tiles are made when they are first needed, from wallpaper for each floor with furniture drawn over it.
The sky behind the garden and roof is a pattern of stars that moves a quarter as fast, so it looks far away. Candles and lanterns flicker by
swapping their pictures every few frames.

The colours, explosions, text and high score table are in `../common/game`, shared with the other games.

## Running on the PC

The game can be built and run on a Linux PC to try out changes without a Pico.
The sprite renderer runs in a second thread in place of core 1, and frames are saved as PPM images.

```sh
host/build.sh
host/build/manor_host play 9000 300 frames
```

The arguments are the mode, how many frames to run, how often to save one and where to put them,
or set `HOST_FRAMES` to a list of the frames to save. The modes are:

- `attract`, with no buttons pressed.
- `play`, a game driven by the demo player, which goes all the way round the house and finds every kitten in about two minutes of game time.
- `idle`, a game where Hazel stands still.
- `tour`, which moves Hazel round the house to look at it.
- `script`, for trying out a jump or a puzzle. `MM_START` is the tile to start on, as in `MM_START=35,28`, and `MM_SCRIPT` the buttons to press, as in `MM_SCRIPT="R30 RJ12 .20 F1"`: each is the letters L R U D F J (or . for none) held for a number of frames. `MM_GIVE` can have `float` or `key4` in it, and `MM_TRACE=1` prints where Hazel is each frame.

The score, lives, kittens and where Hazel is are printed every 10 seconds of game time, and in `play` each step of the demo player's route.

The harness itself is in `../common/host`, shared with the other games; `host/scenario.c` is what presses the buttons.

## Changing the house

The house is the map in `src/mm_map.c`, a character for each tile, with a key at the top. Hazel can jump up 2 tiles,
or about 4 across. Shelves that end a single tile short of a wall make a gap too narrow for her to fall down,
so they should go right up to it or stop 2 short. What each gate's letter does is set in `_group_type`.
