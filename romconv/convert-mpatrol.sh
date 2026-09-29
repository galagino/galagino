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
  #python3 ./logoconv.py ../logos/mpatrol.png ../source/src/machines/mpatrol/mpatrol_logo.h || die

  echo "Converting Moon Patrol"
  cd mpatrol || die

  python3 ./mpatrol_rom_convert.py || die

  cd ..
else
  die
fi

