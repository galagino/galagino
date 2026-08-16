#!/bin/bash

set -e

die() {

  echo "FAIL - FAIL - FAIL - FAIL - FAIL - FAIL - FAIL - FAIL"
  exit 1
}

#------------------------------------
# Xevious
#------------------------------------

if [[ -f ../romszip/xevious.zip ]]; then
  #echo Xevious Logos
  #python3 ./logoconv.py ../logos/xevious.png ../source/src/machines/xevious/xevious_logo.h || die

  echo Converting Burger Time
  cd xevious || die

  python3 ./xevious_rom_convert.py || die

  cd ..
else
  die
fi
