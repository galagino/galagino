#!/usr/bin/env python3
# ============================================================
# Scrambled Egg ROM converter
# Converts "scregg.zip" (Data East set 1)
# ============================================================

import os
import sys
import hashlib

sys.dont_write_bytecode = True

from helper_functions import load_file

ROM_SET = os.path.normpath(os.path.join("..", "..", "romszip", "scregg.zip"))
OUT_DIR = os.path.normpath(os.path.join("..", "..", "source", "src", "machines", "scregg"))

SCREGG_FILES = {
  "romset": {"name": "scregg.zip", "description": "Scrambled Egg"},

  "maincpu01": {"names": ["d00.e14"], "sha1": "e1a329a4452eeb90801d001140ce865bf1ea7716"},
  "maincpu02": {"names": ["d10.d14"], "sha1": "73b3ca6e0d72cd0db951ae9ed1552cf8b7d91e68"},
  "maincpu03": {"names": ["d20.c14"], "sha1": "fc7b2d9094fa5e25c1bf4b68386f640f4502e0c0"},
  "maincpu04": {"names": ["d30.b14"], "sha1": "f2d2fe2236de1b3b2614cc95f61a90571638cd69"},
  "maincpu05": {"names": ["d40.a14"], "sha1": "192cdc506fb0bbfed8ae687f2699397ace3bef30"},

  "gfx1_1":    {"names": ["d50.j12"], "sha1": "88edd35479ceb58244f644a7e0520d225df3bf65"},
  "gfx1_2":    {"names": ["d60.j10"], "sha1": "3067bbd9493614e80d8d3982fe80ef25688d256c"},
  "gfx1_3":    {"names": ["d70.h12"], "sha1": "6a8d257a3fec901453c7216ad894badf96188ebf"},
  "gfx1_4":    {"names": ["d80.h10"], "sha1": "2e38c27b546eeef0fe42340777c8687f4c65ee97"},
  "gfx1_5":    {"names": ["d90.g12"], "sha1": "0da866db6a79f658de3efc609b9ca8520b4d22d0"},
  "gfx1_6":    {"names": ["da0.g10"], "sha1": "e01b72501a01ffc0370cf19c9a379a54800cccc6"},

  "prom1":     {"names": ["dc0.c6"], "sha1": "d09738915da456449bb4e8d9eefb8e6378f0edea"}, # pallete
  "prom2":     {"names": ["db1.b4"], "sha1": "2a283fc17fac32e63385948bfe180d05f1fb8727"}, # unused
}

# ------------------------------------------------------------
# decoder gfx generico stile MAME (planes/xoffs/yoffs come OFFSET BIT
# assoluti), stesso decoder di xevious_rom_convert.py/gaplus_rom_convert.py
# ------------------------------------------------------------
def mame_decode(data, width, height, planes, xoffs, yoffs, bits_per_tile, count):
    tiles = []
    for t in range(count):
        base = t * bits_per_tile
        tile = []
        for y in range(height):
            row = []
            for x in range(width):
                v = 0
                for p in planes:
                    off = base + yoffs[y] + xoffs[x] + p
                    bit = (data[off >> 3] >> (7 - (off & 7))) & 1
                    v = (v << 1) | bit
                row.append(v)
            tile.append(row)
        tiles.append(tile)
    return tiles

# rotazione galagino (portrait): out[y][x] = mame[N-1-x][y]
def rot_galagino(tile):
    n = len(tile)
    return [[tile[n - 1 - x][y] for x in range(n)] for y in range(n)]

# ------------------------------------------------------------
# Layout MAME letti PER INTERO da btime.cpp (righe 2087-2130):
#
# gfx_8x8x3_planar (char set #1, gfx1, MACRO STANDARD MAME): 8x8 3bpp,
# RGN_FRAC(1,3), planeoffset {RGN_FRAC(2,3),RGN_FRAC(1,3),RGN_FRAC(0,3)}
# IDENTICO a tile16layout (stessa regione gfx1, stesso schema di piani —
# char e sprite leggono LO STESSO ROM fisico con granularita' diversa,
# 8x8 vs 16x16). xoffset/yoffset per la variante 8x8 NON split sono lo
# standard MAME ascendente (STEP8(0,1)/STEP8(0,8)), a differenza della
# variante 16x16 che usa lo split "meta' rovesciata" (STEP8(16*8,1) poi
# STEP8(0,1)) visibile in tile16layout stesso.
CHAR_XOFFS = [0,1,2,3,4,5,6,7]
CHAR_YOFFS = [y*8 for y in range(8)]
CHAR_BITS_PER_TILE = 8*8

