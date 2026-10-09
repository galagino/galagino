#!/bin/bash

set -e

die() {

  echo "FAIL - FAIL - FAIL - FAIL - FAIL - FAIL - FAIL - FAIL"
  exit 1
}

#------------------------------------
# Zaxxon
#------------------------------------

if [[ -f ../romszip/zaxxon.zip ]]; then
  #echo Zaxxon
  #python3 ./logoconv.py ../logos/zaxxon.png ../source/src/machines/zaxxon/zaxxon_logo.h || die

  echo "Converting Zaxxon"
  cd zaxxon || die

  python3 ./zaxxon_rom_convert.py || die

  cd ..
else
  die
fi

