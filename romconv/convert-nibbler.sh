#!/bin/bash

set -e

die() {

  echo "FAIL - FAIL - FAIL - FAIL - FAIL - FAIL - FAIL - FAIL"
  exit 1
}

#------------------------------------
# Nibbler
#------------------------------------

if [[ -f ../romszip/nibblerp.zip ]]; then
  #echo Nibbler Logos
  #python3 ./logoconv.py ../logos/nibbler.png ../source/src/machines/nibbler/nibbler_logo.h || die

  echo Converting Nibbler
  cd nibbler || die

  python3 ./nibbler_rom_convert.py || die

  cd ..
else
  die
fi
