#!/bin/bash

set -e

die() {

  echo "FAIL - FAIL - FAIL - FAIL - FAIL - FAIL - FAIL - FAIL"
  exit 1
}

#------------------------------------
# Moon Cresta
#------------------------------------

if [[ -f ../romszip/mooncrst.zip ]]; then
  #echo Moon Cresta Logos
  #python3 ./logoconv.py ../logos/mooncresta.png ../source/src/machines/mooncresta/mooncresta_logo.h || die

  echo Moon Cresta Unpack roms
  python3 ./unpack.py mooncrst.zip || die

  echo Converting Moon Cresta
  cd mooncresta || die
  python3 ./mooncresta_rom_convert.py || die
  cd ..
fi

