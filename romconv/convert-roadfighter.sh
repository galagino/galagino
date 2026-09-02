#!/bin/bash

set -e

die() {

  echo "FAIL - FAIL - FAIL - FAIL - FAIL - FAIL - FAIL - FAIL"
  exit 1
}

#------------------------------------
# Road Fighter
#------------------------------------

if [[ -f ../romszip/roadf2.zip ]]; then
  #echo Road Fighter Logos
  #python3 ./logoconv.py ../logos/roadfighter.png ../source/src/machines/roadfighter/roadfighter_logo.h || die

  echo Converting Road Fighter
  cd roadfighter || die

  python3 ./roadfighter_rom_convert.py || die

  cd ..
else
  die
fi
