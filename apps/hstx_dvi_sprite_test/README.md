
Example application using lots of sprites

An invaders game. The library's pixel perfect sprite collisions decide what hits what.

## Controls

Buttons connect the GPIO to ground (internal pull ups are used).

| GPIO | Button |
|------|--------|
| 6    | Left   |
| 7    | Right  |
| 8    | Fire   |
| 9    | Start  |

GPIOs 2 to 5 are left free for the SD card, wired as in the hstx_dvi_cpm app.
The first button GPIO can be changed with `INV_INP_GPIO_START` in CMakeLists.txt.

## The game

- 40 invaders in three kinds, each made of three sprites in different colours:
  jellies (10), crabs (20) and one eyed commanders (40), which take two hits.
- They all keep an eye on the gun, blink now and then, and wave their tentacles,
  snap their claws or paddle their feet as they march.
- Crabs and commanders dive out of the formation after the gun, bombing on the way,
  and fly back round to their place. They are worth double while out of the formation.
- Each kind drops its own kind of bomb.
- Each wave starts a little lower and the invaders march, bomb, aim and dive more.
- The march speeds up as the invaders are thinned out, and the last one really goes for it.
- Invaders walking into the bases wipe them out. If they reach the gun the game is over.
- The mystery ship turns up every so often. Its score is not as random as it looks.
- A bonus life is awarded every 3000 points.
- The top 10 scores are kept, with initials, until the power is turned off.

## Between games

The attract mode cycles through:

1. The title, spelt out in invaders that fly in, wave about in a rainbow and fly off again
2. The score advance table, with the invaders showing off
3. A demo game played by the Pico
4. The high score table, with a parade along the bottom

Press fire or start at any time to play.

## Running on the PC

The game can be built and run on a Linux PC to try out changes without a Pico.
The sprite renderer runs in a second thread in place of core 1, and frames are saved as PPM images.

```sh
host/build.sh
host/build/invaders_host play 6000 300 frames
```

The arguments are the mode, how many frames to run, how often to save one and where to put them.
The modes are `attract` (no buttons pressed), `play` (a game driven by the demo player) and
`idle` (a game where the gun just sits there). The score, lives, wave and invaders left are
printed every 10 seconds of game time.

Each saved frame is about 1MB, so keep the gap between them large on long runs.

The harness is in `../common/host` and the colours, pictures, explosions and high score table
in `../common/game`, shared with the other games.
