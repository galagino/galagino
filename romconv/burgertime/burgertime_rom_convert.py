#!/usr/bin/env python3
# ============================================================
# Burger Time (Data East 1982) ROM converter
# Converts "btime.zip" (Data East set 1)
# ============================================================

import os
import sys
import hashlib

sys.dont_write_bytecode = True

from helper_functions import load_file

ROM_SET = os.path.normpath(os.path.join("..", "..", "romszip", "btime.zip"))
OUT_DIR = os.path.normpath(os.path.join("..", "..", "source", "src", "machines", "burgertime"))

BURGERTIME_FILES = {
  "romset": {"name": "btime.zip", "description": "Burger Time (Data East set 1)"},

  "maincpu01": {"names": ["aa04.9b"],  "sha1": "ed3f3712423979dcb351941fa85dce6a0a7bb16b"},
  "maincpu02": {"names": ["aa06.13b"], "sha1": "8c77397e934907bc47a739f263196a0f2f81ba3d"},
  "maincpu03": {"names": ["aa05.10b"], "sha1": "d0da4e360039f6a8d8142a4e8e05c1f90c0af68a"},
  "maincpu04": {"names": ["aa07.15b"], "sha1": "4a32bc92f8ff5fbe112f56e62d2c03da8851a7b9"},

  "audiocpu1": {"names": ["ab14.12h"], "sha1": "27940026d0c6212d1138d2fd88880df697218627"},

  "gfx1_1":    {"names": ["aa12.7k"],  "sha1": "24204d591aa2c264a852ee9ba8c4be63efd97728"},
  "gfx1_2":    {"names": ["ab13.9k"],  "sha1": "e64b6381a9298eaf74e79fa5f1ea8e9596c58a49"},
  "gfx1_3":    {"names": ["ab10.10k"], "sha1": "3d2ecfd54a5a9d68b53cf4b4ee1f2daa6aef2123"},
  "gfx1_4":    {"names": ["ab11.12k"], "sha1": "0a55b091cd4e7f317c35defe13d5051b26042eee"},
  "gfx1_5":    {"names": ["aa8.13k"],  "sha1": "d9b1ee2d1f2fd66705d497c80252861b49aa9254"},
  "gfx1_6":    {"names": ["ab9.15k"],  "sha1": "b72633de6268ce16742bba4dcba835df860d6c2f"},
  "gfx2_1":    {"names": ["ab00.1b"],  "sha1": "6a0a8e6b7860859f22daa33634e34fbf91387659"},
  "gfx2_2":    {"names": ["ab01.3b"],  "sha1": "4abdcbd4f3362c3e4463a1274731289f1a72d2e6"},
  "gfx2_3":    {"names": ["ab02.4b"],  "sha1": "4a03bf011dc1fb2902f42587b1174b880cf06df1"},
  "bg_map":    {"names": ["ab03.6b"],  "sha1": "737af6e264183a1f151f277a07cf250d6abb3fd8"},
}

