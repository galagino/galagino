Galagino
========

This is my custom Galagino build.

It has all the games from [speckhoiler](https://github.com/speckhoiler/galagino) and additional games from [SurvivalHacking](https://github.com/SurvivalHacking/galagino3) plus Moon Cresta and Scramble.


### Quality of Life, improvements and fixes

* `TFT_VFLIP` logic reworked to keep track of state and work across ST7789 and ILI9341

* `TFT_INVERT` added to support CYD clones that show inverted colors

* `M6809` emulation uses machineBase methods, so you can have multiple instances just like the `Z80` and `i8048`

* Time Pilot sprite multiplexing

* Bluetooth Controller over i2c -> [galagino-controller](https://github.com/galagino/galagino-controller)

* Support for ESP32-S3

* Support for External DAC (es8311)

* Linux `romconv` scripts

* `pengo.zip`, `pengoj.zip` romsets conversion

### Hardware Used

| Board    | Link | Amazon |
| ---      | ---  | ---    |
| fnk0103b |      |        |
|          |      |        |
| fnk0103f |      |        |
|          |      |        |
| fnk0104  |      |        |
|          |      |        |

### 3d printed enclosure

I've used [Gavin Knight's](https://www.hackster.io/dynamight/cyd-galagino-arcade-cabinet-369ce9) very nice enclosure.

### Games

| Game                           | Marque                        | Screenshot                     | Notes |
| ---                            | ---                           | ---                            | ---   |
| Pac-Man (pacman.zip)           | ![ ](/logos/pacman.png)       | ![ ](/images/pacman.gif)       |       |
| Galaga (galaga.zip)            | ![ ](/logos/galaga.png)       | ![ ](/images/galagino.gif)     |       |
| Dig Dug (digdug.zip)           | ![ ](/logos/digdug.png)       | ![ ](/images/digdug.png)       |       |
| Frogger (frogger.zip)          | ![ ](/logos/frogger.png)      | ![ ](/images/frogger.png)      |       |
| Donkey Kong (dkong.zip)        | ![ ](/logos/dkong.png)        | ![ ](/images/dkong.gif)        |       |
| 1942 (1942.zip)                | ![ ](/logos/1942.png)         | ![ ](/images/1942.png)         |       |
| Lizard Wizard (lizwiz.zip)     | ![ ](/logos/lizwiz.png)       | ![ ](/images/lizwiz.png)       |       |
| Eyes (eyes.zip)                | ![ ](/logos/eyes.png)         | ![ ](/images/eyes.png)         |       |
| Mr. TNT (mrtnt.zip)            | ![ ](/logos/mrtnt.png)        | ![ ](/images/mrtnt.png)        |       |
| The Glob (theglobp.zip)        | ![ ](/logos/theglob.png)      | ![ ](/images/theglob.png)      |       |
| Crush Roller (crush.zip)       | ![ ](/logos/crush.png)        | ![ ](/images/crush.png)        |       |
| Ant Eater (anteater.zip)       | ![ ](/logos/anteater.png)     | ![ ](/images/anteater.png)     |       |
| Bombjack (bombjack.zip)        | ![ ](/logos/bombjack.png)     | ![ ](/images/bombjack.png)     |       |
| Mr. Do! (mrdo.zip)             | ![ ](/logos/mrdo.png)         | ![ ](/images/mrdo.png)         |       |
| Bagman (bagmanm2.zip)          | ![ ](/logos/bagman.png)       | ![ ](/images/bagman.png)       |       |
| Pengo (pengo2u.zip)            | ![ ](/logos/pengo.png)        | ![ ](/images/pengo.png)        |       |
| MsPacman (mspacman.zip)        | ![ ](/logos/mspacman.png)     | ![ ](/images/mspacman.png)     |       |
| Galaxian (galaxian.zip)        | ![ ](/logos/galaxian.png)     | ![ ](/images/galaxian.png)     |       |
| LadyBug (ladybug.zip)          | ![ ](/logos/ladybug.png)      | ![ ](/images/ladybug.png)      |       |
| Space Invaders (invaders.zip)  | ![ ](/logos/invaders.png)     | ![ ](/images/invaders.png)     |       |
| Time Pilot (timeplt.zip)       | ![ ](/logos/timeplt.png)      | ![ ](/images/timeplt.png)      |       |
| Gyruss (gyruss.zip)            | ![ ](/logos/gyruss.png)       | ![ ](/images/gyruss.png)       |       |
| Tutankham (tutankhm.zip)       | ![ ](/logos/tutankhm.png)     | ![ ](/images/tutankham.png)    |       |
| Donkey Kong Jr. (dkongjrj.zip) | ![ ](/logos/dkongjr.png)      | ![ ](/images/dkongjr.png)      |       |
| Star Force (starforc.zip)      | ![ ](/logos/starforce.png)    | ![ ](/images/starforce.png)    |       |
| Moon Cresta (mooncrst.zip)     | ![ ](/logos/mooncresta.png)   | ![ ](/images/mooncresta.png)   |       |
| Scramble (scramble.zip)        | ![_](/logos/scramble.png)     | ![_](/images/scramble.png)     |       |

### ...
