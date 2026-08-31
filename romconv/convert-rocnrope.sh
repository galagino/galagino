#!/bin/bash

set -e

die() {

  echo "FAIL - FAIL - FAIL - FAIL - FAIL - FAIL - FAIL - FAIL"
  exit 1
}

#------------------------------------
# Roc'n Rope
#------------------------------------

if [[ -f ../romszip/rocnrope.zip ]]; then
  #echo "Roc'n Rope Logos"
  #python3 ./logoconv.py ../logos/rocnrope.png ../source/src/machines/rocnrope/rocnrope_logo.h || die

  echo "Converting Roc'n Rope"
  cd rocnrope || die

  python3 ./rocnrope_rom_convert.py || die

  cd ..
else
  die
fi
