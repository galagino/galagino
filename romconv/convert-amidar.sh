#!/bin/bash

set -e

die() {

  echo "FAIL - FAIL - FAIL - FAIL - FAIL - FAIL - FAIL - FAIL"
  exit 1
}

#------------------------------------
# Amidar
#------------------------------------

if [[ -f ../romszip/amidar.zip ]]; then
  #echo Amidar Logos
  #python3 ./logoconv.py ../logos/amidar.png ../source/src/machines/amidar/amidar_logo.h || die

  echo Converting Amidar
  cd amidar || die

  python3 ./amidar_rom_convert.py || die

  cd ..
else
  die
fi
