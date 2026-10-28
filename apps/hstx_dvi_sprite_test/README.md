
Example application using lots of sprites

An invaders game: 220 invaders, all of them sprites.

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

- Each wave starts a little lower and the invaders march, bomb and aim faster.
- The march speeds up as the invaders are thinned out, and the last one really goes for it.
- Invaders walking into the bases wipe them out. If they reach the gun the game is over.
- The mystery ship turns up every so often. Its score is not as random as it looks.
- A bonus life is awarded every 5000 points.
- The top 10 scores are kept, with initials, until the power is turned off.

## Between games

The attract mode cycles through:

1. The title, spelt out in invaders that fly in, wave about in a rainbow and fly off again
2. The score advance table
3. A demo game played by the Pico
4. The high score table, with a parade along the bottom

Press fire or start at any time to play.
