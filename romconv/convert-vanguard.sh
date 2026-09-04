#!/bin/bash

set -e

die() {

  echo "FAIL - FAIL - FAIL - FAIL - FAIL - FAIL - FAIL - FAIL"
  exit 1
}

#------------------------------------
# Vnaguard
#------------------------------------

if [[ -f ../romszip/vanguard.zip ]]; then
  #echo Vnaguard Logos
  #python3 ./logoconv.py ../logos/vanguard.png ../source/src/machines/vanguard/vanguard_logo.h || die

  echo Converting Vnaguard
  cd vanguard || die

  python3 ./vanguard_rom_convert.py || die

  cd ..
else
  die
fi
