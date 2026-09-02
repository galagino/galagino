#!/usr/bin/env python3
"""
ROM converter for MotoRace USA (Irem, 1983) - travrusa.cpp MAME driver
Generates header files for GALAGONE ESP32-S3 emulator.

Hardware: Z80 @ 3.072MHz, Irem M52 sound, ROT270
  Tiles: 8x8, 3bpp planar, 1024 tiles
  Sprites: 16x16, 3bpp planar, 256 sprites
  Colors: 128 char + 16 sprite base colors, 128-entry sprite lookup

"""

import os
import sys
import zipfile
import hashlib

sys.dont_write_bytecode = True

ROM_SET = os.path.normpath(os.path.join("..", "..", "romszip", "motorace.zip"))
OUT_DIR = os.path.normpath(os.path.join("..", "..", "source", "src", "machines", "motorace"))

z = zipfile.ZipFile(ROM_SET)

def load(name):
    return z.read(name)

# ══════════════════════════════════════════════════════════════════
# 1. CPU ROM — decrypt mr.cpu + concatenate mr1-mr3
# ══════════════════════════════════════════════════════════════════
def bitswap16(val, *bits):
    result = 0
    for i, b in enumerate(bits):
        if val & (1 << b):
            result |= 1 << (15 - i)
    return result

def bitswap8(val, *bits):
    result = 0
    for i, b in enumerate(bits):
        if val & (1 << b):
            result |= 1 << (7 - i)
    return result

def decrypt_motorace(data):
    """MAME init_motorace: address and data line scrambling for first 8KB ROM"""
    buf = bytearray(data)
    rom = bytearray(0x2000)
    for A in range(0x2000):
        # Address bitswap: 15,14,13, 9,7,5,3,1, 12,10,8,6,4,2,0, 11
        j = bitswap16(A, 15,14,13, 9,7,5,3,1, 12,10,8,6,4,2,0, 11)
        j &= 0x1FFF
        # Data bitswap: 2,7,4,1,6,3,0,5
        rom[j] = bitswap8(buf[A], 2,7,4,1,6,3,0,5)
    return bytes(rom)

print("Loading CPU ROMs...")
cpu0 = decrypt_motorace(load("mr.cpu"))
cpu1 = load("mr1.3l")
cpu2 = load("mr2.3k")
cpu3 = load("mr3.3j")
cpu_rom = cpu0 + cpu1 + cpu2 + cpu3  # 32KB total
print(f"  CPU ROM: {len(cpu_rom)} bytes")

# ══════════════════════════════════════════════════════════════════
# 2. SOUND ROM — mr10.1a (4KB, loaded at offset 0x7000)
# ══════════════════════════════════════════════════════════════════
print("Loading Sound ROM...")
snd_rom_data = load("mr10.1a")
# Pad to 8KB with 0xFF before the data (it loads at 0x7000 in 8KB region)
snd_rom = b'\xff' * 0x3000 + snd_rom_data
print(f"  Sound ROM: {len(snd_rom_data)} bytes (padded to {len(snd_rom)})")

# ══════════════════════════════════════════════════════════════════
# 3. TILE GFX — mr7.3e, mr8.3c, mr9.3a (3 planes, 8KB each)
#    Format: 8x8, 3bpp planar (gfx_8x8x3_planar)
#    1024 tiles, each plane has 8 bytes per tile
# ══════════════════════════════════════════════════════════════════
print("Loading Tile ROMs...")
tile_plane0 = load("mr7.3e")   # plane 0 (bit 0)
tile_plane1 = load("mr8.3c")   # plane 1 (bit 1)
tile_plane2 = load("mr9.3a")   # plane 2 (bit 2)
num_tiles = len(tile_plane0) // 8  # 1024 tiles
print(f"  Tiles: {num_tiles}")

def decode_tile_3bpp(p0, p1, p2, tile_idx):
    """Decode one 8x8 tile from 3 planar ROMs. Returns 8 rows of 8 pixels (0-7).
    Uses LSB-first bit order (bit 0 = pixel 0) matching MAME's charlayout
    xoffsets {0,1,2,3,4,5,6,7} for travrusa."""
    base = tile_idx * 8
    rows = []
    for row in range(8):
        b0 = p0[base + row]
        b1 = p1[base + row]
        b2 = p2[base + row]
        pixels = []
        for bit in range(8):  # LSB first: pixel 0 = bit 0 (per MAME xoffsets)
            px = ((b0 >> bit) & 1) | (((b1 >> bit) & 1) << 1) | (((b2 >> bit) & 1) << 2)
            pixels.append(px)
        rows.append(pixels)
    return rows

