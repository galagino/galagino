#!/usr/bin/env python3

import os

OUT_DIR = os.path.normpath(os.path.join("..", "..", "source", "src", "machines", "starforce"))
OUT_C_FILE = os.path.normpath(os.path.join(OUT_DIR ,"starforce_palette.h"))

def calculate_color_starforce(raw_palette_byte):

  i      = (raw_palette_byte >> 6) & 0x03;
  b_base = (raw_palette_byte >> 4) & 0x03;
  g_base = (raw_palette_byte >> 2) & 0x03;
  r_base = (raw_palette_byte >> 0) & 0x03;

  r4 = ((r_base << 2) | i) if r_base != 0 else 0;
  g4 = ((g_base << 2) | i) if g_base != 0 else 0;
  b4 = ((b_base << 2) | i) if b_base != 0 else 0;

  r8 = r4 * 17;
  g8 = g4 * 17;
  b8 = b4 * 17;

  r5 = r8 >> 3;
  g6 = g8 >> 2;
  b5 = b8 >> 3;

  # 5. Impacchettamento nel formato standard Little Endian
  rgb565_le = (r5 << 11) | (g6 << 5) | b5;

  # 6. ESEGUI LO SWAP DEI BYTE
  # Questo trasforma 0xABCD in 0xCDAB
  return ((rgb565_le & 0xFF00) >> 8) | ((rgb565_le & 0x00FF) << 8);

def convert_colors():
  rgb565 = []
  for i in range(0, 256, 1):
    rgb565.append( calculate_color_starforce(i) )
  return rgb565

def write_palette(filename, rgb565):
  with open(filename, 'w') as f:
    f.write("// Star Force palette: RGB565\n")
    f.write("const unsigned short starforce_rgb565_palette[256] = {\n")
    for i in range(0, 256, 1):
      f.write("0x{:04x}".format(rgb565[i] & 0xFFFF))
      if i+1 < len(rgb565): 
        f.write(",")
        if (i+1) % 8 == 0:      
          f.write("\n")
    f.write("};\n")
    print("Written: {}".format(filename))

# ---- Main ----
def main():
    os.makedirs(OUT_DIR, exist_ok=True)

    rgb565 = convert_colors()
    write_palette(OUT_C_FILE, rgb565)

if __name__ == "__main__":
  main()