GALAGINO_FILES = {
  "file_cpu_rom":    "burgertime_rom.h",
  "array_cpu_rom":   "burgertime_rom",
  "file_colormap":   "burgertime_colormap.h",
  "array_colormap":  "burgertime_colormap",
  "file_tilemap":    "burgertime_tiles.h",
  "array_tilemap":   "burgertime_tiles",
  "file_spritemap":  "burgertime_spritemap.h",
  "array_spritemap": "burgertime_sprites",

  "preview_tiles":   "burgertime_tiles_preview.png",
  "preview_sprites": "burgertime_sprites_preview.png",
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
    with open(os.path.join(OUT_DIR, "burgertime_chartiles.h"), "w") as f:
        print("// Burger Time char set #1 (aa12.7k+ab13.9k+ab10.10k+ab11.12k+aa8.13k+ab9.15k)", file=f)
        print("// 1024 tile 8x8 3bpp (valori pixel 0-7). pen0 = trasparente quando il", file=f)
        print("// tilemap speciale e' attivo (m_bnj_scroll[0]&0x10), opaco altrimenti.", file=f)
        print("// Colore SEMPRE fisso a palette RAM[0..7] (color group 0, vedi btime.cpp).", file=f)
        print("const unsigned char burgertime_chartiles[][8][8] = {", file=f)
        rows = []
        for t in tiles:
            trows = []
            for y in range(8):
                trows.append("{" + ",".join(str(v) for v in t[y]) + "}")
            rows.append(" {" + ",".join(trows) + "}")
        print(",\n".join(rows), file=f)
        print("};", file=f)

def write_sprite_tiles(tiles):
    with open(os.path.join(OUT_DIR, "burgertime_spritetiles.h"), "w") as f:
        print("// Burger Time sprites (STESSA ROM del char set #1, gfx1, granularita'", file=f)
        print("// 16x16 invece di 8x8 -- tile16layout). 256 sprite 16x16 3bpp.", file=f)
        print("// pen0 = trasparente (transpen ultimo parametro 0 in btime.cpp).", file=f)
        print("// Colore SEMPRE fisso a palette RAM[0..7] (color group 0).", file=f)
        print("const unsigned char burgertime_spritetiles[][16][16] = {", file=f)
        rows = []
        for t in tiles:
            trows = []
            for y in range(16):
                trows.append("{" + ",".join(str(v) for v in t[y]) + "}")
            rows.append(" {" + ",".join(trows) + "}")
        print(",\n".join(rows), file=f)
        print("};", file=f)

def write_bg_tiles(tiles):
    with open(os.path.join(OUT_DIR, "burgertime_bgtiles.h"), "w") as f:
        print("// Burger Time sfondo (ab00.1b+ab01.3b+ab02.4b, gfx2, tile16layout).", file=f)
        print("// 64 tile 16x16 3bpp (valori pixel 0-7). Layer OPAQUE (mai trasparente,", file=f)
        print("// vedi gfxdecode->gfx(2)->opaque in draw_background). Colore SEMPRE", file=f)
        print("// fisso a palette RAM[8..15] (color group base 8, vedi GFXDECODE_ENTRY).", file=f)
        print("const unsigned char burgertime_bgtiles[][16][16] = {", file=f)
        rows = []
        for t in tiles:
            trows = []
            for y in range(16):
                trows.append("{" + ",".join(str(v) for v in t[y]) + "}")
            rows.append(" {" + ",".join(trows) + "}")
        print(",\n".join(rows), file=f)
        print("};", file=f)

def write_bg_map(data):
    write_rom("burgertime_bgmap.h", "burgertime_bgmap", data,
               "Burger Time bg_map lookup ROM (ab03.6b), 0x800 byte grezzi: "
               "4 banchi x 0x200 selezionabili da m_btime_tilemap[i&3], ogni "
               "banco indicizzato 0..0xff, valore = indice diretto in burgertime_bgtiles "
               "(vedi draw_background() in btime.cpp)")

def preview(char_tiles, sprite_tiles, bg_tiles, outpng):
    try:
        from PIL import Image
    except ImportError:
        print("PIL non disponibile, niente preview")
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
def convert_burgertime(romset, files, galagino):

  os.makedirs(OUT_DIR, exist_ok=True)

  print(f"Load ROM from: {os.path.abspath(romset)}")
  print(f"Target files:  {os.path.abspath(OUT_DIR)}")

  cpu01 = load_file(romset, files["maincpu01"]["names"], files["maincpu01"]["sha1"])
  cpu02 = load_file(romset, files["maincpu02"]["names"], files["maincpu02"]["sha1"])
  cpu03 = load_file(romset, files["maincpu03"]["names"], files["maincpu03"]["sha1"])
  cpu04 = load_file(romset, files["maincpu04"]["names"], files["maincpu04"]["sha1"])

  audio = load_file(romset, files["audiocpu1"]["names"], files["audiocpu1"]["sha1"])

  gfx1_1 = load_file(romset, files["gfx1_1"]["names"], files["gfx1_1"]["sha1"])
  gfx1_2 = load_file(romset, files["gfx1_2"]["names"], files["gfx1_2"]["sha1"])
  gfx1_3 = load_file(romset, files["gfx1_3"]["names"], files["gfx1_3"]["sha1"])
  gfx1_4 = load_file(romset, files["gfx1_4"]["names"], files["gfx1_4"]["sha1"])
  gfx1_5 = load_file(romset, files["gfx1_5"]["names"], files["gfx1_5"]["sha1"])
  gfx1_6 = load_file(romset, files["gfx1_6"]["names"], files["gfx1_6"]["sha1"])

  gfx2_1 = load_file(romset, files["gfx2_1"]["names"], files["gfx2_1"]["sha1"])
  gfx2_2 = load_file(romset, files["gfx2_2"]["names"], files["gfx2_2"]["sha1"])
  gfx2_3 = load_file(romset, files["gfx2_3"]["names"], files["gfx2_3"]["sha1"])

  bg_map = load_file(romset, files["bg_map"]["names"], files["bg_map"]["sha1"])

  files_ok = all(v is not None for v in [cpu01, cpu02, cpu03, cpu04, audio,
                                         gfx1_1, gfx1_2, gfx1_3, gfx1_4, gfx1_5, gfx1_6,
                                         gfx2_1, gfx2_2, gfx2_3,
                                         bg_map])
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

  # --- gfx2 (background): 3 files = 0x1800 byte ---
  gfx2 = gfx2_1 + gfx2_2 + gfx2_3
  assert len(gfx2) == 0x1800
  gfx2_bits = len(gfx2) * 8
  planes_gfx2 = planes3(gfx2_bits)
  bg_count = gfx2_bits // 3 // TILE16_BITS_PER_TILE
  bg_tiles = [rot_galagino(t) for t in
              mame_decode(gfx2, 16, 16, planes_gfx2, TILE16_XOFFS, TILE16_YOFFS,
                            TILE16_BITS_PER_TILE, bg_count)]
  write_bg_tiles(bg_tiles)
  print(f"bg tiles: {bg_count}")

  write_bg_map(bg_map)

  # --- ROM CPU (rimangono CIFRATE in flash, decrittate a runtime dal core
  # m6502 patchato con l'hook fetch — vedi decocpu7.cpp) ---
  # maincpu: solo 0xc000-0xffff popolato in questo set (16KB, 4 file);
  # 0xb000-0xbfff resta a 0 (non presente nel set "btime" Data East set 1).
  maincpu = bytearray(0x1000) + cpu01 + cpu02 + cpu03 + cpu04
  assert len(maincpu) == 0x5000  # 0xb000-0xffff
  write_rom("burgertime_rom_main.h", "burgertime_rom_main", bytes(maincpu),
            "Burger Time main CPU ROM 0xb000-0xffff (0xb000-0xbfff empty in this"
            "set), encrypted DECO CPU-7 -- runtime decryption (see burgertime.cpp "
            "fetch_cpu7)")

  write_rom("burgertime_rom_audio.h", "burgertime_rom_audio", audio,
            "Burger Time audio CPU ROM 0xe000-0xefff (mirror 0x1000-0x1fff), not encrypted")

  #preview(char_tiles, sprite_tiles, bg_tiles, "burgertime_preview.png")
  print("Burger Time conversion finished.")

# -------------------------------------------------------------------

def main():
  if os.path.isfile(ROM_SET):
    convert_burgertime(ROM_SET, BURGERTIME_FILES, GALAGINO_FILES)
  else:
    print("ERROR: No roms.")
    sys.exit(1)

# -------------------------------------------------------------------

if __name__ == "__main__":
    main()

