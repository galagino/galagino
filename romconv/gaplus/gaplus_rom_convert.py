#!/usr/bin/env python3
# ============================================================
# Gaplus / Galaga 3 (Namco 1984) ROM converter per galagino29-main
#
# ATTENZIONE romset IBRIDO (vedi memoria project_gaplus.md): il codice
# main+sub e' del set "galaga3" (Version 2/3 PCB), ma gfx1 (char) e i due
# prom colore sprite sono della variante "gaplus" (gp2-*, non gp3-*) --
# il file .zip fornito e' cosi', si usa il contenuto REALE per nome.
#
# Riferimenti MAME: gaplus.cpp / gaplus_v.cpp / gaplus_m.cpp (forniti
# dall'utente). Hardware gemello di galaga (STESSO tileaddr.h, stesso
# split videoram tile/attr 0x400+0x400, stesse 2 categorie di priorita')
# e di mappy/todruaga (stesso namcoio 56XX/58XX, stessa WSG 15XX, stessa
# trasformazione ROT90 per gli sprite: gal_x = 208-(sy+16*y), gal_y =
# sx+16*x -- qui pero' sizex/sizey sono INDIPENDENTI, non sempre quadrati,
# e c'e' il flag "duplicate" in piu' che gaplus ha e mappy no).
# ============================================================

import os
import sys
import wave

sys.dont_write_bytecode = True

from helper_functions import load_file

ROM_SET_GAPLUS  = os.path.normpath(os.path.join("..", "..", "romszip", "gaplus.zip"))
ROM_SET_GALAGA3 = os.path.normpath(os.path.join("..", "..", "romszip", "galaga3.zip"))
OUT_DIR = os.path.normpath(os.path.join("..", "..", "source", "src", "machines", "gaplus"))

GAPLUS_FILES = {
  "romset": {"name": "gaplus.zip", "description": "Gaplus (GP2 rev. B)" },

  "maincpu1":   {"names": ["gp2-4.8d"],   "sha1": "93fcd8b940491abf6344181811d0b35765d7e45c"}, #gaplus
  "maincpu2":   {"names": ["gp2-3b.8c"],  "sha1": "81402b28a2d5ac2d1301252534afa0cb65d7e162"}, #gaplus
  "maincpu3":   {"names": ["gp2-2b.8b"],  "sha1": "025c2f3978772e1ecbbf36842dc7c2203ee91a1f"}, #gaplus

  "subcpu1":    {"names": ["gp2-8.11d"],  "sha1": "f230eb0ad757f0714c0ac81c812e950778452947"}, #gaplus
  "subcpu2":    {"names": ["gp2-7.11c"],  "sha1": "b86020f819fefb134cb57e203f7c90b1b29581c8"}, #gaplus
  "subcpu3":    {"names": ["gp2-6.11b"],  "sha1": "398059da967c80321a9ec94d982a6c0b3c970c5f"}, #gaplus

  "soundcpu1":  {"names": ["gp2-1.4b"],   "sha1": "4e0a31d84cb7aca497485dbe0240009d58275765"}, #gaplus

  "gfx1_1":     {"names": ["gp2-5.8s"],   "sha1": "a0107fa4659597ac42c875ab1c0deb845534268b"}, #gaplus characters

  "gfx2_1":     {"names": ["gp2-11.11p"], "sha1": "16873e0ac5f975768d596d7d32af7571f4817f2b"}, #gaplus objects
  "gfx2_2":     {"names": ["gp2-10.11n"], "sha1": "fc346e98737c9fc20810e32d4c150ae4b4051979"}, #gaplus objects
  "gfx2_3":     {"names": ["gp2-12.11r"], "sha1": "368e4541a5151e906a189712bc05192c2ceec8ae"}, #gaplus objects
  "gfx2_4":     {"names": ["gp2-9.11m"],  "sha1": "99c1e67c3b216aa1b63f199e21c73cdedde80e1b"}, #gaplus objects

  "proms1":     {"names": ["gp2-3.1p"],   "sha1": "dcd6dfbfbd5281ba0c7b7c189d6fde23617ed3e3"}, #gaplus objects red palette ROM (4 bits)
  "proms2":     {"names": ["gp2-1.1n"],   "sha1": "c76f9d9b066e268621d41a703c5280261234709a"}, #gaplus green palette ROM (4 bits)
  "proms3":     {"names": ["gp2-2.2n"],   "sha1": "64d7b333f529d3ba66aeefd380fd1cbf9ddf460d"}, #gaplus blue palette ROM (4 bits)
  "proms4":     {"names": ["gp2-7.6s"],   "sha1": "781ffe9088476798409cb922350eff881590cf35"}, #gaplus char color ROM
  "proms5":     {"names": ["gp2-6.6p"],   "sha1": "955dcef363870ee8e91edc73b9ea3ce489738aad"}, #gaplus sprite color ROM (lower 4 bits)
  "proms6":     {"names": ["gp2-5.6n"],   "sha1": "a93a5bc448dc127e1389d10a9cb06acadfe940cf"}, #gaplus sprite color ROM (upper 4 bits)

  "soundprom" : {"names": ["gp2-4.3f"],   "sha1": "e6a23cd5ce3d3e76de3b70c8ab5a3c45b1147af4"}, #gaplus

  "plds1" :     {"names": ["pal10l8.8n"], "sha1": "1aa7fa1a61795703af84ae427d0d8588ef8c4c3f"}, #gaplus
}

