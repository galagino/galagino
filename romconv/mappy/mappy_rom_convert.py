#!/usr/bin/env python3
# ============================================================
# Mappy (Namco 1983) ROM converter
#
# ============================================================

import os
import sys

sys.dont_write_bytecode = True

from helper_functions import load_file

ROM_SET = os.path.normpath(os.path.join("..", "..", "romszip", "mappy.zip"))
OUT_DIR = os.path.normpath(os.path.join("..", "..", "source", "src", "machines", "mappy"))

MAPPY_FILES = {
  "romset": {"name": "mappy.zip", "description": "Mappy (US)" },

  "maincpu1": {"names": ["mpx_3.1d"], "sha1": "b9722941438e93325e84691ada4e95620bec73b2"}, 
  "maincpu2": {"names": ["mp1_2.1c"], "sha1": "e5198703cdf47b2cd7fc9f2a5fde7bf4ab2275db"},
  "maincpu3": {"names": ["mpx_1.1b"], "sha1": "1dbc4f42d4c16a08240a221bec27dcc3a8dd7461"},

  "subcpu":   {"names": ["mp1_4.1k"], "sha1": "f36b57f7f1e79f00b3f07afe1960bca5f5325ee2"},

  "tiles":    {"names": ["mp1_5.3b"], "sha1": "76610149c65f955484fef1c033ddc3fed3f4e568"},
  
  "sprites1": {"names": ["mp1_6.3m"], "sha1": "3cc216793c6a5f73c437ad2524563deb3b5e2890"},
  "sprites2": {"names": ["mp1_7.3n"], "sha1": "8dfbf03953d5219d9eb5fc654ec3392442ba1dc4"},

  "proms1":   {"names": ["mp1-5.5b"], "sha1": "2e356706c07f43eeb67783fb122bdc7fed1b3589"},
  "proms2":   {"names": ["mp1-6.4c"], "sha1": "f578e14f15783acb2073644db4a2f0d196cc0957"},
  "proms3":   {"names": ["mp1-7.5k"], "sha1": "2e387e5d8b8cab005f67f821b4db65d0ae8bd362"},
  "proms4":   {"names": ["mp1-3.3m"], "sha1": "847cbaf7c88616576c410177e066ae1d792ac0ba"},
}

# ------------------------------------------------------------
# decoder gfx generico stile MAME (planes[0] = bit PIU' significativo)
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

def flip_tile(tile, fx, fy):
    out = tile
    if fy: out = list(reversed(out))
    if fx: out = [list(reversed(r)) for r in out]
    return out

# ------------------------------------------------------------
# layout MAME
# ------------------------------------------------------------
# charlayout pacman/galaga/mappy: 8x8x2bpp, 16 byte/char
CHAR_PLANES = [0, 4]
CHAR_XOFFS = [64, 65, 66, 67, 0, 1, 2, 3]
CHAR_YOFFS = [y * 8 for y in range(8)]

# galaga spritelayout (per autotest): 16x16x2bpp, 64 byte/sprite
GSPR_PLANES = [0, 4]
GSPR_XOFFS = [0,1,2,3, 64,65,66,67, 128,129,130,131, 192,193,194,195]
GSPR_YOFFS = [y*8 for y in range(8)] + [256 + y*8 for y in range(8)]

# mappy spritelayout_4bpp: 16x16x4bpp, 128 byte/sprite (stream interallacciato)
MSPR_PLANES = [0, 4, 8, 12]
MSPR_XOFFS = [0,1,2,3, 128,129,130,131, 256,257,258,259, 384,385,386,387]
MSPR_YOFFS = [y*16 for y in range(8)] + [512 + y*16 for y in range(8)]

# ------------------------------------------------------------
# AUTOTEST: il decoder generico + rot_galagino deve riprodurre
# ESATTAMENTE parse_chr/parse_sprite (copiati da tileconv/spriteconv)
# sulle ROM di galaga gia' validate su HW
# ------------------------------------------------------------
def parse_chr_ref(data):
    char = []
    for y in range(8):
        row = []
        for x in range(8):
            byte = data[15 - x - 2*(y&4)]
            c0 = 1 if byte & (0x08 >> (y&3)) else 0
            c1 = 2 if byte & (0x80 >> (y&3)) else 0
            row.append(c0+c1)
        char.append(row)
    return char

