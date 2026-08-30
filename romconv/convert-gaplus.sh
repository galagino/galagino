#!/bin/bash

set -e

die() {

  echo "FAIL - FAIL - FAIL - FAIL - FAIL - FAIL - FAIL - FAIL"
  exit 1
}

#------------------------------------
# Gaplus
#------------------------------------

if [[ -f ../romszip/gaplus.zip ]] || [[ -f ../romszip/galaga3.zip ]]; then
  #echo Gaplus
  #python3 ./logoconv.py ../logos/gaplus.png ../source/src/machines/gaplus/gaplus_logo.h || die

  echo "Converting Gaplus"
  cd gaplus || die

  python3 ./gaplus_rom_convert.py || die

  cd ..
else
  die
fi

