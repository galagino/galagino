#!/bin/bash

set -e

die() {

  echo "FAIL - FAIL - FAIL - FAIL - FAIL - FAIL - FAIL - FAIL"
  exit 1
}

#------------------------------------
# Mappy
#------------------------------------

if [[ -f ../romszip/mappy.zip ]]; then
  #echo Mappy
  #python3 ./logoconv.py ../logos/mappy.png ../source/src/machines/mappy/mappy_logo.h || die

  echo "Converting Mappy"
  cd mappy || die

  python3 ./mappy_rom_convert.py || die

  cd ..
else
  die
fi

