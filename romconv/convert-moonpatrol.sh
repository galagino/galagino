#!/bin/bash

set -e

die() {

  echo "FAIL - FAIL - FAIL - FAIL - FAIL - FAIL - FAIL - FAIL"
  exit 1
}

#------------------------------------
# Moon Patrol
#------------------------------------

if [[ -f ../romszip/mpatrol.zip ]]; then
  #echo Moon Patrol
  #python3 ./logoconv.py ../logos/moonpatrol.png ../source/src/machines/moonpatrol/moonpatrol_logo.h || die

  echo "Converting Moon Patrol"
  cd moonpatrol || die

  python3 ./moonpatrol_rom_convert.py || die

  cd ..
else
  die
fi

