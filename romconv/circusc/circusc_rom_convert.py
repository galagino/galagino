#!/usr/bin/env python3
# ============================================================
# Circus Charlie (Konami 1984) ROM converter
#
# Hardware (MAME konami/circusc.cpp):
# - tiles 8x8 4bpp "packed msb" (gfx_8x8x4_packed_msb): 32 byte/tile,
#   2 pixel/byte, nibble ALTO = pixel di sinistra; 512 tile da 16KB
# - sprite 16x16 4bpp packed msb: 128 byte/sprite, 384 sprite da 48KB
# - palette PROM 32B bbgggrrr (identica a mappy) + 2 lookup 256B:
#   char pen = lut+0x10, sprite pen = lut; trasparenza sprite =
#   transpen_mask(color, 0): pen p trasparente se lut[c*16+p]==lut[c*16+0]
#
# Formati output:
# - tiles PRE-RUOTATI ROT90 galagino (out[y][x] = mame[7-x][y], come
#   mappy/todruaga) e impacchettati [8][4] nibble LSB-first (nibble basso
#   = colonna pari) -> blit stile timeplt con ptr[c]
# - sprite in ORIENTAMENTO LANDSCAPE nativo [16][8] nibble LSB-first,
#   senza varianti flip (48KB una copia sola): la rotazione la fa il blit
#   trasposto (screen_x = spr_x + 15 - r) e i flip si fanno a runtime,
#   come rocnrope
# ============================================================

import os
import sys

sys.dont_write_bytecode = True
from helper_functions import load_file

ROM_SET = os.path.normpath(os.path.join("..", "..", "romszip", "circusc.zip"))
OUT_DIR = os.path.normpath(os.path.join("..", "..", "source", "src", "machines", "circusc"))

REQUIRED = [
    # main CPU (KONAMI-1), 0x6000-0xFFFF
    ("380_s05.3h",   0x2000, "0e5bd350fa5fee42569eb0c4accf7512d645b792"),  # 380_s05.3h @6000 - "s05"
    ("380_q04.4h",   0x2000, "458c398911453d558003f49c298b0d593c941c11"),  # 380_q04.4h @8000 - "q04"
    ("380_q03.5h",   0x2000, "03211f0cc90b6e356989c5e2a41b70f4ff2ead83"),  # 380_q03.5h @a000 - "q03" 
    ("380_q02.6h",   0x2000, "a1f65e73c4e5abff1b0970bad32a128173245561"),  # 380_q02.6h @c000 - "q02"
    ("380_q01.7h",   0x2000, "2f40e1a109d129bb127a8b98e27817988cd08c8b"),  # 380_q01.7h @e000 - "q02"
    # audio CPU (Z80), 0x0000-0x3FFF
    ("380_l14.5c",   0x2000, "67103d61994fd3a1e2de7cf9487e4f763234b18e"),  # 380_l14.5c       - "cd05_l14.bin"
    ("380_l15.7c",   0x2000, "14f305717edcc2471e763b262960a0b96eef3530"),  # 380_l15.7c       - "cd07_l15.bin"
    # tiles
    ("380_j12.4a",   0x2000, "73b9e3d46dfe9e39b390c634df153648a0906876"),  # 380_j12.4a       - "a04_j12.bin"
    ("380_j13.5a",   0x2000, "4d0b0a773c385b7f1dcf024760d0437f47e78fbe"),  # 380_j13.5a       - "a05_k13.bin"
    # sprites
    ("380_j06.11e",  0x2000, "70a50dcc86dfbdaa9c2af613105aae7f90747804"),  # 380_j06.11e      - "e11_j06.bin"      
    ("380_j07.12e",  0x2000, "2ad7cbcbdbb434dc43e9c94cd00df9e57ac097f5"),  # 380_j07.12e      - "e12_j07.bin"
    ("380_j08.13e",  0x2000, "b22ad7cfda392894208eb4b39505f38bfe4c4342"),  # 380_j08.13e      - "e13_j08.bin"
    ("380_j09.14e",  0x2000, "1a649ec667d377ffab26b4694be790b3a2742f30"),  # 380_j09.14e      - "e14_j09.bin"
    ("380_j10.15e",  0x2000, "4c02b75a62993cce60d2cb87b81c7738abbc9a0d"),  # 380_j10.15e      - "e15_j10.bin"
    ("380_j11.16e",  0x2000, "d315588e6cc2f4263be621d2d8603c8215a90046"),  # 380_j11.16e      - "e16_j11.bin"
    # PROM
    ("380_j18.2a",   0x0020, "599acd25f36445221c553510a5de23ddba5ecc15"),  # palette          - "a02_j18.bin"
    ("380_j17.7b",   0x0100, "0d61d468f6d3e1570fd18d236ec8cab92db4ed5c"),  # char lut         - "b07_j17.bin"
    ("380_j16.10c",  0x0100, "86df21c8e0b1ed51a0a4bd33dbb33f6efdea7d39"),  # sprite lut       - "c10_j16.bin"
]

