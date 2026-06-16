#!/usr/bin/python3

import os
import re
import zipfile

ZIP_FILE = os.path.normpath(os.path.join("..", "romszip", "M6502-081707.zip"))
DEST_DIR = os.path.normpath(os.path.join("..", "source", "src", "cpus", "m6502"))

COPY     = ["Codes.h",     "M6502.c", "M6502.h", "Tables.h"]
RENAME   = ["Codes6502.h", "M6502.c", "M6502.h", "Tables6502.h"]

REPLACE  = [
  [b'#include "Tables.h"', b'#include "Tables6502.h"'],
  [b'#include "Codes.h"', b'#include "Codes6502.h"'],

  [b"/* #define LSB_FIRST */", b"#define LSB_FIRST      "],
# [b"/* #define FAST_RDOP */", b"#define FAST_RDOP      "],

  [b"typedef unsigned short word;",
   b"#undef word\n"+
   b"#define word unsigned short\n"+
   b"//typedef unsigned short word;"],

  [b"FAST_RDOP", b"FAST_6502_RDOP"],
  [b" pair",     b" pair_6502"],
  [b"INT_NONE",  b"INT_6502_NONE"],
  [b"INT_IRQ",   b"INT_6502_IRQ"],
  [b"INT_NMI",   b"INT_6502_NMI"],
  [b"INT_QUIT",  b"INT_6502_QUIT"],
  [b"C_FLAG",    b"C_6502_FLAG"],
  [b"Z_FLAG",    b"Z_6502_FLAG"],
  [b"I_FLAG",    b"I_6502_FLAG"],
  [b"D_FLAG",    b"D_6502_FLAG"],
  [b"B_FLAG",    b"B_6502_FLAG"],
  [b"R_FLAG",    b"R_6502_FLAG"],
  [b"V_FLAG",    b"V_6502_FLAG"],
  [b"N_FLAG",    b"N_6502_FLAG"],

  [b"#endif /* M6502_H */",
   b"#endif /* M6502_H */\n"
   b"\n" + 
   b'#include "../../emulation/emulation.h"']
] 

def unpack_6502(name):
  with zipfile.ZipFile(name, 'r') as zip:
    for v in range(len(COPY)):
      file=COPY[v]
      dest=RENAME[v]
      print("Copying", file, "\t->\t", dest )
      code = zip.read("M6502/" + file)
      with open(os.path.normpath(os.path.join(DEST_DIR, dest)), "wb") as f:
        for line in code.split(b"\r\n"):
          for i in REPLACE:
            line = line.replace(i[0], i[1])
          f.write(line + b"\n")

def main():
  os.makedirs(DEST_DIR, exist_ok=True)

  print(f"Load ZIP from: {os.path.abspath(ZIP_FILE)}")
  print(f"Target files:  {os.path.abspath(DEST_DIR)}")

  unpack_6502(ZIP_FILE)

if __name__ == "__main__":
    main()
