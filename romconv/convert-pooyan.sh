#!/bin/bash

set -e

die() {

  echo "FAIL - FAIL - FAIL - FAIL - FAIL - FAIL - FAIL - FAIL"
  exit 1
}

#------------------------------------
# Pooyan
#------------------------------------
if [[ -f ../romszip/pooyan.zip ]]; then
  echo Pooyan Unpack roms
  python3 ./unpack.py pooyan.zip || die

  #echo Pooyan Logos
  #python3 ./logoconv.py ../logos/pooyan.png ../source/src/machines/pooyan/pooyan_logo.h || die

  cd pooyan || die

  echo Converting Pooyan
  python3 ./pooyan_rom_convert.py

  cd ..
else
  die
fi