def rotate_tile_cw90(tile_8x8):
    """Rotate 8x8 tile 90° clockwise for ROT270 portrait display.
       portrait[py][px] = landscape[7-px][py]"""
    rotated = []
    for py in range(8):
        row = []
        for px in range(8):
            row.append(tile_8x8[7 - px][py])
        rotated.append(row)
    return rotated

def pack_tile_row(pixels_8):
    """Pack 8 pixels (3bpp each) into unsigned long (32 bits).
    px0 at MSB (shift 28), px7 at LSB (shift 0) — reversed nibble order
    so the renderer can scan pixel_col from high to low for ROT270."""
    val = 0
    for i, px in enumerate(pixels_8):
        val |= (px & 7) << ((7 - i) * 4)
    return val

# Convert all tiles WITHOUT rotation (landscape orientation)
# The renderer will handle ROT270 mapping using NOT(counters) like the VHDL hardware
tiles_packed = []
for t in range(num_tiles):
    landscape = decode_tile_3bpp(tile_plane0, tile_plane1, tile_plane2, t)
    packed = [pack_tile_row(landscape[r]) for r in range(8)]
    tiles_packed.append(packed)

# ══════════════════════════════════════════════════════════════════
# 4. SPRITE GFX — mr4.3n, mr5.3m, mr6.3k (3 planes, 8KB each)
#    Format: 16x16, 3bpp planar
#    MAME spritelayout: planes {2/3, 1/3, 0/3}, xoffsets {0-7, 128+0..128+7}, yoffsets {0*8..15*8}
#    charincrement = 32*8 = 256 bits = 32 bytes per sprite per plane
#    256 sprites total
# ══════════════════════════════════════════════════════════════════
print("Loading Sprite ROMs...")
spr_plane0 = load("mr4.3n")   # RGN_FRAC(0,3) = LSB (bit 0)
spr_plane1 = load("mr5.3m")   # RGN_FRAC(1,3) = mid (bit 1)
spr_plane2 = load("mr6.3k")   # RGN_FRAC(2,3) = MSB (bit 2)
num_sprites = len(spr_plane0) // 32  # 256 sprites
print(f"  Sprites: {num_sprites}")

def decode_sprite_3bpp(p0, p1, p2, spr_idx):
    """Decode one 16x16 sprite from 3 planar ROMs.
    MAME layout: xoffsets = {0,1,2,3,4,5,6,7, 16*8+0,...16*8+7}
                 yoffsets = {0*8,1*8,...,15*8}
    So: top-left 8 cols from bytes 0-15, top-right 8 cols from bytes 16-31.
    Left half: LSB-first (nibble-reversed vs tiles) for correct sprite rendering.
    Right half: LSB-first with same plane order as left half (b0→bit0).
    """
    base = spr_idx * 32
    rows = []
    for row in range(16):
        pixels = []
        # Left half (columns 0-7): bytes at base + row
        b0 = p0[base + row]
        b1 = p1[base + row]
        b2 = p2[base + row]
        for bit in range(8):  # LSB first for sprites
            px = ((b0 >> bit) & 1) | (((b1 >> bit) & 1) << 1) | (((b2 >> bit) & 1) << 2)
            pixels.append(px)
        # Right half (columns 8-15): bytes at base + 16 + row
        b0 = p0[base + 16 + row]
        b1 = p1[base + 16 + row]
        b2 = p2[base + 16 + row]
        for bit in range(8):  # LSB first, same plane order as left half
            px = ((b0 >> bit) & 1) | (((b1 >> bit) & 1) << 1) | (((b2 >> bit) & 1) << 2)
            pixels.append(px)
        rows.append(pixels)
    return rows  # 16 rows x 16 pixels

def rotate_sprite_cw90(spr_16x16):
    """Rotate 16x16 sprite 90° clockwise for ROT270 portrait display."""
    rotated = []
    for py in range(16):
        row = []
        for px in range(16):
            row.append(spr_16x16[15 - px][py])
        rotated.append(row)
    return rotated

def flip_sprite(spr_16x16, flip_x, flip_y):
    """Apply horizontal/vertical flip."""
    result = []
    for r in range(16):
        src_r = (15 - r) if flip_y else r
        row = spr_16x16[src_r]
        if flip_x:
            row = list(reversed(row))
        result.append(row)
    return result

def pack_sprite_row(pixels_16):
    """Pack 16 pixels (3bpp each) into two unsigned longs (4 bits per pixel).
    First 8 pixels in word0, next 8 in word1.
    Reversed nibble order: px0 at MSB (shift 28), px7 at LSB (shift 0)
    so the renderer can scan columns correctly for ROT270."""
    w0 = 0
    w1 = 0
    for i in range(8):
        w0 |= (pixels_16[i] & 7) << ((7 - i) * 4)
    for i in range(8):
        w1 |= (pixels_16[8 + i] & 7) << ((7 - i) * 4)
    return (w0, w1)

