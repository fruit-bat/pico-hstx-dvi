# Elite

A game in the style of Elite, drawn in 3D with sprites, from the wireframe test. So far you can launch from the space station at Lave,
fly about, look at the charts of the galaxy and the data on its systems, jump to other systems through hyperspace, fight pirates, dock again, and trade.

## Between games

The title, with a Cobra Mk III turning, then the galactic chart, the short range chart, the data on the systems nearest us,
and every type of ship flying round a space station, the next galaxy each time round. Start or fire begins a game.

## Docking

A game starts docked, with your status shown, as in the original. Fire launches you through the station's tunnel.
To dock again, fly into the slot from in front of it, pointing at the station, with the slot level:
the station spins, so roll to match it. Touching the station any other way, or the planet, is the end of the game.
Close in front of the slot, squares hang in space to guide you in, turning with the station, green when you are lined up
and amber when not. They are always drawn as wireframes.

## Hyperspace

On the charts or the data, fire starts a countdown of 10 seconds to the system the cursor is on,
if it is within reach of the fuel, shown by the circle round us on the charts. The jump uses as much fuel as it is long,
up to 7 light years with full tanks, and fuel is bought at the station.
It cannot start, or end, too close to anything, the planet, the sun, the station or another ship: we are mass locked.
Each system's planet is as big as its data says and coloured as it is on the charts,
and where its sun and station are comes from its numbers, so it is the same each time.

## Trading

Docked, select also shows the market and the equipment for sale. In the market, up and down pick a good,
right buys and left sells, held to go on. The goods, and how their prices and how many are for sale depend on the economy,
moved a little each time we arrive, are as in Ian Bell's Text Elite. Goods sold by the tonne fill the hold, of 20 tonnes;
gold, platinum and gem-stones take no room. The equipment for sale depends on the tech level:
fuel, missiles, a large cargo bay of 35 tonnes, the ECM, and beam and military lasers, which fire all the time.
Like Commander Jameson you start with 100 credits, a pulse laser, three missiles and no ECM.

## Combat

Fire shoots the pulse laser, which heats up as it fires and stops when too hot until it has cooled.
B held with fire launches a missile at the ship nearest ahead, if any are left.
B held with select sets off the ECM, if fitted, which blows up every missile flying.

The other ships are traders going about their business, pirates that come for you,
and the police, who come for you if you destroy anyone but a pirate. Anyone shot at shoots back, or runs if it is a trader badly hurt.
Each type of ship has a character, loosely after the original:

- Light fighters, the Sidewinder, Gecko and Worm, are fast and weave as they come, small and hard to hit, but fragile and weak.
- The Mamba, Krait, Adder and Cobra Mk I are all-rounders.
- The Asp Mk II and Fer-de-Lance are heavy fighters: fast, tough and hard hitting, for a big bounty.
- The Python, Boa and Anaconda are slow and turn badly, but are tough, hit hard from a long way off and keep their distance,
  and may fire missiles: deadly head on, but easy to get behind.
- The police fly Vipers, fast and accurate.

Each system has a character from its data. Where the government is weak there are more pirates, and more of them heavy fighters;
where it is strong, more police. The more people, the busier it is, and industrial systems see the big freighters.
Hits take your shields down, fore or aft, then your energy, and they build up again slowly. With no energy left the game is over.
Pirates bring a bounty, and the ships destroyed raise your rating, as in the original.

## The controls

- Left and right roll, and up and down dive and climb, as with a joystick.
- B held with up or down speeds up or slows down.
- A fires the laser, launches you when docked, from the status screen, and in flight on the charts or data jumps to the system the cursor is on.
- B held with A launches a missile, and with select sets off the ECM.
- Select shows the next screen: the front view, the short range chart, the galactic chart, and the data on the system
  the cursor is on. On the charts the pad moves the cursor, faster with B held.
- Start shows the options: the way of drawing, solid wireframes, see-through wireframes, filled faces with dark outlines,
  or plain filled faces, whether the faces are dithered for more shades, the shape of the screen, and ending the game.
- The display is 640 by 480, 4:3, and a 16:9 screen stretches it across. So that circles stay round,
  everything is drawn a quarter narrower for 16:9, which it starts with; 4:3 is for older screens that show it as it is.

A NES or SNES pad works when built with `GAME_NESPAD`, and buttons on GPIOs 4 to 11, each to ground,
in the order left, right, up, down, A, B, select and start.

## How it works

- `el_draw` is the drawing of the wireframe test: each object is a sprite, with a list of the lines and convex shapes that make it up.
  See the wireframe test's README for how it works and what it costs.
- `el_space` holds where things are and which way they face, the dust that streams past as we fly, and the planets.
  Each speck of dust is a streak from where it was a few frames before, so the faster we go or turn the longer they are,
  which shows how fast we are going.
  As in the original game we are always at the middle of things, and everything is seen along our ship's axes.
  A planet's edge is worked out in 3D, as the circle on it where our line of sight just touches it, so it is right however close we are,
  and it has more sides the bigger it looks.
- `el_hud` is the dashboard, `el_chart` the charts and data, and `galaxy.c` makes the galaxies as the original game did.
- `el_attract` is what is shown between games, and `el_game` the game.

## The meter

The line at the top shows how well the renderer is keeping up, as in the wireframe test, and it is printed over USB serial each second.
It is turned on by `HSTX_DVI_SPRITE_TIMING` in `CMakeLists.txt`.

## Running on the PC

```sh
host/build.sh
host/build/elite_host attract 3600 300 frames
host/build/elite_host fly 1500 100 frames
host/build/elite_host dock 420 20 frames
```

`fly` starts a game, launches, flies about, looks at the charts and changes the options.
`dock` and `crash` launch, then are put on a run at the slot, level or rolled, to dock or hit the station.
`hyper` jumps from Lave to Diso, then tries to go back without enough fuel.
`fight` puts a Sidewinder ahead and fires at it, then a Python and fires a missile at it.
`trade` buys and sells in the market at Lave, and buys a missile.
See `../common/host` for more on how this works.