def parse_sprite_ref(data):
    sprite = []
    for y in range(16):
        row = []
        for x in range(16):
            idx = ((y&8)<<1) + (((x&8)^8)<<2) + (7-(x&7)) + 2*(y&4)
            c0 = 1 if data[idx] & (0x08 >> (y&3)) else 0
            c1 = 2 if data[idx] & (0x80 >> (y&3)) else 0
            row.append(c0+c1)
        sprite.append(row)
    return sprite

def selftest():
    with open(os.path.join(ROMS, "gg1_9.4l"), "rb") as f:
        cdata = f.read()
    dec = mame_decode(cdata, 8, 8, CHAR_PLANES, CHAR_XOFFS, CHAR_YOFFS, 128, 256)
    for t in range(256):
        ref = parse_chr_ref(cdata[16*t:16*(t+1)])
        got = rot_galagino(dec[t])
        if got != ref:
            raise AssertionError(f"AUTOTEST char {t} FALLITO")
    with open(os.path.join(ROMS, "gg1_11.4d"), "rb") as f:
        sdata = f.read()
    dec = mame_decode(sdata, 16, 16, GSPR_PLANES, GSPR_XOFFS, GSPR_YOFFS, 512, 64)
    for t in range(64):
        ref = parse_sprite_ref(sdata[64*t:64*(t+1)])
        got = rot_galagino(dec[t])
        if got != ref:
            raise AssertionError(f"AUTOTEST sprite {t} FALLITO")
    print("Autotest decoder vs tileconv/spriteconv (ROM galaga): OK")

# ------------------------------------------------------------
# scritture header
# ------------------------------------------------------------
def write_tiles(tiles):
    with open(os.path.join(OUT_DIR, "mappy_tilemap.h"), "w") as f:
        print("// Mappy tiles (mp1_5.3b, ROMREGION_INVERT) — 256 tile 8x8 2bpp", file=f)
        print("// pixel LSB-first come galaga_tilemap (blit: (pix>>2c)&3)", file=f)
        print("const unsigned short mappy_tilemap[][8] = {", file=f)
        rows = []
        for t in tiles:
            vals = []
            for y in range(8):
                v = 0
                for x in range(8):
                    v |= t[y][x] << (2*x)
                vals.append(hex(v))
            rows.append(" { " + ",".join(vals) + " }")
        print(",\n".join(rows), file=f)
        print("};", file=f)

def write_sprites(sprites):
    # varianti come spriteconv galaga: [0]=(fx0,fy0) [1]=(fx0,fy1) [2]=(fx1,fy0) [3]=(fx1,fy1)
    with open(os.path.join(OUT_DIR, "mappy_spritemap.h"), "w") as f:
        print("// Mappy sprites (mp1_6.3m+mp1_7.3n interallacciate) — 128 sprite 16x16 4bpp", file=f)
        print("// [variante flip][codice][riga*2+meta']: nibble LSB-first,", file=f)
        print("// [2r]=pixel 0-7, [2r+1]=pixel 8-15 (stile 1942 4bpp)", file=f)
        print("const unsigned long mappy_sprites[][128][32] = {", file=f)
        for (fx, fy) in [(0,0),(0,1),(1,0),(1,1)]:
            print(" {", file=f)
            rows = []
            for s in sprites:
                t = flip_tile(s, fx, fy)
                vals = []
                for y in range(16):
                    v = 0
                    for x in range(16):
                        v |= t[y][x] << (4*x)
                    vals.append(hex(v & 0xffffffff))
                    vals.append(hex(v >> 32))
                rows.append("  { " + ",".join(vals) + " }")
            print(",\n".join(rows), file=f)
            print(" }," if not (fx and fy) else " }", file=f)
        print("};", file=f)

def rgb565_swapped(c):
    # bbgggrrr -> RGB565 byte-swapped, identico a cmapconv.py (galaga/pacman)
    b = 31*((c>>6) & 0x3)//3
    g = 63*((c>>3) & 0x7)//7
    r = 31*((c>>0) & 0x7)//7
    rgb = (r << 11) + (g << 5) + b
    return ((rgb & 0xff00) >> 8) + ((rgb & 0xff) << 8)