# Convert all sprites with 4 orientations (no flip, flipY, flipX, flipXY)
# NO rotation — stored in landscape orientation, renderer handles ROT270
sprites_packed = []  # [orientation][sprite_idx] = list of 16 (w0,w1) tuples
for flip_x, flip_y in [(False, False), (False, True), (True, False), (True, True)]:
    orientation = []
    for s in range(num_sprites):
        landscape = decode_sprite_3bpp(spr_plane0, spr_plane1, spr_plane2, s)
        flipped = flip_sprite(landscape, flip_x, flip_y)
        packed = [pack_sprite_row(flipped[r]) for r in range(16)]
        orientation.append(packed)
    sprites_packed.append(orientation)

# ══════════════════════════════════════════════════════════════════
# 5. COLOR PROMS
# ══════════════════════════════════════════════════════════════════
print("Loading Color PROMs...")
char_prom = load("mmi6349.ij")    # 512 bytes, char palette (first 128 used)
spr_prom = load("tbp18s.2")       # 32 bytes, sprite base palette (16 entries)
spr_lookup = load("tbp24s10.3")   # 256 bytes, sprite color lookup table

def prom_to_rgb(byte_val):
    """Convert travrusa color PROM byte to RGB, matching MAME travrusa_palette exactly.
    Red: 2 bits (7,6) with 2-resistor network (470, 220 ohm).
    Green/Blue: 3 bits each with 3-resistor network (1K, 470, 220 ohm)."""
    # Red: bits 7,6 (2 resistors: 470, 220 ohm) — per MAME travrusa_palette
    r = 0x52 * ((byte_val >> 6) & 1) + 0xAD * ((byte_val >> 7) & 1)
    # Green: bits 5,4,3 (3 resistors: 1K, 470, 220)
    g = 0x21 * ((byte_val >> 3) & 1) + 0x47 * ((byte_val >> 4) & 1) + 0x97 * ((byte_val >> 5) & 1)
    # Blue: bits 2,1,0 (3 resistors: 1K, 470, 220)
    b = 0x21 * ((byte_val >> 0) & 1) + 0x47 * ((byte_val >> 1) & 1) + 0x97 * ((byte_val >> 2) & 1)
    return min(r, 255), min(g, 255), min(b, 255)

def rgb_to_565_swapped(r, g, b):
    """Convert RGB888 to RGB565 byte-swapped for ESP32 SPI display."""
    r5 = (r >> 3) & 0x1F
    g6 = (g >> 2) & 0x3F
    b5 = (b >> 3) & 0x1F
    val = (r5 << 11) | (g6 << 5) | b5
    return ((val & 0xFF) << 8) | ((val >> 8) & 0xFF)

# Character colors: 128 indirect colors (indices 0-127)
# Each PROM byte maps directly to a color
char_colors_rgb = []
for i in range(128):
    r, g, b = prom_to_rgb(char_prom[i])
    char_colors_rgb.append(rgb_to_565_swapped(r, g, b))

# Sprite base colors: 16 entries from sprite PROM (at offset 0x200 in combined PROMs)
spr_base_rgb = []
for i in range(16):
    r, g, b = prom_to_rgb(spr_prom[i])
    spr_base_rgb.append(rgb_to_565_swapped(r, g, b))

# Character colormap: 16 palettes x 8 colors
# Characters use direct color mapping: palette * 8 + pixel_value → char_colors[palette*8+pixel]
# In MAME: pen_indirect(i, i) for i < 0x80, so char pixel i → indirect color i
# tile attr & 0x0F = palette (0-15), pixel 0-7 → color index = palette*8 + pixel
char_colormap = []
for pal in range(16):
    colors = []
    for px in range(8):
        idx = pal * 8 + px
        if idx < 128:
            colors.append(char_colors_rgb[idx])
        else:
            colors.append(0)
    char_colormap.append(colors)

# Sprite colormap: 16 palettes x 8 colors
# MAME: pen_indirect(0x80+i, (spr_lookup[i] & 0x0F) | 0x80) for sprite pens
# Sprite pixel (0-7) with attr palette (0-15):
#   lookup_index = palette * 8 + pixel (128 entries in lookup table, but only 0x80 used)
#   Wait - the lookup table is 256 bytes (0x100) at offset 0x220
#   For sprites: pen i (0x80 to 0xFF) → ctabentry = (lookup[i-0x80] & 0x0F) | 0x80
#   So: pen = attr_palette * 8 + pixel_value (0-127 within sprite range)
#   → indirect_color = (spr_lookup[pen] & 0x0F) | 0x80
#   → spr_base_rgb[spr_lookup[pen] & 0x0F]
spr_colormap = []
for pal in range(16):
    colors = []
    for px in range(8):
        lookup_idx = pal * 8 + px
        if lookup_idx < len(spr_lookup):
            base_idx = spr_lookup[lookup_idx] & 0x0F
            colors.append(spr_base_rgb[base_idx])
        else:
            colors.append(0)
    spr_colormap.append(colors)

