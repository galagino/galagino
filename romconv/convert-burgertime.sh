#!/bin/bash

set -e

die() {

  echo "FAIL - FAIL - FAIL - FAIL - FAIL - FAIL - FAIL - FAIL"
  exit 1
}

#------------------------------------
# Burger Time
#------------------------------------

if [[ -f ../romszip/btime.zip ]]; then
  #echo Burger Time Logos
  #python3 ./logoconv.py ../logos/burgertime.png ../source/src/machines/burgertime/burgertime_logo.h || die

  echo Converting Burger Time
  cd burgertime || die

  python3 ./burgertime_rom_convert.py || die

  cd ..
else
  die
fi