GALAGA3_FILES = {
  "romset": {"name": "galaga3.zip", "description": "Galaga 3 (GP3 rev. D)" },

  "maincpu1":   {"names": ["gp3-4c.8d"],  "sha1": "e39f77af16016d28170e4ac1c2a784b0a7ec5454"},
  "maincpu2":   {"names": ["gp3-3c.8c"],  "sha1": "2b6bb2a5d77a837810180391ef6c0ce745bfed64"},
  "maincpu3":   {"names": ["gp3-2d.8b"],  "sha1": "b176b46bd6f2501d3a74ed11186be8411fd1105b"},

  "subcpu1":    {"names": ["gp3-8b.11d"], "sha1": "bbed2056dc28dc2828e29987c16d89fb16e7059e"},
  "subcpu2":    {"names": ["gp2-7.11c"],  "sha1": "b86020f819fefb134cb57e203f7c90b1b29581c8"},
  "subcpu3":    {"names": ["gp3-6b.11b"], "sha1": "a19f2942dafc899d686a42240fc2f7a7a7d3b1f5"},

  "soundcpu1":  {"names": ["gp2-1.4b"],   "sha1": "4e0a31d84cb7aca497485dbe0240009d58275765"},

  "gfx1_1":     {"names": ["gp3-5.8s"],   "sha1": "0a556b45976bc36eb99048b1512c446b472da1d2"}, # characters

  "gfx2_1":     {"names": ["gp2-11.11p"], "sha1": "16873e0ac5f975768d596d7d32af7571f4817f2b"}, # objects
  "gfx2_2":     {"names": ["gp2-10.11n"], "sha1": "fc346e98737c9fc20810e32d4c150ae4b4051979"}, # objects
  "gfx2_3":     {"names": ["gp2-12.11r"], "sha1": "368e4541a5151e906a189712bc05192c2ceec8ae"}, # objects
  "gfx2_4":     {"names": ["gp2-9.11m"],  "sha1": "99c1e67c3b216aa1b63f199e21c73cdedde80e1b"}, # objects

  "proms1":     {"names": ["gp2-3.1p"],   "sha1": "dcd6dfbfbd5281ba0c7b7c189d6fde23617ed3e3"}, # red palette ROM (4 bits)
  "proms2":     {"names": ["gp2-1.1n"],   "sha1": "c76f9d9b066e268621d41a703c5280261234709a"}, # green palette ROM (4 bits)
  "proms3":     {"names": ["gp2-2.2n"],   "sha1": "64d7b333f529d3ba66aeefd380fd1cbf9ddf460d"}, # blue palette ROM (4 bits)
  "proms4":     {"names": ["gp2-7.6s"],   "sha1": "781ffe9088476798409cb922350eff881590cf35"}, # char color ROM
  "proms5":     {"names": ["gp3-6.6p"],   "sha1": "6d0512958bc522d22e69336677369507847f8f6f"}, # sprite color ROM (lower 4 bits)
  "proms6":     {"names": ["gp3-5.6n"],   "sha1": "2ba51ccdd0428fc48758ed8fea36c8ce0e752a45"}, # sprite color ROM (upper 4 bits)

  "soundprom" : {"names": ["gp2-4.3f"],   "sha1": "e6a23cd5ce3d3e76de3b70c8ab5a3c45b1147af4"},

  "plds1" :     {"names": ["pal10l8.8n"], "sha1": "1aa7fa1a61795703af84ae427d0d8588ef8c4c3f"},
}

