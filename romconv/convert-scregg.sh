#!/bin/bash

set -e

die() {

  echo "FAIL - FAIL - FAIL - FAIL - FAIL - FAIL - FAIL - FAIL"
  exit 1
}

#------------------------------------
# Scrambled Egg
#------------------------------------

if [[ -f ../romszip/scregg.zip ]]; then
  #echo Scrambled Egg Logos
  #python3 ./logoconv.py ../logos/scregg.png ../source/src/machines/scregg/scregg_logo.h || die

  echo Converting Scrambled Egg
  cd scregg || die

  python3 ./scregg_rom_convert.py || die

  cd ..
else
  die
fi