def write_colormaps(pal_prom, char_lut, spr_lut):
    pal = [rgb565_swapped(c) for c in pal_prom]  # 32 colori
    # 0x0000 e' riservato alla trasparenza nei blit: i neri veri diventano quasi-neri
    def nudge(v):
        return v if v != 0 else 0x2000  # r=1 (formato swapped) ~ invisibile
    with open(os.path.join(OUT_DIR, "mappy_cmap.h"), "w") as f:
        print("// Colormap Mappy da mp1-6.4c (char, pen+0x10) e mp1-7.5k (sprite)", file=f)
        print("// Trasparenza hardware = nibble lookup 0xF:", file=f)
        print("//  - tiles opachi: colore reale (prima passata, tilemap OPACO)", file=f)
        print("//  - tiles_prio: 0x0000 dove lookup==0xF (ridisegno tile prioritari", file=f)
        print("//    sopra gli sprite: attr bit6, si salta il colore 'trasparente')", file=f)
        print("//  - sprites: 0x0000 dove lookup==0xF (pixel non disegnato)", file=f)
        print("const unsigned short mappy_colormap_tiles[][4] = {", file=f)
        rows = []
        for g in range(64):
            vals = [hex(nudge(pal[16 + (char_lut[g*4+p] & 0x0f)])) for p in range(4)]
            rows.append("{" + ",".join(vals) + "}")
        print(",\n".join(rows), file=f)
        print("};", file=f)
        print("const unsigned short mappy_colormap_tiles_prio[][4] = {", file=f)
        rows = []
        for g in range(64):
            vals = []
            for p in range(4):
                lut = char_lut[g*4+p] & 0x0f
                vals.append(hex(0) if lut == 0x0f else hex(nudge(pal[16 + lut])))
            rows.append("{" + ",".join(vals) + "}")
        print(",\n".join(rows), file=f)
        print("};", file=f)
        print("const unsigned short mappy_colormap_sprites[][16] = {", file=f)
        rows = []
        for g in range(16):
            vals = []
            for p in range(16):
                lut = spr_lut[g*16+p] & 0x0f
                vals.append(hex(0) if lut == 0x0f else hex(nudge(pal[lut])))
            rows.append("{" + ",".join(vals) + "}")
        print(",\n".join(rows), file=f)
        print("};", file=f)

def write_rom(name, sym, data, comment):
    with open(os.path.join(OUT_DIR, name), "w") as f:
        print(f"// {comment}", file=f)
        print(f"const unsigned char {sym}[] = {{", file=f)
        for i in range(0, len(data), 16):
            print("  " + ",".join(f"0x{b:02x}" for b in data[i:i+16]) + ",", file=f)
        print("};", file=f)

def write_wavetable(prom):
    # 256 byte, 4 bit bassi = 8 forme d'onda x 32 campioni, centrate (-8..7)
    with open(os.path.join(OUT_DIR, "mappy_wavetable.h"), "w") as f:
        print("// Mappy WSG 15XX waveforms (mp1-3.3m): 8 forme x 32 campioni", file=f)
        print("const signed char mappy_wavetable[][32] = {", file=f)
        rows = []
        for w in range(8):
            vals = [str((prom[w*32+i] & 0x0f) - 8) for i in range(32)]
            rows.append(" { " + ",".join(vals) + " }")
        print(",\n".join(rows), file=f)
        print("};", file=f)

