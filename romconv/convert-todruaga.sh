#!/bin/bash

set -e

die() {

  echo "FAIL - FAIL - FAIL - FAIL - FAIL - FAIL - FAIL - FAIL"
  exit 1
}

#------------------------------------
# Tower of Druaga
#------------------------------------

if [[ -f ../romszip/todruaga.zip ]]; then
  #echo Tower of Druaga Logos
  #python3 ./logoconv.py ../logos/todruaga.png ../source/src/machines/todruaga/todruaga_logo.h || die

  echo Converting Tower of Druaga
  cd todruaga || die

  python3 ./todruaga_rom_convert.py || die

  cd ..
else
  die
fi
