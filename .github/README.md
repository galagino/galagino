Galagino
========

This is my custom Galagino build.

It has Moon Cresta, Scramble and Super Cobra and the games from [speckhoiler/galagino](https://github.com/speckhoiler/galagino), from [SurvivalHacking/galagino3](https://github.com/SurvivalHacking/galagino3), [SurvivalHacking/spinnerino](https://github.com/SurvivalHacking/spinnerino), [VirtualClaudioBoy/GalaginoPlus](https://github.com/VirtualClaudioBoy/GalaginoPlus) and [BaasPierre/GalaginoPlusGoldstar](https://github.com/BaasPierre/GalaginoPlusGoldstar).


### Quality of Life, improvements and fixes

* `TFT_VFLIP` logic reworked to keep track of state and work across ST7789 and ILI9341

* `TFT_INVERT` added to support CYD clones that show inverted colors

* `m6809` emulation uses machineBase methods, so you can have multiple instances just like the `Z80` and `i8048`

* Time Pilot sprite multiplexing

* Bluetooth Controller over i2c see: [galagino-controller](https://github.com/galagino/galagino-controller)

* Bluetooth Controller supports multiple action/fire/bomb buttons.

* Support for ESP32-S3 CYD clone with 16MiB Flash

* Support for External DAC (es8311). ESP32-S3's don't have internal DACs.

* Linux `romconv` scripts.

* `pengo.zip`, `pengoj.zip` romsets conversion (which is the one with the popcorn music).

* Enabled machines selection moved to `platformio.ini`

* File generation without unziping, some roms cause name clashes and is much cleaner. Not all roms yet.

* Flash and PSRAM SPI configs for maximum speed available on each ESP32 version.

* Many code cleanups for reduced RAM used - with 44 games around 260k free heap (Flash is the limiting factor, you need and ESP32 with 8MiB of flash).

* mos6502 emulation (WIP)

### Limitations

* You need an ESP32 board with at least 8MiB of flash to have a build with all the games. See below for an ESP32-S3 CYD clone with 16MiB Flash.


### Hardware Used

| Board    | Link                                                            | Amazon                                  | Notes               |
| ---      | ---                                                             | ---                                     | ---                 |
| fnk0103b | [github](https://github.com/Freenove/Freenove_ESP32_Display)    | [Amzn](https://amazon.es/dp/)           | ST7789 - SPI 80MHz  |
|          |                                                                 |                                         |                     |
| fnk0103f | [github](https://github.com/Freenove/Freenove_ESP32_Display)    | [Amzn](https://amazon.es/dp/)           | ILI9341 - SPI 40MHz |
|          |                                                                 |                                         |                     |
| fnk0104a | [github](https://github.com/Freenove/Freenove_ESP32_S3_Display) | [Amzn](https://amazon.es/dp/B0FSQLPQ6M) | ESP32-S3 - IPS Display - External DAC |
|          |                                                                 |                                         | ILI9341 - SPI 40MHz - 16MiB Flash     |

### 3d printed enclosure

I've used [Gavin Knight's](https://www.hackster.io/dynamight/cyd-galagino-arcade-cabinet-369ce9) very nice enclosure.

### Games

| Game                           | Marquee                       | Screenshot                     | Notes |
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
| Burger Time (btime.zip)        | ![_](/logos/burgertime.png)   | ![_](/images/burgertime.png)   |       |
| Xevious (xevious.zip)          | ![_](/logos/xevious.png)      | ![_](/images/xevious.png)      |       |
| Bump 'n' Jump (bnj.zip)        | ![_](/logos/bnj.png)          | ![_](/images/bnj.png)          |       |
| Mappy (mappy.zip)              | ![_](/logos/mappy.png)        | ![_](/images/mappy.png)        |       |
| Gaplus (gaplus.zip)            | ![_](/logos/gaplus.png)       | ![_](/images/gaplus.png)       |       |
| Alibaba (alibaba.zip)          | ![_](/logos/alibaba.png)      | ![_](/images/alibaba.png)      |       |
| Amidar (amidar.zip)            | ![_](/logos/amidar.png)       | ![_](/images/amidar.png)       |       |
| Turtles (turtles.zip)          | ![_](/logos/turtles.png)      | ![_](/images/turtles.png)      |       |
| Circus Charlie (circusc.zip)   | ![_](/logos/circusc.png)      | ![_](/images/circusc.png)      |       |
| Roc'n Rope (rocnrope.zip)      | ![_](/logos/rocnrope.png)     | ![_](/images/rocnrope.png)     |       |
| Tower of Druaga (todruaga.zip) | ![_](/logos/todruaga.png)     | ![_](/images/todruaga.png)     |       |
| Van Van Car (vanvan.zip)       | ![_](/logos/vanvan.png)       | ![_](/images/vanvan.png)       |       |
| Pinball Action (pbaction.zip)  | ![_](/logos/pbaction.png)     | ![_](/images/pbaction.png)     |       |
| Road Fighter (roadf2.zip)      | ![_](/logos/roadfighter.png)  | ![_](/images/roadfighter.png)  |       |                
| Motorace USA (motorace.zip)    | ![_](/logos/motorace.png)     | ![_](/images/motorace.png)     | X 256 x 240 Y |

### ...
