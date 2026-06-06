#!/bin/bash

set -e

die() {

  echo "FAIL - FAIL - FAIL - FAIL - FAIL - FAIL - FAIL - FAIL"
  exit 1
}

#------------------------------------
# Scramble
#------------------------------------

if [[ -f ../romszip/scramble.zip ]]; then
  #echo Scramble Logos
  #python3 ./logoconv.py ../logos/scramble.png ../source/src/machines/scramble/scramble_logo.h || die

  echo Converting Scramble
  cd scramble || die
  python3 ./scramble_rom_convert.py || die
  cd ..


fi
