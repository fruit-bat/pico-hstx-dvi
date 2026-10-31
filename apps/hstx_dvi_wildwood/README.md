# Wildwood

A flip screen maze adventure in the style of the 1984 jungle classic, using sprites.
Hazel the witch wanders a wood full of animals, and gets on by helping them rather than fighting them.

## Controls

Buttons connect the GPIO to ground (internal pull ups are used).

| GPIO | Button  |
|------|---------|
| 6    | Left    |
| 7    | Right   |
| 8    | Up      |
| 9    | Down    |
| 10   | Cast    |
| 11   | Tiptoe  |

- Hazel walks in eight directions and casts her spell the way she is facing.
- Hold tiptoe to creep along slowly and quietly.
- Cast or tiptoe starts a game.

GPIOs 2 to 5 are left free for the SD card, wired as in the hstx_dvi_cpm app.
The first button GPIO can be changed with `WW_INP_GPIO_START` in CMakeLists.txt.

Built with `-DGAME_NESPAD=ON` (see the top README), NES or SNES gamepads work as well: D-pad to walk, A to cast and B to tiptoe.

## The game

- The wood is a maze of 48 screens, made afresh each night. The map in the top right fills in as Hazel explores.
- The moon charm has been broken into four pieces, and each is earned a different way:
  - **Heal**: a hurt deer needs three spells to be well again.
  - **Calm**: a grumpy boar needs four spells to calm down.
  - **Help**: a lost owlet follows Hazel once found, and has to be led back to its mother.
  - **Hush**: the last piece lies beside a sleeping bear. Tiptoe to it without waking him.
- The other animals are happy, hurt (cast to heal), grumpy (cast to calm, and they bump into Hazel) or asleep (creep past for points, or they wake up grumpy).
- Hazel has hearts, lost when grumpy animals or the wolf bump into her. Lose them all and she is dizzy and loses a life, and her hats are her lives.
- Stay in one screen too long and the old wolf comes. Nothing calms him but the whole moon charm, though a spell makes him back off for a moment, and leaving the screen gets away from him.
- With the whole charm the wolf turns up quickly. Calm him to end the night, then the next one is harder.
- Magic flowers change colour as they bloom. Pink fills Hazel's hearts, yellow makes her speedy, cyan hushes her so nobody notices her, and purple muddles her controls.
- An extra life every 10000 points.

## Between games

The title with the folk of the wood and what they need, a demo game played by the Pico,
and the high score table with Hazel and her friends walking along the bottom.

## How it is drawn

Each screen is drawn by a renderer of its own that copies 32 pixel square tiles straight into the row,
so it costs the same however busy the screen is. The bushes and ponds have a picture for each way they can
join up with their neighbours, so they run together into rounded clumps. The pictures are made when the game starts.

The animals and Hazel are up to 32 pixels tall, made of a top and bottom half each with up to four colours.
They and everything else are on sprites handed out afresh every frame. Nearer things are given lower sprite ids, so they are drawn in front.

The colours, explosions, text and high score table are in `../common/game`, shared with the other games.

## Running on the PC

The game can be built and run on a Linux PC to try out changes without a Pico.
The sprite renderer runs in a second thread in place of core 1, and frames are saved as PPM images.

```sh
host/build.sh
host/build/wildwood_host play 6000 300 frames
```

The arguments are the mode, how many frames to run, how often to save one and where to put them.
The modes are `attract` (no buttons pressed), `play` (a game driven by the demo player),
`idle` (a game where Hazel stands still) and `charm` (as play, but with the whole charm from the start,
to see the end of a night). The score, lives, pieces and rooms visited are printed every 10 seconds of game time.

The demo player usually gets through the first night in about 10 minutes of game time.

The harness itself is in `../common/host`, shared with the other games; `host/scenario.c` is what presses the buttons.
