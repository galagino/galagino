#!/bin/bash

set -e

die() {

  echo "FAIL - FAIL - FAIL - FAIL - FAIL - FAIL - FAIL - FAIL"
  exit 1
}

#------------------------------------
# Motorace USA
#------------------------------------

if [[ -f ../romszip/motorace.zip ]]; then
  #echo Motorace USA Logos
  #python3 ./logoconv.py ../logos/motorace.png ../source/src/machines/motorace/motorace_logo.h || die

  echo Converting Motorace USA
  cd motorace || die

  python3 ./motorace_rom_convert.py || die

  cd ..
else
  die
fi
