#!/bin/bash

set -e

die() {

  echo "FAIL - FAIL - FAIL - FAIL - FAIL - FAIL - FAIL - FAIL"
  exit 1
}

#------------------------------------
# Phoenix
#------------------------------------

if [[ -f ../romszip/phoenix.zip ]]; then
  #echo Phoenix Logos
  #python3 ./logoconv.py ../logos/phoenix.png ../source/src/machines/phoenix/phoenix_logo.h || die

  echo Converting Phoenix
  cd phoenix || die

  python3 ./phoenix_rom_convert.py || die

  cd ..
else
  die
fi
