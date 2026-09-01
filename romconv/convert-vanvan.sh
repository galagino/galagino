#!/bin/bash

set -e

die() {

  echo "FAIL - FAIL - FAIL - FAIL - FAIL - FAIL - FAIL - FAIL"
  exit 1
}

#------------------------------------
# Van Van Car
#------------------------------------

if [[ -f ../romszip/vanvan.zip ]]; then
  echo Van Van Car Unpack roms
  python3 ./unpack.py vanvan.zip || die

  #echo Van Van Car Logos
  #python3 ./logoconv.py ../logos/vanvan.png ../source/src/machines/vanvan/vanvan_logo.h || die

  echo "Van Van Car CPU code (main bank 0x0000-0x3fff)"
  python3 ./romconv.py vanvan_rom ./roms/van-1.50 ./roms/van-2.51 ./roms/van-3.52 ./roms/van-4.53 ../source/src/machines/vanvan/vanvan_rom.h || die

  echo "Van Van Car CPU code (extra bank 0x8000-0x8fff)"
  python3 ./romconv.py vanvan_rom2 ./roms/van-5.39 ../source/src/machines/vanvan/vanvan_rom2.h || die

  echo Van Van Car Tiles
  python3 ./tileconv.py vanvan_tilemap ./roms/van-20.18 ../source/src/machines/vanvan/vanvan_tilemap.h || die

  echo Van Van Car Sprites
  python3 ./spriteconv.py vanvan_sprites pacman ./roms/van-21.19 ../source/src/machines/vanvan/vanvan_spritemap.h || die

  echo Van Van Car Colormaps
  python3 ./cmapconv.py vanvan_colormap ./roms/6331-1.6 0 ./roms/6301-1.37 ../source/src/machines/vanvan/vanvan_cmap.h || die
fi