# ------------------------------------------------------------
# decoder gfx generico stile MAME (planes date come OFFSET BIT assoluti,
# stesso identico decoder di mappy_rom_convert.py)
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
# layout MAME (bit offset assoluti dentro il buffer GIA' ricostruito
# come da driver_init, vedi main())
# ------------------------------------------------------------
# charlayout: 8x8x2bpp, planeoffset={4,6}, 32 byte/char (256 bit)
CHAR_PLANES = [4, 6]
CHAR_XOFFS = [16*8, 16*8+1, 24*8, 24*8+1, 0, 1, 8*8, 8*8+1]
CHAR_YOFFS = [y*8 for y in range(8)]
CHAR_BITS_PER_TILE = 32*8
CHAR_COUNT = 512   # 0x4000 byte totali (dopo driver_init) / 32 byte

# spritelayout: 16x16x3bpp RGN_FRAC(1,2), planeoffset={RGN_FRAC(1,2),0,4}
# region gfx2 totale 0xC000 byte dopo driver_init -> RGN_FRAC(1,2) = meta'
# in BIT = 0xC000*8/2 = 0x30000
SPR_XOFFS = [0,1,2,3, 8*8,8*8+1,8*8+2,8*8+3, 16*8,16*8+1,16*8+2,16*8+3, 24*8,24*8+1,24*8+2,24*8+3]
SPR_YOFFS = [y*8 for y in range(8)] + [32*8 + y*8 for y in range(8)]
SPR_BITS_PER_TILE = 64*8
SPR_COUNT = 384    # 0x6000 (meta' regione) / 64 byte

# ------------------------------------------------------------
# write header
# ------------------------------------------------------------
def write_tiles(tiles):
    with open(os.path.join(OUT_DIR, "gaplus_tilemap.h"), "w") as f:
        print("// Gaplus tiles (gp2-5.8s + driver_init unpack nibble) — 512 tile 8x8 2bpp", file=f)
        print("const unsigned short gaplus_tilemap[][8] = {", file=f)
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
    # same as mappy/galaga: [0]=(fx0,fy0) [1]=(fx0,fy1) [2]=(fx1,fy0) [3]=(fx1,fy1)
    with open(os.path.join(OUT_DIR, "gaplus_spritemap.h"), "w") as f:
        print("// Gaplus sprites (gp2-11+gp2-10+gp2-12+gp2-9 + driver_init unpack) —", file=f)
        print("const unsigned long gaplus_sprites[][384][32] = {", file=f)
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

