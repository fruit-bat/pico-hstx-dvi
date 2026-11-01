# Elite

A game in the style of Elite, drawn in 3D with sprites, from the wireframe test. So far you can launch from the space station at Lave,
fly about, look at the charts of the galaxy and the data on its systems, jump to other systems through hyperspace, fight pirates, dock again, and trade.

## Between games

The title, with a Cobra Mk III turning, then the galactic chart, the short range chart, the data on the systems nearest us,
and every type of ship flying round a space station, the next galaxy each time round. Fire begins a game, and start shows the options.

## Docking

A game starts docked, with your status shown, as in the original. Fire launches you through the station's tunnel.
To dock again, fly into the slot from in front of it, pointing at the station, with the slot level:
the station spins, so roll to match it. Touching the station any other way, or the planet, is the end of the game.
Close in front of the slot, squares hang in space to guide you in, turning with the station, green when you are lined up
and amber when not. They are always drawn as wireframes.

A docking computer, sold from tech level 9, flies you in: out to a point in front of the slot, round the side first
if you are behind the station, then in along the line to the slot, rolling to keep it level. X on a SNES pad turns it
on and off, as does DOCKING in the options for other pads. Touching the pad or fire takes over from it.

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
fuel, missiles, a large cargo bay of 35 tonnes, the ECM, fuel scoops, a docking computer, and pulse, beam and military lasers, the last two of which fire all the time.
A laser can go at the front, rear, left or right, chosen with left and right, in place of a lesser one there.
Like Commander Jameson you start with 100 credits, a pulse laser, three missiles and no ECM.

## The overdrive

The sun is a long way off, so holding B and up on at full speed for half a second goes into the overdrive,
about nine times faster: the speed gauge goes red, past a mark near its end, and the sun is about 15 seconds from the planet.
It burns a tenth of a light year of fuel each second, so getting there costs about a light year and a half,
which fuel scoops soon win back. It cannot be used close to the planet, the sun, the station or another ship,
and drops out to full speed when near any of them, when out of fuel, or with B and down.

## The sun, fuel scoops and canisters

Close to the sun the cabin heats up, shown by CT on the dashboard; too hot, it drains your energy, and flying into the sun is the end.
With fuel scoops fitted, flying close enough to the sun fills the tanks as you go.
A destroyed trader or pirate may leave a cargo canister drifting. With fuel scoops, fly into it, aiming at or a little below the sights,
to pick it up, which puts a tonne, kilo or gram of something at random in the hold, if there is room, and shows its name;
otherwise it breaks against the ship, taking a little off the shields, with CARGO LOST, or NO FUEL SCOOPS if none are fitted.
The inventory, one of the screens select shows, lists the cargo in the hold, with the fuel and cash.

## Combat

Fire shoots the laser the way you look, if one is fitted there, which heats up as it fires and stops when too hot until it has cooled.
B held with fire launches a missile at the ship nearest the middle of the view, if any are left.
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
- B held with up or down speeds up or slows down. Held on at full speed for half a second, it goes into the overdrive.
- A fires the laser, launches you when docked, from the status screen, and in flight on the charts or data jumps to the system the cursor is on.
- B held with A launches a missile, and with select sets off the ECM.
- Select shows the next screen: the front, rear, left and right views, the inventory, the short range chart, the galactic chart,
  and the data on the system the cursor is on. The controls fly the ship the same whichever way you look.
  On the charts the pad moves the cursor, faster with B held.
- On a SNES pad, in flight, the shoulders look round the ship: the right one clockwise, front, right, rear and left,
  and the left one the other way. From a chart either goes back to the front view. X turns the docking computer on and off.
- Start shows the options: the way of drawing the view, solid wireframes, see-through wireframes, filled faces with dark outlines,
  or plain filled faces, whether the faces are dithered for more shades, the dashboard in lines or filled,
  the shape of the screen, and in a game, DOCKING, which turns on the docking computer, MAX EQUIP, which fits everything (for trying things out, to go when the game is done), and ending it. They can be changed on the title too.
- The display is 640 by 480, 4:3, and a 16:9 screen stretches it across. So that circles stay round,
  everything is drawn a quarter narrower for 16:9, which it starts with; 4:3 is for older screens that show it as it is.

A NES or SNES pad works when built with `GAME_NESPAD`, and buttons on GPIOs 4 to 11, each to ground,
in the order left, right, up, down, A, B, select and start, unless built with `GAME_BUTTONS` off.

## Saving

Built with `-DELITE_SDCARD=ON`, commanders are saved on an SD card on SPI0, GPIOs 2 to 5 as for `hstx_dvi_cpm`,
in the directory `pico_dvi_elite`, as `commander1.dat` to `commander4.dat`, and the options in `settings.dat`,
saved when you leave them and read when it starts.
Starting a game then shows the four slots: fire plays the commander in one, or a new one if it is empty,
and select starts a new one in it. Left for 30 seconds, it goes back to the title. A new commander is named first: up and down change the letter, held to go on, left and right
move along, and fire takes the name. The game is saved whenever you dock or launch, so always as docked, as in the original,
and a commander loaded starts docked where it was saved.
The SD card takes GPIOs 4 and 5, so the buttons move to GPIOs 6 to 11, without select and start, which come from a pad.

Without it, or without a card, the game starts straight away as Commander Jameson and nothing is saved.

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

`options` shows the options on the title, then starts a game and waits, which goes back to the title from the commanders.
`fly` starts a game, launches, flies about, looks at the charts, changes the options and fits everything.
`autodock` is put behind the station, facing away, and the docking computer, turned on in the options, flies it in.
`dock` and `crash` launch, then are put on a run at the slot, level or rolled, to dock or hit the station.
`hyper` jumps from Lave to Diso, then tries to go back without enough fuel.
`fight` puts a Sidewinder ahead and fires at it, then a Python and fires a missile at it.
`views` puts a Sidewinder behind with a rear laser fitted, looks back and fires at it, then looks left and right.
`overdrive` launches, climbs away, faces the sun and holds B and up, going into the overdrive once clear of the station, until near the sun.
`scoop` is put beside the sun with fuel scoops and an empty tank, fills it, scoops up a canister a little above the nose, shows the inventory, then dives into the sun.
`trade` buys and sells in the market at Lave, buys a missile, and tries for a rear pulse laser.
Built with `EL_HOST_SAVE` defined, commanders are saved in `pico_dvi_elite` where it is run, and `save` picks slot 2,
names it NAMESON, buys food and launches, saving it, to show in the slot when run again,
then fills the dashboard in the options, which stays filled when run again.
See `../common/host` for more on how this works.
