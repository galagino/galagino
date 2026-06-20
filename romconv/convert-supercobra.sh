#!/bin/bash

set -e

die() {

  echo "FAIL - FAIL - FAIL - FAIL - FAIL - FAIL - FAIL - FAIL"
  exit 1
}

#------------------------------------
# Super Cobra
#------------------------------------

if [[ -f ../romszip/scobra.zip ]]; then
  #echo Super Cobra Logos
  #python3 ./logoconv.py ../logos/supercobra.png ../source/src/machines/supercobra/supercobra_logo.h || die

  echo Converting Super Cobra
  cd supercobra || die

  python3 ./supercobra_rom_convert.py || die

  cd ..
else
  die
fi
