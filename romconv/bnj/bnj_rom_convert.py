#!/usr/bin/env python3
# ============================================================
# Bump 'n' Jump (Data East 1982, set "bnj"
# Bump 'n' Jump (Data East 1982, set "bnjm" / Bally Midway license) ROM
#
# ============================================================


import os
import sys
import hashlib

sys.dont_write_bytecode = True

from helper_functions import load_file

ROM_SET = os.path.normpath(os.path.join("..", "..", "romszip", "bnj.zip"))
OUT_DIR = os.path.normpath(os.path.join("..", "..", "source", "src", "machines", "bnj"))

BNJ_FILES = {
  "romset": {"name": "bnj.zip", "description": "Bump 'n' Jump"},

  "maincpu1": {"names": ["ad08.12b"],    "sha1": "83105718c2d18ef75ca18ae92b34545cb939bc02"},
  "maincpu2": {"names": ["ad07.12c"],    "sha1": "f62d752bb7a995e120ed4d642793c543f0ef13ca"},
  "maincpu3": {"names": ["ad06.12d"],    "sha1": "f231ed008537aeeeacbec64f485e9a96ab3441e1"},

  "audiocpu": {"names": ["ad05.6c"],     "sha1": "1279d564e65fd3ccac25b1f9fbb40d910de2b544"},

  "gfx1_1":   {"names": ["ad00.4e"],     "sha1": "cacf71fa6c0f7121d077381a0ff6222f534295ab"},
  "gfx1_2":   {"names": ["ad01.4f"],     "sha1": "5e52554f594f569527af4768d244cc40a7b4460a"},
  "gfx1_3":   {"names": ["ad02.4h"],     "sha1": "e98f0eb476b8f033f5cc70a6e503afc4e651fd45"},

  "gfx2_1":   {"names": ["ad03.10e"],     "sha1": "b356512d2ebd4e2005e76496b434e5ecebadb251"},
  "gfx2_2":   {"names": ["ad04.10f"],     "sha1": "49d5f9c0b695f474197fbb761bacc065b6b5808a"},

  "plds_1":   {"names": ["pb-5.10k.bin"], "sha1": "d61c149d4df93a2074debf7c5e46557c6b06d10d"}
}

BNJM_FILES = {
  "romset": {"name": "bnjm.zip", "description": "Bump 'n' Jump (Midway)"},

  "maincpu1":  {"names": ["bnj12b.bin"],     "sha1": "56284076d938c33c1492a07281b936681eb09808"}, # maincpu @0xa000
  "maincpu2":  {"names": ["bnj12c.bin"],     "sha1": "4a964389cc8035b9264d4cb133eb6d3826e74b95"}, # maincpu @0xc000
  "maincpu3":  {"names": ["bnj12d.bin"],     "sha1": "08a4ddea4037f9e14d0d9f4262a1746b0a3a140c"}, # maincpu @0xe000

  "audiocpu1": {"names": ["bnj6c.bin"],      "sha1": "1279d564e65fd3ccac25b1f9fbb40d910de2b544"}, # audiocpu @0xe000

  "gfx1_01":   {"names": ["bnj4e.bin"],      "sha1": "cacf71fa6c0f7121d077381a0ff6222f534295ab"}, # gfx1 third0 (low)
  "gfx1_02":   {"names": ["bnj4f.bin"],      "sha1": "5e52554f594f569527af4768d244cc40a7b4460a"}, # gfx1 third1 (mid)
  "gfx1_03":   {"names": ["bnj4h.bin"],      "sha1": "e98f0eb476b8f033f5cc70a6e503afc4e651fd45"}, # gfx1 third2 (high)

  "gfx2_01":   {"names": ["bnj10e.bin"],     "sha1": "b356512d2ebd4e2005e76496b434e5ecebadb251"}, # gfx2 half0
  "gfx2_02":   {"names": ["bnj10f.bin"],     "sha1": "49d5f9c0b695f474197fbb761bacc065b6b5808a"}  # gfx2 half1
}

# ------------------------------------------------------------
# generic gfx decoder MAME (planes/xoffs/yoffs as absolute OFFSET BIT)
# same as burgertimer_rom_convert.py
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

def rot_galagino(tile):
    n = len(tile)
    return [[tile[n - 1 - x][y] for x in range(n)] for y in range(n)]

# ------------------------------------------------------------
# Layout MAME (btime.cpp righe 2087-2130, IDENTICI a btime per char/sprite:
# gfx1 di bnj e' la STESSA disposizione fisica di btime, solo contenuto
# artistico diverso -- vedi gfx_bnj righe 2149-2152: usa gfx_8x8x3_planar
# per i char e tile16layout per gli sprite, esattamente come gfx_btime).
CHAR_XOFFS = [0,1,2,3,4,5,6,7]
CHAR_YOFFS = [y*8 for y in range(8)]
CHAR_BITS_PER_TILE = 8*8