# ------------------------------------------------------------
# preview PNG (validazione offline orientamento/decode)
# ------------------------------------------------------------
def preview(tiles, sprites, pal_prom, char_lut, spr_lut, outpng):
    try:
        from PIL import Image
    except ImportError:
        print("PIL missing, no preview")
        return
    def pal_rgb(c):
        return (255*((c>>0)&7)//7, 255*((c>>3)&7)//7, 255*((c>>6)&3)//3)
    pal = [pal_rgb(c) for c in pal_prom]
    # tiles: griglia 16x16 (144px), sprite: griglia 16x8 (288px)
    img = Image.new("RGB", (16*18, 16*9 + 8*18 + 8), (32, 32, 32))
    px = img.load()
    for t in range(256):
        gx, gy = (t % 16) * 9, (t // 16) * 9
        for y in range(8):
            for x in range(8):
                lut = char_lut[1*4 + tiles[t][y][x]] & 0x0f
                px[gx + x, gy + y] = pal[16 + lut]
    # sprites: griglia 16x8, gruppo colore 0
    base = 16*9 + 8
    for s in range(128):
        gx, gy = (s % 16) * 18, base + (s // 16) * 18
        for y in range(16):
            for x in range(16):
                lut = spr_lut[0*16 + sprites[s][y][x]] & 0x0f
                px[gx + x, gy + y] = (0,0,0) if lut == 0x0f else pal[lut]
    img = img.resize((img.width*3, img.height*3), Image.NEAREST)
    img.save(outpng)
    print("preview:", outpng)

# ------------------------------------------------------------
def convert_mappy(romset, files, galagino):

  os.makedirs(OUT_DIR, exist_ok=True)

  print(f"Load ROM from: {os.path.abspath(romset)}")
  print(f"Target files:  {os.path.abspath(OUT_DIR)}")

  cpu01 = load_file(romset, files["maincpu1"]["names"], files["maincpu1"]["sha1"])
  cpu02 = load_file(romset, files["maincpu2"]["names"], files["maincpu2"]["sha1"])
  cpu03 = load_file(romset, files["maincpu3"]["names"], files["maincpu3"]["sha1"])

  subcpu = load_file(romset, files["subcpu"]["names"], files["subcpu"]["sha1"])

  tiles1 = load_file(romset, files["tiles"]["names"], files["tiles"]["sha1"])

  sprites1 = load_file(romset, files["sprites1"]["names"], files["sprites1"]["sha1"])
  sprites2 = load_file(romset, files["sprites2"]["names"], files["sprites2"]["sha1"])

  proms1 = load_file(romset, files["proms1"]["names"], files["proms1"]["sha1"])
  proms2 = load_file(romset, files["proms2"]["names"], files["proms2"]["sha1"])
  proms3 = load_file(romset, files["proms3"]["names"], files["proms3"]["sha1"])
  proms4 = load_file(romset, files["proms4"]["names"], files["proms4"]["sha1"])

  #this requires galaga files
  #selftest() 

  # tiles (inverted: ROMREGION_INVERT)
  tdata = bytes(b ^ 0xFF for b in tiles1)
  tiles = [rot_galagino(t) for t in
           mame_decode(tdata, 8, 8, CHAR_PLANES, CHAR_XOFFS, CHAR_YOFFS, 128, 256)]
  write_tiles(tiles)

  # sprites: stream interlaced 16 bit (mp1_6 = even bytes, mp1_7 = odd bytes)
  r6, r7 = sprites1, sprites2
  sdata = bytearray(0x4000)
  sdata[0::2] = r6
  sdata[1::2] = r7
  sprites = [rot_galagino(s) for s in
             mame_decode(bytes(sdata), 16, 16, MSPR_PLANES, MSPR_XOFFS, MSPR_YOFFS, 1024, 128)]
  write_sprites(sprites)

  write_colormaps(proms1, proms2, proms3)
  write_wavetable(proms4)

  main_rom = cpu01 + cpu02 + cpu03
  write_rom("mappy_rom_main.h", "mappy_rom_main", main_rom,
            "Mappy main M6809 ROM 0xA000-0xFFFF (mpx_3.1d+mp1_2.1c+mpx_1.1b)")
  write_rom("mappy_rom_sub.h", "mappy_rom_sub", subcpu,
            "Mappy sound M6809 ROM 0xE000-0xFFFF (mp1_4.1k)")

  #preview(tiles, sprites, roms["mp1-5.5b"], roms["mp1-6.4c"], roms["mp1-7.5k"],
  #        "mappy_preview.png")
  print("Mappy conversion finished.")

def main():
  if os.path.isfile(ROM_SET):
    convert_mappy(ROM_SET, MAPPY_FILES, {})
  else:
    print("ERROR: No roms.")
    sys.exit(1)

if __name__ == "__main__":
    main()
