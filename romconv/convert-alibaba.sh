#!/bin/bash

set -e

die() {

  echo "FAIL - FAIL - FAIL - FAIL - FAIL - FAIL - FAIL - FAIL"
  exit 1
}

#------------------------------------
# Ali Baba and 40 Thieves
#------------------------------------

if [[ -f ../romszip/alibaba.zip ]]; then
  #echo Ali Baba and 40 Thieves
  #python3 ./logoconv.py ../logos/alibaba.png ../source/src/machines/alibaba/alibaba_logo.h || die

  echo "Converting Ali Baba and 40 Thieves"
  cd alibaba || die

  python3 ./alibaba_rom_convert.py || die

  cd ..
else
  die
fi

