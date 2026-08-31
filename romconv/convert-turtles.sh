#!/bin/bash

set -e

die() {

  echo "FAIL - FAIL - FAIL - FAIL - FAIL - FAIL - FAIL - FAIL"
  exit 1
}

#------------------------------------
# Turtles
#------------------------------------

if [[ -f ../romszip/turtles.zip ]]; then
  #echo Turtles Logos
  #python3 ./logoconv.py ../logos/turtles.png ../source/src/machines/turtles/turtles_logo.h || die

  echo Converting Turtles
  cd turtles || die

  python3 ./turtles_rom_convert.py || die

  cd ..
else
  die
fi
