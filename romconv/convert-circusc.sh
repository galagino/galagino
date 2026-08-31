#!/bin/bash

set -e

die() {

  echo "FAIL - FAIL - FAIL - FAIL - FAIL - FAIL - FAIL - FAIL"
  exit 1
}

#------------------------------------
# Circus Charlie
#------------------------------------

if [[ -f ../romszip/circusc.zip ]]; then
  #echo Circus Charlie Logos
  #python3 ./logoconv.py ../logos/circusc.png ../source/src/machines/circusc/circusc_logo.h || die

  echo Converting Circus Charlie
  cd circusc || die

  python3 ./circusc_rom_convert.py || die

  cd ..
else
  die
fi