# tile16layout (sprite gfx1 E sfondo gfx2): 16x16 3bpp, RGN_FRAC(1,3),
# stesso planeoffset del char, xoffset split (meta' destra prima)
TILE16_XOFFS = [16*8+x for x in range(8)] + [x for x in range(8)]
TILE16_YOFFS = [y*8 for y in range(16)]
TILE16_BITS_PER_TILE = 32*8

def planes3(region_bits):
  return [2*(region_bits//3), 1*(region_bits//3), 0*(region_bits//3)]

# ------------------------------------------------------------
def write_rom(name, sym, data, comment):
  with open(os.path.join(OUT_DIR, name), "w") as f:
    print(f"// {comment}", file=f)
    print(f"const unsigned char {sym}[] = {{", file=f)
    for i in range(0, len(data), 16):
      print("  " + ",".join(f"0x{b:02X}" for b in data[i:i+16]) + ",", file=f)
    print("};", file=f)

def write_char_tiles(tiles):
    with open(os.path.join(OUT_DIR, "scregg_chartiles.h"), "w") as f:
        print("// Burger Time char set #1 (aa12.7k+ab13.9k+ab10.10k+ab11.12k+aa8.13k+ab9.15k)", file=f)
        print("// 1024 tile 8x8 3bpp (valori pixel 0-7). pen0 = trasparente quando il", file=f)
        print("// tilemap speciale e' attivo (m_bnj_scroll[0]&0x10), opaco altrimenti.", file=f)
        print("// Colore SEMPRE fisso a palette RAM[0..7] (color group 0, vedi btime.cpp).", file=f)
        print("const unsigned char scregg_chartiles[][8][8] = {", file=f)
        rows = []
        for t in tiles:
            trows = []
            for y in range(8):
                trows.append("{" + ",".join(str(v) for v in t[y]) + "}")
            rows.append(" {" + ",".join(trows) + "}")
        print(",\n".join(rows), file=f)
        print("};", file=f)

def write_sprite_tiles(tiles):
    with open(os.path.join(OUT_DIR, "scregg_spritetiles.h"), "w") as f:
        print("// Burger Time sprites (STESSA ROM del char set #1, gfx1, granularita'", file=f)
        print("// 16x16 invece di 8x8 -- tile16layout). 256 sprite 16x16 3bpp.", file=f)
        print("// pen0 = trasparente (transpen ultimo parametro 0 in btime.cpp).", file=f)
        print("// Colore SEMPRE fisso a palette RAM[0..7] (color group 0).", file=f)
        print("const unsigned char scregg_spritetiles[][16][16] = {", file=f)
        rows = []
        for t in tiles:
            trows = []
            for y in range(16):
                trows.append("{" + ",".join(str(v) for v in t[y]) + "}")
            rows.append(" {" + ",".join(trows) + "}")
        print(",\n".join(rows), file=f)
        print("};", file=f)

def preview(char_tiles, sprite_tiles, bg_tiles, outpng):
    try:
        from PIL import Image
    except ImportError:
        print("PIL import failed.")
        return
    # palette di comodo SOLO per la preview (in gioco e' RAM dinamica):
    # scala di grigi 8 livelli, cosi' si vede la forma dei tile.
    def gray(v):
        g = v * 255 // 7
        return (g, g, g)

    W = 32*9
    char_rows = (len(char_tiles) + 31) // 32
    spr_rows = (len(sprite_tiles) + 15) // 16
    bg_rows = (len(bg_tiles) + 15) // 16
    H = char_rows*9 + spr_rows*18 + bg_rows*18 + 24
    img = Image.new("RGB", (W, H), (32, 32, 96))
    px = img.load()

    for t, tile in enumerate(char_tiles):
        gx, gy = (t % 32) * 9, (t // 32) * 9
        for y in range(8):
            for x in range(8):
                px[gx + x, gy + y] = gray(tile[y][x])

    base = char_rows*9 + 8
    for s, tile in enumerate(sprite_tiles):
        gx, gy = (s % 16) * 18, base + (s // 16) * 18
        for y in range(16):
            for x in range(16):
                px[gx + x, gy + y] = gray(tile[y][x])

    base2 = base + spr_rows*18 + 8
    for b, tile in enumerate(bg_tiles):
        gx, gy = (b % 16) * 18, base2 + (b // 16) * 18
        for y in range(16):
            for x in range(16):
                px[gx + x, gy + y] = gray(tile[y][x])

    img = img.resize((img.width*3, img.height*3), Image.NEAREST)
    img.save(outpng)
    print("preview:", outpng)

# ------------------------------------------------------------

def write_c_array(path, name, data, comment):
    with open(path, 'w', newline='\n') as f:
        f.write(f"// Auto-generated by romconv/scregg/scregg_rom_convert.py - DO NOT EDIT\n")
        if comment:
            f.write(f"// {comment}\n")
        f.write(f"const unsigned char {name}[] = {"{"}\n")
        for i in range(0, len(data), 16):
            chunk = data[i:i+16]
            line = ','.join(f'0x{b:02X}' for b in chunk)
            f.write('  ' + line + ',\n')
        f.write('};\n')

# ------------------------------------------------------------

def convert_scregg(romset, files):

  os.makedirs(OUT_DIR, exist_ok=True)

  print(f"Load ROM from: {os.path.abspath(romset)}")
  print(f"Target files:  {os.path.abspath(OUT_DIR)}")

  cpu01 = load_file(romset, files["maincpu01"]["names"], files["maincpu01"]["sha1"])
  cpu02 = load_file(romset, files["maincpu02"]["names"], files["maincpu02"]["sha1"])
  cpu03 = load_file(romset, files["maincpu03"]["names"], files["maincpu03"]["sha1"])
  cpu04 = load_file(romset, files["maincpu04"]["names"], files["maincpu04"]["sha1"])
  cpu05 = load_file(romset, files["maincpu05"]["names"], files["maincpu05"]["sha1"])

  gfx1_1 = load_file(romset, files["gfx1_1"]["names"], files["gfx1_1"]["sha1"])
  gfx1_2 = load_file(romset, files["gfx1_2"]["names"], files["gfx1_2"]["sha1"])
  gfx1_3 = load_file(romset, files["gfx1_3"]["names"], files["gfx1_3"]["sha1"])
  gfx1_4 = load_file(romset, files["gfx1_4"]["names"], files["gfx1_4"]["sha1"])
  gfx1_5 = load_file(romset, files["gfx1_5"]["names"], files["gfx1_5"]["sha1"])
  gfx1_6 = load_file(romset, files["gfx1_6"]["names"], files["gfx1_6"]["sha1"])

  prom1  = load_file(romset, files["prom1"]["names"], files["prom1"]["sha1"])

  files_ok = all(v is not None for v in [cpu01, cpu02, cpu03, cpu04, cpu05,
                                         gfx1_1, gfx1_2, gfx1_3, gfx1_4, gfx1_5, gfx1_6,
                                         prom1])
  if not files_ok:
    print("ERROR: Not all files have been loaded")
    sys.exit(1)
    return


  gfx1 = gfx1_1 + gfx1_2 + gfx1_3 + gfx1_4 + gfx1_5 + gfx1_6
  assert len(gfx1) == 0x6000
  gfx1_bits = len(gfx1) * 8
  planes_gfx1 = planes3(gfx1_bits)

  char_count = gfx1_bits // 3 // CHAR_BITS_PER_TILE
  char_tiles = [rot_galagino(t) for t in
                mame_decode(gfx1, 8, 8, planes_gfx1, CHAR_XOFFS, CHAR_YOFFS,
                            CHAR_BITS_PER_TILE, char_count)]
  write_char_tiles(char_tiles)
  print(f"char tiles: {char_count}")

  sprite_count = gfx1_bits // 3 // TILE16_BITS_PER_TILE
  sprite_tiles = [rot_galagino(t) for t in
                  mame_decode(gfx1, 16, 16, planes_gfx1, TILE16_XOFFS, TILE16_YOFFS,
                              TILE16_BITS_PER_TILE, sprite_count)]
  write_sprite_tiles(sprite_tiles)
  print(f"sprite tiles: {sprite_count}")

  # --- ROM CPU (rimangono CIFRATE in flash, decrittate a runtime dal core
  # m6502 patchato con l'hook fetch — vedi decocpu7.cpp) ---
  # maincpu: solo 0xc000-0xffff popolato in questo set (16KB, 4 file);
  # 0xb000-0xbfff resta a 0 (non presente nel set "btime" Data East set 1).
  maincpu = cpu01 + cpu02 + cpu03 + cpu04 + cpu05
  assert len(maincpu) == 0x5000  # 0x3000-0x7fff
  write_rom("scregg_rom.h", "scregg_rom", bytes(maincpu),
            "Burger Time main CPU ROM 0xb000-0xffff (0xb000-0xbfff empty in this"
           )

  write_c_array(os.path.join(OUT_DIR, 'scregg_colorprom.h'), 
                'scregg_colorprom', prom1, 'Scregg color PROMs')

  #preview(char_tiles, sprite_tiles, bg_tiles, "scregg_preview.png")
  print("Scrambled Egg conversion finished.")

# -------------------------------------------------------------------

def main():
  if os.path.isfile(ROM_SET):
    convert_scregg(ROM_SET, SCREGG_FILES)
  else:
    print("ERROR: No roms.")
    sys.exit(1)

# -------------------------------------------------------------------

if __name__ == "__main__":
    main()