def rgb565_swapped(r, g, b):
    # r,g,b gia' 0..255 -> RGB565 byte-swapped, identico a cmapconv.py
    rgb = ((r*31//255) << 11) + ((g*63//255) << 5) + (b*31//255)
    return ((rgb & 0xff00) >> 8) + ((rgb & 0xff) << 8)

def decode_palette(red_prom, green_prom, blue_prom):
    # gaplus_palette(): resistenze pesate 0x0e/0x1f/0x43/0x8f sui 4 bit
    def comp(byte):
        v = 0
        if byte & 1: v += 0x0e
        if byte & 2: v += 0x1f
        if byte & 4: v += 0x43
        if byte & 8: v += 0x8f
        return v
    pal = []
    for i in range(256):
        r = comp(red_prom[i])
        g = comp(green_prom[i])
        b = comp(blue_prom[i])
        pal.append(rgb565_swapped(r, g, b))
    return pal

def nudge(v):
    return v if v != 0 else 0x2000  # nero vero -> quasi nero (0 e' il marcatore trasparenza)

def write_colormaps(pal, char_lut, spr_lut_lo, spr_lut_hi):
    with open(os.path.join(OUT_DIR, "gaplus_cmap.h"), "w") as f:
        print("// Colormap Gaplus da gp2-6s.bin (char lut, 64 gruppi x4 pen,", file=f)
        print("// pen = 0xF0+lut, SOLO 16 colori fisici usati per tutto il testo)", file=f)
        print("// e gp2-6n.bin/gp2-6p.bin (sprite lut upper/lower nibble, 64 gruppi", file=f)
        print("// x8 pen, colore DIRETTO lut_lo|lut_hi<<4 = indice 0-255 nella", file=f)
        print("// stessa tavolozza RGB della palette -- NON indiretto come mappy).", file=f)
        print("// Trasparenza hardware: MAME chiama configure_groups(gfx(0),0xff)", file=f)
        print("// -- QUALSIASI pixel char il cui lut (nibble basso) vale 0x0F", file=f)
        print("// risolve a pen indiretto 0xF0+0x0F=0xFF ed e' TRASPARENTE in", file=f)
        print("// ENTRAMBE le passate (non solo quella prioritaria): le stelle", file=f)
        print("// dello starfield, disegnate PRIMA della tilemap, devono restare", file=f)
        print("// visibili nei buchi. Sprite -> pen 0xFF trasparente (separato).", file=f)

        print("const unsigned short gaplus_colormap_tiles[][4] = {", file=f)
        rows = []
        for g in range(64):
            vals = []
            for p in range(4):
                lut = char_lut[g*4+p] & 0x0f
                vals.append(hex(0) if lut == 0x0f else hex(nudge(pal[0xf0 + lut])))
            rows.append("{" + ",".join(vals) + "}")
        print(",\n".join(rows), file=f)
        print("};", file=f)

        print("const unsigned short gaplus_colormap_tiles_prio[][4] = {", file=f)
        rows = []
        for g in range(64):
            vals = []
            for p in range(4):
                lut = char_lut[g*4+p] & 0x0f
                vals.append(hex(0) if lut == 0x0f else hex(nudge(pal[0xf0 + lut])))
            rows.append("{" + ",".join(vals) + "}")
        print(",\n".join(rows), file=f)
        print("};", file=f)

        print("const unsigned short gaplus_colormap_sprites[][8] = {", file=f)
        rows = []
        for g in range(64):
            vals = []
            for p in range(8):
                idx = g*8 + p
                lut = (spr_lut_lo[idx] & 0x0f) | ((spr_lut_hi[idx] & 0x0f) << 4)
                vals.append(hex(0) if lut == 0xff else hex(nudge(pal[lut])))
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
    with open(os.path.join(OUT_DIR, "gaplus_wavetable.h"), "w") as f:
        print("// Gaplus WSG 15XX waveforms: 8 waves x 32 samples", file=f)
        print("const signed char gaplus_wavetable[][32] = {", file=f)
        rows = []
        for w in range(8):
            vals = [str((prom[w*32+i] & 0x0f) - 8) for i in range(32)]
            rows.append(" { " + ",".join(vals) + " }")
        print(",\n".join(rows), file=f)
        print("};", file=f)

MAX_STARS = 250
STARFIELD_CLIPPING_X = 16
SCREEN_W, SCREEN_H = 288, 224

def gen_starfield():
    # from gaplus_v.cpp starfield_init() (LFSR 32bit)
    stars = []
    generator = 0
    sett = 0
    for y in range(SCREEN_H):
        for x in range(SCREEN_W - STARFIELD_CLIPPING_X*2 - 1, -1, -1):
            generator = (generator << 1) & 0xFFFFFFFF
            bit1 = (~generator >> 17) & 1
            bit2 = (generator >> 5) & 1
            if bit1 ^ bit2:
                generator |= 1
            if ((~generator) >> 16) & 1 and (generator & 0xff) == 0xff:
                color = ((~(generator >> 8)) & 0xFFFFFFFF) % 7 + 1
                color_base = {0: 0x250, 1: 0x230, 2: 0x210}[sett]
                if color and len(stars) < MAX_STARS:
                    stars.append((x + STARFIELD_CLIPPING_X, y, color_base + color, sett))
                    sett = (sett + 1) % 3
    return stars

def write_starfield(stars, pal, spr_lut_lo, spr_lut_hi):
    with open(os.path.join(OUT_DIR, "gaplus_starseed.h"), "w") as f:
        print("struct gaplus_star_S { unsigned short x, y; unsigned short col; unsigned char set; };", file=f)
        print(f"#define GAPLUS_NUM_STARS {len(stars)}", file=f)
        print("const struct gaplus_star_S gaplus_starseed[] = {", file=f)
        rows = []
        for (x, y, penidx, sett) in stars:
            local = penidx - 256
            lut = (spr_lut_lo[local] & 0x0f) | ((spr_lut_hi[local] & 0x0f) << 4)
            col = 0 if lut == 0xff else nudge(pal[lut])
            rows.append(f" {{ {x}, {y}, {hex(col)}, {sett} }}")
        print(",\n".join(rows), file=f)
        print("};", file=f)

def write_sample_bang():
    try:
        import numpy as np
    except ImportError:
        print("numpy non disponibile: gaplus_sample_bang.h NON generato")
        return
    path = os.path.join("..", "..", "samples", "gaplus_bang.wav")
    if not os.path.exists(path):
        print("gaplus_bang.wav missing, gaplus_sample_bang.h not generated")
        return
    w = wave.open(path, "rb")
    nch, sw, fr, nframes = w.getnchannels(), w.getsampwidth(), w.getframerate(), w.getnframes()
    frames = w.readframes(nframes)
    dtype = {1: np.int8, 2: "<i2", 4: "<i4"}[sw]
    data = np.frombuffer(frames, dtype=dtype).astype(np.float64)
    if nch > 1:
        data = data.reshape(-1, nch).mean(axis=1)

    target_fr = 24000
    n_out = int(len(data) * target_fr / fr)
    x_old = np.linspace(0, 1, len(data), endpoint=False)
    x_new = np.linspace(0, 1, n_out, endpoint=False)
    resampled = np.interp(x_new, x_old, data)

    peak = np.abs(resampled).max()
    scale = 127.0 / peak if peak > 0 else 1.0
    samples8 = np.clip(np.round(resampled * scale), -128, 127).astype(np.int8)

    with open(os.path.join(OUT_DIR, "gaplus_sample_bang.h"), "w") as f:
        print(f"// Explosion samples for \"bang\" (samples/gaplus_bang.wav),", file=f)
        print(f"// resampled @24000 Hz signed 8bit (bit pattern in unsigned char,", file=f)
        print(f"// {len(samples8)} samples ({len(samples8)/target_fr:.3f}s a 24000 Hz)", file=f)
        print("const unsigned char gaplus_sample_bang[] = {", file=f)
        vals = [f"0x{(int(v) & 0xFF):02X}" for v in samples8]
        for i in range(0, len(vals), 16):
            print("  " + ",".join(vals[i:i+16]) + ",", file=f)
        print("};", file=f)
    print(f"gaplus_sample_bang.h: {len(samples8)} samples @24kHz ({len(samples8)/target_fr:.3f}s)")

# ------------------------------------------------------------
# preview PNG
# ------------------------------------------------------------
def preview(tiles, sprites, pal, char_lut, spr_lut_lo, spr_lut_hi, outpng):
    try:
        from PIL import Image
    except ImportError:
        print("PIL not available")
        return
    def unswap(c):
        rgb = ((c & 0xff) << 8) | (c >> 8)
        r = (rgb >> 11) & 0x1f
        g = (rgb >> 5) & 0x3f
        b = rgb & 0x1f
        return (r*255//31, g*255//63, b*255//31)
    img = Image.new("RGB", (16*18, 32*9 + 8*18 + 8), (32, 32, 32))
    px = img.load()
    for t in range(min(512, len(tiles))):
        gx, gy = (t % 16) * 9, (t // 16) * 9
        for y in range(8):
            for x in range(8):
                lut = char_lut[1*4 + tiles[t][y][x]] & 0x0f
                px[gx + x, gy + y] = unswap(pal[0xf0 + lut])
    base = 32*9 + 8
    for s in range(min(128, len(sprites))):
        gx, gy = (s % 16) * 18, base + (s // 16) * 18
        for y in range(16):
            for x in range(16):
                v = sprites[s][y][x]
                lut = (spr_lut_lo[v] & 0x0f) | ((spr_lut_hi[v] & 0x0f) << 4)
                px[gx + x, gy + y] = (0,0,0) if lut == 0xff else unswap(pal[lut])
    img = img.resize((img.width*3, img.height*3), Image.NEAREST)
    img.save(outpng)
    print("preview:", outpng)

# ------------------------------------------------------------
def convert_gaplus(romset, files, galagino):

  os.makedirs(OUT_DIR, exist_ok=True)

  print(f"Load ROM from: {os.path.abspath(romset)}")
  print(f"Target files:  {os.path.abspath(OUT_DIR)}")

  cpu01 = load_file(romset, files["maincpu1"]["names"], files["maincpu1"]["sha1"])
  cpu02 = load_file(romset, files["maincpu2"]["names"], files["maincpu2"]["sha1"])
  cpu03 = load_file(romset, files["maincpu3"]["names"], files["maincpu3"]["sha1"])

  sub01 = load_file(romset, files["subcpu1"]["names"], files["subcpu1"]["sha1"])
  sub02 = load_file(romset, files["subcpu2"]["names"], files["subcpu2"]["sha1"])
  sub03 = load_file(romset, files["subcpu3"]["names"], files["subcpu3"]["sha1"])

  soundcpu = load_file(romset, files["soundcpu1"]["names"], files["soundcpu1"]["sha1"])

  gfx1_1 = load_file(romset, files["gfx1_1"]["names"], files["gfx1_1"]["sha1"])

  gfx2_1 = load_file(romset, files["gfx2_1"]["names"], files["gfx2_1"]["sha1"])
  gfx2_2 = load_file(romset, files["gfx2_2"]["names"], files["gfx2_2"]["sha1"])
  gfx2_3 = load_file(romset, files["gfx2_3"]["names"], files["gfx2_3"]["sha1"])
  gfx2_4 = load_file(romset, files["gfx2_4"]["names"], files["gfx2_4"]["sha1"])

  proms1 = load_file(romset, files["proms1"]["names"], files["proms1"]["sha1"])
  proms2 = load_file(romset, files["proms2"]["names"], files["proms2"]["sha1"])
  proms3 = load_file(romset, files["proms3"]["names"], files["proms3"]["sha1"])
  proms4 = load_file(romset, files["proms4"]["names"], files["proms4"]["sha1"])
  proms5 = load_file(romset, files["proms5"]["names"], files["proms5"]["sha1"])
  proms6 = load_file(romset, files["proms6"]["names"], files["proms6"]["sha1"])

  soundprom = load_file(romset, files["soundprom"]["names"], files["soundprom"]["sha1"])

  plds1 = load_file(romset, files["plds1"]["names"], files["plds1"]["sha1"])

  char_src = gfx1_1 # gaplus="gp2-5.8s" galaga3="gp3-5.8s"
  char_full = bytearray(0x4000)
  char_full[0:0x2000] = char_src
  for i in range(0x2000):
    char_full[0x2000 + i] = char_src[i] >> 4
  tiles = [rot_galagino(t) for t in
           mame_decode(bytes(char_full), 8, 8, CHAR_PLANES, CHAR_XOFFS, CHAR_YOFFS,
                         CHAR_BITS_PER_TILE, CHAR_COUNT)]
  write_tiles(tiles)

  # --- gfx2 (sprite): replica driver_init: 4 rom da 8KB @0/0x2000/0x4000/
  # 0x6000, poi rom[0x8000+i] = rom[0x6000+i]<<4 per i<0x2000, poi
  # 0xA000-0xBFFF a zero (ROM_FILL, "optional non usata") ---
  spr_full = bytearray(0xC000)
  spr_full[0x0000:0x2000] = gfx2_1 #gaplus="gp2-11.11p"
  spr_full[0x2000:0x4000] = gfx2_2 #gaplus="gp2-10.11n"
  spr_full[0x4000:0x6000] = gfx2_3 #gaplus="gp2-12.11r"
  spr_full[0x6000:0x8000] = gfx2_4 #gaplus="gp2-9.11m"
  for i in range(0x2000):
    spr_full[0x8000 + i] = (gfx2_4[i] << 4) & 0xff #gaplus="gp2-9.11m"
  # 0xa000-0xbfff resta a zero (bytearray gia' inizializzato a 0)

  half_bits = (0xC000 * 8) // 2  # RGN_FRAC(1,2)
  SPR_PLANES = [half_bits, 0, 4]
  sprites = [rot_galagino(s) for s in
             mame_decode(bytes(spr_full), 16, 16, SPR_PLANES, SPR_XOFFS, SPR_YOFFS,
                         SPR_BITS_PER_TILE, SPR_COUNT)]
  write_sprites(sprites)

  pal = decode_palette(proms1, proms2, proms3) #"gp2-1.1p" "gp2-1.1n" "gp2-2.2n"
  write_colormaps(pal, proms4, proms5, proms6) #"gp2-6s.bin" "gp2-6p.bin" "gp2-6n.bin"
  write_wavetable(soundprom)                   #"gp2-4.3f"

  main_rom = cpu01 + cpu02 + cpu03             #"gp3-4c.8d" "gp3-3c.8c" "gp3-2d.8b"
  assert len(main_rom) == 0x6000
  write_rom("gaplus_rom_main.h", "gaplus_rom_main", main_rom,
            "Gaplus main M6809 ROM 0xA000-0xFFFF")

  sub_rom = sub01 + sub02 + sub03              #"gp3-8b.11d" "gp2-7.11c" "gp3-6b.11b"
  assert len(sub_rom) == 0x6000
  write_rom("gaplus_rom_sub.h", "gaplus_rom_sub", sub_rom,
            "Gaplus sub M6809 ROM 0xA000-0xFFFF")

  sub_rom2 = soundcpu                          #"gp2-1.4b"
  assert len(sub_rom2) == 0x2000
  write_rom("gaplus_rom_sub2.h", "gaplus_rom_sub2", soundcpu,
            "Gaplus sound (sub2) M6809 ROM 0xE000-0xFFFF")

  stars = gen_starfield()
  write_starfield(stars, pal, proms5, proms6)  #"gp2-6p.bin" "gp2-6n.bin"
  print(f"Starfield: {len(stars)} stars generated")

  write_sample_bang()

  #preview(tiles, sprites, pal, proms4, proms5, proms6, "gaplus_preview.png") # "gp2-6s.bin" "gp2-6p.bin" "gp2-6n.bin"
  print("Gaplus conversion finished.")

# ------------------------------------------------------------

def main():
  if os.path.isfile(ROM_SET_GAPLUS):
    convert_gaplus(ROM_SET_GAPLUS, GAPLUS_FILES, {})
  elif os.path.isfile(ROM_SET_GALAGA3):
    convert_gaplus(ROM_SET_GALAGA3, GALAGA3_FILES, {})
  else:
    print("ERROR: No roms.")
    sys.exit(1)

if __name__ == "__main__":
    main()
