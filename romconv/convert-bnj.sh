#!/bin/bash

set -e

die() {

  echo "FAIL - FAIL - FAIL - FAIL - FAIL - FAIL - FAIL - FAIL"
  exit 1
}

#------------------------------------
# Bump'n'Jump
#------------------------------------

if [[ -f ../romszip/bnj.zip ]]; then
  #echo "Bump'n'Jump Logo"
  #python3 ./logoconv.py ../logos/bnj.png ../source/src/machines/bnj/bnj_logo.h || die

  echo "Converting Bump'n'Jump"
  cd bnj || die

  python3 ./bnj_rom_convert.py || die

  cd ..
else
  die
fi
