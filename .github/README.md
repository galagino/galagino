Galagino
========

This is my custom Galagino build.

It has all the games from [speckhoiler](https://github.com/speckhoiler/galagino) and additional games from [SurvivalHacking](https://github.com/SurvivalHacking/galagino3) plus Moon Cresta, Scramble and Super Cobra.


### Quality of Life, improvements and fixes

* `TFT_VFLIP` logic reworked to keep track of state and work across ST7789 and ILI9341

* `TFT_INVERT` added to support CYD clones that show inverted colors

* `m6809` emulation uses machineBase methods, so you can have multiple instances just like the `Z80` and `i8048`

* Time Pilot sprite multiplexing

* Bluetooth Controller over i2c -> [galagino-controller](https://github.com/galagino/galagino-controller)

* Support for ESP32-S3

* Support for External DAC (es8311)

* Linux `romconv` scripts

* `pengo.zip`, `pengoj.zip` romsets conversion

* File generation without unziping, some roms cause name clashes and is much cleaner. Not all roms yet.

* mos6502 emulation (WIP)



### Hardware Used

| Board    | Link                                                            | Amazon                                  | Notes               |
| ---      | ---                                                             | ---                                     | ---                 |
| fnk0103b | [github](https://github.com/Freenove/Freenove_ESP32_Display)    | [Amzn](https://amazon.es/dp/)           | ST7789 - SPI 80MHz  |
|          |                                                                 |                                         |                     |
| fnk0103f | [github](https://github.com/Freenove/Freenove_ESP32_Display)    | [Amzn](https://amazon.es/dp/)           | ILI9341 - SPI 60MHz |
|          |                                                                 |                                         |                     |
| fnk0104a | [github](https://github.com/Freenove/Freenove_ESP32_S3_Display) | [Amzn](https://amazon.es/dp/B0FSQLPQ6M) | ESP32-S3 - IPS Display - External DAC |
|          |                                                                 |                                         | ILI9341 - SPI 60MHz - 16MiB Flash     |

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
| Super Cobra (scobra.zip)       | ![_](/logos/supercobra.png)   | ![_](/images/supercobra.png)   |       |
| Donkey Kong 3 (dkong3.zip)     | ![_](/logos/dkong3.png)       | ![_](/images/dkong3.png)       |       |
| Pooyan (pooyan.zip)            | ![_](/logos/pooyan.png)       | ![_](/images/pooyan.png)       |       |
| Phoenix (phoenix.zip)          | ![_](/logos/phoenix.png)      | ![_](/images/phoenix.png)      |       |

### ...
