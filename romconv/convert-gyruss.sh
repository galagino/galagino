#!/bin/bash

set -e

die() {

  echo "FAIL - FAIL - FAIL - FAIL - FAIL - FAIL - FAIL - FAIL"
  exit 1
}

#------------------------------------
# Gyruss
#------------------------------------

if [[ -f ../romszip/gyruss.zip ]]; then
  echo Gyruss Unpack roms
  python3 ./unpack.py gyruss.zip || die

  #echo Gyruss Logos
  #python3 ./logoconv.py ../logos/gyruss.png ../source/src/machines/gyruss/gyruss_logo.h || die

  echo Converting Gyruss
  cd gyruss
  python3 ./gyruss_rom_convert.py || die
  cd ..
fi