TILE16_XOFFS = [16*8+x for x in range(8)] + [x for x in range(8)]
TILE16_YOFFS = [y*8 for y in range(16)]
TILE16_BITS_PER_TILE = 32*8

def planes3(region_bits):
    return [2*(region_bits//3), 1*(region_bits//3), 0*(region_bits//3)]

# bnj_tile16layout (btime.cpp righe 2121-2130), NUOVO in questo progetto,
# usato SOLO per lo sfondo scrollabile di bnj (regione gfx2, 0x2000 byte =
# bnj10e.bin+bnj10f.bin, ciascuno 0x1000):
#
#   bnj_tile16layout = { 16,16, RGN_FRAC(1,2), 3,
7#     { RGN_FRAC(1,2)+4, RGN_FRAC(0,2)+0, RGN_FRAC(0,2)+4 },
#     { STEP4(3*16*8,1), STEP4(2*16*8,1), STEP4(1*16*8,1), STEP4(0*16*8,1) },
#     { STEP16(0,8) }, 64*8 }
#
# planeoffset: plane0 = meta'-alta regione (bnj10f) NIBBLE ALTO (+4);
# plane1 = meta'-bassa regione (bnj10e) NIBBLE BASSO (+0); plane2 =
# meta'-bassa regione (bnj10e) NIBBLE ALTO (+4). Il nibble basso di
# bnj10f (meta'-alta) NON e' usato da questo layout (3bpp, non 4bpp).
# xoffset: 4 gruppi da 4 colonne, in ordine INVERTITO (col 12-15, poi 8-11,
# poi 4-7, poi 0-3) -- ogni gruppo di 4 pixel adiacenti condivide lo stesso
# nibble ma bit diversi (STEP4 con incremento 1 = 4 bit consecutivi nello
# stesso nibble). charincrement 64*8=512 bit = 64 byte/tile.
def bnj_bg_layout_decode(gfx2):
    total_bits = len(gfx2) * 8
    half_bits = total_bits // 2
    planes = [half_bits + 4, 0 + 0, 0 + 4]
    xoffs = ([3*16*8 + i for i in range(4)] +
             [2*16*8 + i for i in range(4)] +
             [1*16*8 + i for i in range(4)] +
             [0*16*8 + i for i in range(4)])
    yoffs = [y*8 for y in range(16)]
    bits_per_tile = 64*8
    count = half_bits // bits_per_tile
    return mame_decode(gfx2, 16, 16, planes, xoffs, yoffs, bits_per_tile, count), count

# ------------------------------------------------------------
def write_rom(name, sym, data, comment):
  with open(os.path.join(OUT_DIR, name), "w") as f:
    print(f"// {comment}", file=f)
    print(f"const unsigned char {sym}[] = {{", file=f)
    for i in range(0, len(data), 16):
      print("  " + ",".join(f"0x{b:02x}" for b in data[i:i+16]) + ",", file=f)
    print("};", file=f)

def write_char_tiles(tiles):
    with open(os.path.join(OUT_DIR, "bnj_chartiles.h"), "w") as f:
        print("// Bump 'n' Jump char set #1 (gfx1).", file=f)
        print("// 1024 tile 8x8 3bpp (pixel values 0-7). Colors from ", file=f)
        print("// palette RAM[0..7] (color group 0).", file=f)
        print("const unsigned char bnj_chartiles[][8][8] = {", file=f)
        rows = []
        for t in tiles:
            trows = ["{" + ",".join(str(v) for v in t[y]) + "}" for y in range(8)]
            rows.append(" {" + ",".join(trows) + "}")
        print(",\n".join(rows), file=f)
        print("};", file=f)

def write_sprite_tiles(tiles):
    with open(os.path.join(OUT_DIR, "bnj_spritetiles.h"), "w") as f:
        print("// Bump 'n' Jump sprites", file=f)
        print("// (gfx1 16x16 -- tile16layout). 256 sprite 16x16 3bpp.", file=f)
        print("const unsigned char bnj_spritetiles[][16][16] = {", file=f)
        rows = []
        for t in tiles:
            trows = ["{" + ",".join(str(v) for v in t[y]) + "}" for y in range(16)]
            rows.append(" {" + ",".join(trows) + "}")
        print(",\n".join(rows), file=f)
        print("};", file=f)

def write_bg_tiles(tiles):
    with open(os.path.join(OUT_DIR, "bnj_bgtiles.h"), "w") as f:
        print("// Bump 'n'Jump background", file=f)
        print("// gfx2 bnj_tile16layout -- nibble packing", file=f)
        print("// converter). 16x16 3bpp, layer OPAQUE", file=f)
        print("// colors from palette RAM[8..15]", file=f)
        print("const unsigned char bnj_bgtiles[][16][16] = {", file=f)
        rows = []
        for t in tiles:
            trows = ["{" + ",".join(str(v) for v in t[y]) + "}" for y in range(16)]
            rows.append(" {" + ",".join(trows) + "}")
        print(",\n".join(rows), file=f)
        print("};", file=f)

def preview(char_tiles, sprite_tiles, bg_tiles, outpng):
    try:
        from PIL import Image
    except ImportError:
        print("PIL import error. no preview")
        return

    # preview pallete - in game pallete comes from RAM
    PALETTE = [(20,20,20),(200,60,60),(60,200,60),(60,60,200),
               (200,200,60),(200,60,200),(60,200,200),(230,230,230)]
    def col(v):
        return PALETTE[v & 7]

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
                px[gx + x, gy + y] = col(tile[y][x])

    base = char_rows*9 + 8
    for s, tile in enumerate(sprite_tiles):
        gx, gy = (s % 16) * 18, base + (s // 16) * 18
        for y in range(16):
            for x in range(16):
                px[gx + x, gy + y] = col(tile[y][x])

    base2 = base + spr_rows*18 + 8
    for b, tile in enumerate(bg_tiles):
        gx, gy = (b % 16) * 18, base2 + (b // 16) * 18
        for y in range(16):
            for x in range(16):
                px[gx + x, gy + y] = col(tile[y][x])

    img = img.resize((img.width*3, img.height*3), Image.NEAREST)
    img.save(outpng)
    print("preview:", outpng)

# ------------------------------------------------------------
def convert_bnj(romset, files, galagino):
  os.makedirs(OUT_DIR, exist_ok=True)

  print(f"Load ROM from: {os.path.abspath(romset)}")
  print(f"Target files:  {os.path.abspath(OUT_DIR)}")

  cpu01 = load_file(romset, files["maincpu1"]["names"], files["maincpu1"]["sha1"])
  cpu02 = load_file(romset, files["maincpu2"]["names"], files["maincpu2"]["sha1"])
  cpu03 = load_file(romset, files["maincpu3"]["names"], files["maincpu3"]["sha1"])

  audio = load_file(romset, files["audiocpu"]["names"], files["audiocpu"]["sha1"])

  gfx1_1 = load_file(romset, files["gfx1_1"]["names"], files["gfx1_1"]["sha1"])
  gfx1_2 = load_file(romset, files["gfx1_2"]["names"], files["gfx1_2"]["sha1"])
  gfx1_3 = load_file(romset, files["gfx1_3"]["names"], files["gfx1_3"]["sha1"])

  gfx2_1 = load_file(romset, files["gfx2_1"]["names"], files["gfx2_1"]["sha1"])
  gfx2_2 = load_file(romset, files["gfx2_2"]["names"], files["gfx2_2"]["sha1"])

  gfx1 = gfx1_1 + gfx1_2 + gfx1_3
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

  gfx2 = gfx2_1 + gfx2_2
  assert len(gfx2) == 0x2000
  bg_tiles_raw, bg_count = bnj_bg_layout_decode(gfx2)
  bg_tiles = [rot_galagino(t) for t in bg_tiles_raw]
  write_bg_tiles(bg_tiles)
  print(f"bg tiles: {bg_count}")

  # --- ROM CPU (rimangono CIFRATE in flash: DECO C10707 e' STATICA,
  # decifrata a runtime dal core m6502 patchato via hook fetch, stesso
  # meccanismo di btime/CPU-7 ma bitswap fisso senza stato) ---
  ### XXX - FIX-ME - decrypt here

  maincpu = cpu01 + cpu02 + cpu03  
  assert len(maincpu) == 0x6000  # 0xa000-0xffff
  write_rom("bnj_rom_main.h", "bnj_rom_main", maincpu,
            "Bump 'n' Jump main CPU ROM 0xa000-0xffff (24KB), DECO C10707 "
            "(bitswap bit5<->bit6 for all fetch opcode.")

  write_rom("bnj_rom_audio.h", "bnj_rom_audio", audio,
            "Bump 'n' Jump audio CPU ROM 0xe000-0xefff (mirrored 0x1000-0x1fff)")

  #preview(char_tiles, sprite_tiles, bg_tiles, "bnj_preview.png")
  print("Bump'n'Jump (ROM/gfx) conversion done.")

def main():
  if os.path.isfile(ROM_SET):
    convert_bnj(ROM_SET, BNJ_FILES, {})
  else:
    print("ERROR: No roms.")
    sys.exit(1)

if __name__ == "__main__":
    main()