# ══════════════════════════════════════════════════════════════════
# 6. WRITE HEADER FILES
# ══════════════════════════════════════════════════════════════════

os.makedirs(OUT_DIR, exist_ok=True)

# ── motorace_rom.h ──
print("Writing motorace_rom.h...")
with open(os.path.join(OUT_DIR, "motorace_rom.h"), "w") as f:
    f.write("// MotoRace USA - CPU ROM (32KB, first 8KB decrypted)\n")
    f.write("// Auto-generated by motorace_rom_convert.py\n\n")
    f.write(f"const unsigned char motorace_rom[{len(cpu_rom)}] PROGMEM = {{\n")
    for i in range(0, len(cpu_rom), 16):
        chunk = cpu_rom[i:i+16]
        f.write("  " + ", ".join(f"0x{b:02x}" for b in chunk) + ",\n")
    f.write("};\n")

# ── motorace_snd_rom.h ──
print("Writing motorace_snd_rom.h...")
with open(os.path.join(OUT_DIR, "motorace_snd_rom.h"), "w") as f:
    f.write("// MotoRace USA - Sound ROM (4KB at offset 0x7000)\n")
    f.write("// Auto-generated by motorace_rom_convert.py\n\n")
    f.write(f"const unsigned char motorace_snd_rom[{len(snd_rom)}] PROGMEM = {{\n")
    for i in range(0, len(snd_rom), 16):
        chunk = snd_rom[i:i+16]
        f.write("  " + ", ".join(f"0x{b:02x}" for b in chunk) + ",\n")
    f.write("};\n")

# ── motorace_tilemap.h ──
print("Writing motorace_tilemap.h...")
with open(os.path.join(OUT_DIR, "motorace_tilemap.h"), "w") as f:
    f.write("// MotoRace USA - Tilemap: 1024 tiles, 8x8, 3bpp, pixel-order fixed\n")
    f.write("// 4 bits per pixel packed in unsigned long (8 pixels per row)\n")
    f.write("// Auto-generated by motorace_rom_convert.py\n\n")
    f.write(f"const unsigned long motorace_tilemap[{num_tiles}][8] PROGMEM = {{\n")
    for t in range(num_tiles):
        row_strs = [f"0x{tiles_packed[t][r]:08x}" for r in range(8)]
        f.write("  { " + ", ".join(row_strs) + " },\n")
    f.write("};\n")

# ── motorace_spritemap.h ──
print("Writing motorace_spritemap.h...")
with open(os.path.join(OUT_DIR, "motorace_spritemap.h"), "w") as f:
    f.write("// MotoRace USA - Spritemap: 256 sprites, 16x16, 3bpp, 4 orientations\n")
    f.write("// Two unsigned longs per row (8+8 pixels), 4 bits per pixel\n")
    f.write("// Auto-generated by motorace_rom_convert.py\n\n")
    f.write(f"const unsigned long motorace_spritemap[4][{num_sprites}][32] PROGMEM = {{\n")
    for ori in range(4):
        f.write("  {\n")
        for s in range(num_sprites):
            vals = []
            for r in range(16):
                w0, w1 = sprites_packed[ori][s][r]
                vals.append(f"0x{w0:08x}")
                vals.append(f"0x{w1:08x}")
            f.write("    { " + ", ".join(vals) + " },\n")
        f.write("  },\n")
    f.write("};\n")

# ── motorace_cmap.h ──
print("Writing motorace_cmap.h...")
with open(os.path.join(OUT_DIR, "motorace_cmap.h"), "w") as f:
    f.write("// MotoRace USA - Color maps (RGB565 byte-swapped for ESP32 SPI)\n")
    f.write("// Auto-generated by motorace_rom_convert.py\n\n")

    # Character colormap: 16 palettes x 8 colors
    f.write("// Character colormap: 16 palettes x 8 colors\n")
    f.write(f"const unsigned short motorace_char_cmap[16][8] PROGMEM = {{\n")
    for pal in range(16):
        vals = [f"0x{char_colormap[pal][c]:04x}" for c in range(8)]
        f.write("  { " + ", ".join(vals) + " },\n")
    f.write("};\n\n")

    # Sprite colormap: 16 palettes x 8 colors
    f.write("// Sprite colormap: 16 palettes x 8 colors\n")
    f.write(f"const unsigned short motorace_spr_cmap[16][8] PROGMEM = {{\n")
    for pal in range(16):
        vals = [f"0x{spr_colormap[pal][c]:04x}" for c in range(8)]
        f.write("  { " + ", ".join(vals) + " },\n")
    f.write("};\n")

print(f"\nDone! Files written to {OUT_DIR}")