# ------------------------------------------------------------
# decode 4bpp "packed msb": 2 pixel/byte, nibble alto = pixel sinistro
# ------------------------------------------------------------
def decode_packed(data, base, w, h):
    tile = []
    for y in range(h):
        row = []
        for x in range(w):
            b = data[base + y * (w // 2) + (x >> 1)]
            row.append((b >> 4) & 0xF if (x & 1) == 0 else b & 0xF)
        tile.append(row)
    return tile

# rotazione galagino (portrait, ROT90 + 180 display): out[y][x] = mame[N-1-x][y]
def rot_galagino(tile):
    n = len(tile)
    return [[tile[n - 1 - x][y] for x in range(n)] for y in range(n)]

# ------------------------------------------------------------
# scritture header
# ------------------------------------------------------------
def write_tiles(tiles):
    # pre-ruotati; [8][4] nibble LSB-first (nibble basso = colonna pari)
    with open(os.path.join(OUT_DIR, "circusc_tilemap.h"), "w") as f:
        print("// Circus Charlie tiles — 512 tile 8x8 4bpp", file=f)
        print("// PRE-ROTATED ROT90 galagino; nibble LSB-first:", file=f)
        print("//   px = (tile[r][c>>1] >> ((c&1)*4)) & 0xF", file=f)
        print("const unsigned char circusc_tilemap[][8][4] = {", file=f)
        rows = []
        for t in tiles:
            lines = []
            for y in range(8):
                vals = []
                for xb in range(4):
                    v = t[y][2 * xb] | (t[y][2 * xb + 1] << 4)
                    vals.append(f"0x{v:02X}")
                lines.append("{" + ",".join(vals) + "}")
            rows.append(" {" + ",".join(lines) + "}")
        print(",\n".join(rows), file=f)
        print("};", file=f)

def write_sprites(sprites):
    # orientamento LANDSCAPE nativo (rotazione nel blit trasposto), una
    # sola copia (flip a runtime): [16][8] nibble LSB-first
    with open(os.path.join(OUT_DIR, "circusc_spritemap.h"), "w") as f:
        print("// Circus Charlie sprites — 384 sprite 16x16 4bpp", file=f)
        print("// LANDSCAPE orientation, flip at runtime (like rocnrope):", file=f)
        print("//   px = (spr[row][col>>1] >> ((col&1)*4)) & 0xF", file=f)
        print("const unsigned char circusc_spritemap[][16][8] = {", file=f)
        rows = []
        for s in sprites:
            lines = []
            for y in range(16):
                vals = []
                for xb in range(8):
                    v = s[y][2 * xb] | (s[y][2 * xb + 1] << 4)
                    vals.append(f"0x{v:02X}")
                lines.append("{" + ",".join(vals) + "}")
            rows.append(" {" + ",".join(lines) + "}")
        print(",\n".join(rows), file=f)
        print("};", file=f)

def rgb565_swapped(c):
    b = 31*((c>>6) & 0x3)//3
    g = 63*((c>>3) & 0x7)//7
    r = 31*((c>>0) & 0x7)//7
    rgb = (r << 11) + (g << 5) + b
    return ((rgb & 0xff00) >> 8) + ((rgb & 0xff) << 8)

def write_colormaps(pal_prom, char_lut, spr_lut):
    pal = [rgb565_swapped(c) for c in pal_prom]
    def nudge(v):
        return v if v != 0 else 0x2000  # 0x0000 reserved for transparency
    with open(os.path.join(OUT_DIR, "circusc_cmap.h"), "w") as f:
        print("// Colormap Circus Charlie da 380_j17.7b (char, pen+0x10) e 380_j16.10c", file=f)
        print("// (sprite). Tiles are OPAQUE.", file=f)
        print("// Sprite: trasparency = transpen_mask(c,0):", file=f)
        print("// pen p trasparent if lut[c*16+p]==lut[c*16+0] -> 0x0000", file=f)
        print("const unsigned short circusc_colormap_tiles[][16] = {", file=f)
        rows = []
        for g in range(16):
            vals = [hex(nudge(pal[16 + (char_lut[g*16+p] & 0x0f)])) for p in range(16)]
            rows.append("{" + ",".join(vals) + "}")
        print(",\n".join(rows), file=f)
        print("};", file=f)
        print("const unsigned short circusc_colormap_sprites[][16] = {", file=f)
        rows = []
        for g in range(16):
            vals = []
            t0 = spr_lut[g*16] & 0x0f
            for p in range(16):
                lut = spr_lut[g*16+p] & 0x0f
                vals.append(hex(0) if lut == t0 else hex(nudge(pal[lut])))
            rows.append("{" + ",".join(vals) + "}")
        print(",\n".join(rows), file=f)
        print("};", file=f)

def write_rom(name, sym, data, comment):
    with open(os.path.join(OUT_DIR, name), "w") as f:
        print(f"// {comment}", file=f)
        print(f"const unsigned char {sym}[] = {{", file=f)
        for i in range(0, len(data), 16):
            print("  " + ",".join(f"0x{b:02X}" for b in data[i:i+16]) + ",", file=f)
        print("};", file=f)

# ------------------------------------------------------------
def preview(tiles_rot, sprites, pal_prom, char_lut, spr_lut, outpng):
    try:
        from PIL import Image
    except ImportError:
        print("PIL import failed")
        return
    def pal_rgb(c):
        return (255*((c>>0)&7)//7, 255*((c>>3)&7)//7, 255*((c>>6)&3)//3)
    pal = [pal_rgb(c) for c in pal_prom]
    img = Image.new("RGB", (16*18, 32*9 + 8 + 24*18), (32, 32, 32))
    px = img.load()
    # 512 tile (pre-ruotati: a video appariranno come sul cabinet verticale)
    for t in range(512):
        gx, gy = (t % 16) * 9, (t // 16) * 9
        for y in range(8):
            for x in range(8):
                lut = char_lut[0*16 + tiles_rot[t][y][x]] & 0x0f
                px[gx + x, gy + y] = pal[16 + lut]
    # 384 sprite (landscape nativo, gruppo colore 1)
    base = 32*9 + 8
    for s in range(384):
        gx, gy = (s % 16) * 18, base + (s // 16) * 18
        t0 = spr_lut[1*16] & 0x0f
        for y in range(16):
            for x in range(16):
                lut = spr_lut[1*16 + sprites[s][y][x]] & 0x0f
                px[gx + x, gy + y] = (0,0,0) if lut == t0 else pal[lut]
    img = img.resize((img.width*2, img.height*2), Image.NEAREST)
    img.save(outpng)
    print("preview:", outpng)

# ------------------------------------------------------------
def main():

    print(f"Load ROM from: {os.path.abspath(ROM_SET)}")
    print(f"Target files:  {os.path.abspath(OUT_DIR)}")

    if not os.path.isfile(ROM_SET):
      print(f"ERROR: missing {ROM_SET}", file=sys.stderr)
      sys.exit(1)

    os.makedirs(OUT_DIR, exist_ok=True)

    roms = {}
    for name, size, sha1 in REQUIRED:
        roms[name] = load_file(ROM_SET, [name], sha1)

    # tiles: 16KB -> 512 tile pre-rotated
    tdata = roms["380_j12.4a"] + roms["380_j13.5a"]
    tiles = [rot_galagino(decode_packed(tdata, 32*t, 8, 8)) for t in range(512)]
    write_tiles(tiles)

    # sprites: 48KB -> 384 sprites in landscape
    sdata = roms["380_j06.11e"] + roms["380_j07.12e"] + roms["380_j08.13e"] + \
            roms["380_j09.14e"] + roms["380_j10.15e"] + roms["380_j11.16e"]
    sprites = [decode_packed(sdata, 128*s, 16, 16) for s in range(384)]
    write_sprites(sprites)

    write_colormaps(roms["380_j18.2a"], roms["380_j17.7b"], roms["380_j16.10c"])

    main_rom = roms["380_s05.3h"] + roms["380_q04.4h"] + roms["380_q03.5h"] + roms["380_q02.6h"] + roms["380_q01.7h"]

    write_rom("circusc_main_rom.h", "circusc_main_rom", main_rom,
              "Circus Charlie main KONAMI-1 ROM 0x6000-0xffff")

    write_rom("circusc_audio_rom.h", "circusc_audio_rom",
              roms["380_l14.5c"] + roms["380_l15.7c"],
              "Circus Charlie sound Z80 ROM 0x0000-0x3fff")

    #preview(tiles, sprites, roms["380_j18.2a"], roms["380_j17.7b"],
    #        roms["c10_j16.bin"], "circusc_preview.png")
    print("Circus Charlie conversion finished.")

if __name__ == "__main__":
    main()
