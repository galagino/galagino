#!/bin/bash

set -e

die() {

  echo "FAIL - FAIL - FAIL - FAIL - FAIL - FAIL - FAIL - FAIL"
  exit 1
}

#------------------------------------
# Donkey Kong 3
#------------------------------------
if [[ -f ../romszip/dkong3.zip ]]; then
  echo Donkey Kong 3 Unpack roms
  python3 ./unpack.py dkong3.zip || die

  #echo Donkey Kong 3 Logos
  #python3 ./logoconv.py ../logos/dkong3.png ../source/src/machines/dkong3/dkong3_logo.h || die

  cd dkong3 || die

  echo Donkey Kong 3 CPU code
  python3 ./cpu_conv.py
  python3 ./sound_conv.py

  echo Donkey Kong 3 Tiles
  python3 ./tilemap_conv.py || die
  #python3 ./view_tiles_graphic.py || die

  echo Donkey Kong 3 Colormaps
  python3 ./cmap_conv.py || die 
  python3 ./color_codes_conv.py || die

  echo Donkey Kong 3 Sprites
  python3 ./sprites_conv.py || die

  cd ..
else
  die
fi
