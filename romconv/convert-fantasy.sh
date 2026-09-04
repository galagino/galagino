#!/bin/bash

set -e

die() {

  echo "FAIL - FAIL - FAIL - FAIL - FAIL - FAIL - FAIL - FAIL"
  exit 1
}

#------------------------------------
# Fantasy
#------------------------------------

if [[ -f ../romszip/fantasyu.zip ]]; then
  #echo Fantasy Logos
  #python3 ./logoconv.py ../logos/fantasy.png ../source/src/machines/fantasy/fantasy_logo.h || die

  echo Converting Fantasy
  cd fantasy || die

  python3 ./fantasy_rom_convert.py || die

  cd ..
else
  die
fi
