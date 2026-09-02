#!/bin/bash

set -e

die() {

  echo "FAIL - FAIL - FAIL - FAIL - FAIL - FAIL - FAIL - FAIL"
  exit 1
}

#------------------------------------
# Pinball Action
#------------------------------------

if [[ -f ../romszip/pbaction.zip ]]; then
  #echo Pinball Action Logos
  #python3 ./logoconv.py ../logos/pbaction.png ../source/src/machines/pbaction/pbaction_logo.h || die

  echo Converting Pinball Action
  cd pbaction || die

  python3 ./pbaction_rom_convert.py || die

  cd ..
else
  die
fi
