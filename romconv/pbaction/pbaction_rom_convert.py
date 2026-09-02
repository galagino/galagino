#!/usr/bin/env python3
# ============================================================
# pbaction_rom_convert.py
#
# One-shot ROM converter for the Galagino Plus `pbaction` machine.
# The `pbaction` machine IS MAME `pbaction` = "Pinball Action (set 1)"
# (Tehkan, 1985, unencrypted Z80 + Z80 audio, ROT90).
#
# With no arguments it reads GalaginoPlus-main\romszip\pbaction.zip and writes
# the headers into GalaginoPlus-main\source\src\machines\pbaction\ (both paths
# resolved relative to this script, so the working directory does not matter).
# It verifies every size + CRC32 against the MAME `pbaction` set first, so a
# wrong/renamed dump is caught early.
#
# USAGE
#     python3 pbaction_rom_convert.py
#
# INPUT FILES  (MAME `pbaction` set 1 - `mame pbaction -verifyroms` must pass)
#     b-p7.bin b-n7.bin b-l7.bin           maincpu   0x0000 / 0x4000 / 0x8000
#     a-e3.bin                             audiocpu  0x0000 (0x2000)
#     a-s6.bin a-s7.bin a-s8.bin           fgchars   3 x 0x2000  (charlayout1, 3bpp)
#     a-j5.bin a-j6.bin a-j7.bin a-j8.bin  bgchars 4 x 0x4000 (charlayout2, 4bpp)
#     b-c7.bin b-d7.bin b-f7.bin           sprites   3 x 0x2000  (spritelayout1/2, 3bpp)
#
# OUTPUT HEADERS  (written to OUTDIR)
#     pbaction_main_rom.h    const unsigned char pbaction_main_rom[0xc000]
#     pbaction_audio_rom.h   const unsigned char pbaction_audio_rom[0x2000]
#     pbaction_fg_tiles.h    const unsigned char pbaction_fg_tiles[1024][8][8]   (pen 0-7)
#     pbaction_bg_tiles.h    const unsigned char pbaction_bg_tiles[2048][8][8]   (pen 0-15)
#     pbaction_sprites16.h   const unsigned char pbaction_sprites16[256][16][16] (pen 0-7)
#     pbaction_sprites32.h   const unsigned char pbaction_sprites32[32][32][32]  (pen 0-7)
#
# All gfx are rotated 90 degrees clockwise here (ROT90 cabinet), matching the
# galagino blitter convention used by bnj / bombjack (rot_galagino()).
#
# NOT generated (hand-authored / not ROM-derived):
#     pbaction_dipswitches.h  - DIP config (verify vs `mame pbaction -listxml`)
#     pbaction_logo.h         - custom menu artwork (already present)
#     palette                 - pbaction has NO colour PROM; the palette is
#                               256 x xBGR_444 written by the CPU into palette
#                               RAM at 0xe400-0xe5ff and decoded live by the
#                               port. Nothing to extract.
#
# MAME reference: src/mame/tehkan/pbaction.cpp  (driver by Nicola Salmoria)
#   main_map            0x0000-0xbfff ROM
#   gfx_pbaction        GFXDECODE_START, charlayout1/2, spritelayout1/2
#   get_bg/fg_tile_info tile code math (bg: +0x10*(attr&0x70) -> 2048 codes;
#                                       fg: +0x10*(attr&0x30) -> 1024 codes)
# ============================================================

import os
import sys

sys.dont_write_bytecode = True
from helper_functions import load_file

ROM_SET = os.path.normpath(os.path.join("..", "..", "romszip", "pbaction.zip"))
OUT_DIR = os.path.normpath(os.path.join("..", "..", "source", "src", "machines", "pbaction"))

# --- MAME pbaction (set 1) ROM manifest: name -> (size, SHA1) -----------
PBACTION_ROMS = {
    # maincpu
    "b-p7.bin": "c9e605f9d291cb8c7163655ea96c605b7d30365f",
    "b-n7.bin": "a4c3205bfe5fba8bb1ff3ad15941a77c35b44a27",
    "b-l7.bin": "e75731d9bea80e0dc09798dd46e3b947fdb54aaa",
    # audiocpu
    "a-e3.bin": "df2827197cd55c3685e5ac8b26c20800623cb932",
    # fgchars (charlayout1, 3bpp)
    "a-s6.bin": "bd27439b91f41db3fd7eedb44e828d61b793bda0",
    "a-s7.bin": "7c8eff087f18cc2ff0572ea45e681a3a1ec94fad",
    "a-s8.bin": "74b6d926b8f456c8d0101f0232c5d3662423b396",
    # bgchars (charlayout2, 4bpp)
    "a-j5.bin": "0c0a05a26d793ba98b0f421d464ff4b1d301ff9e",
    "a-j6.bin": "18795ecbcd2da94f1cfcce5559d652388d1b8bc0",
    "a-j7.bin": "52ab15c63332f0fa98884fa9adc8d35b93c939c4",
    "a-j8.bin": "58f48d24903b797e8451bf231f9e8df621685d9f",
    # sprites (spritelayout1 16x16 + spritelayout2 32x32, 3bpp)
    "b-c7.bin": "69ad8e419e340d2f548468ed7838102789b978da",
    "b-d7.bin": "060f70ed6386c808303a488c97691257681bd8f3",
    "b-f7.bin": "56f47d25761b3850c49a3a81b5ea35f12bd77b14",
}

# --------------------------------------------------------------------------
def rs_load(name):

  return load_file(ROM_SET, [name], PBACTION_ROMS[name])

# --------------------------------------------------------------------------
class RomSource:
    """Reads the pbaction ROM files either from a .zip
       with size + CRC32 verification against the MAME manifest above.
    """

    def __init__(self, path: Path):
        if zipfile.is_zipfile(path):
            self.zip = zipfile.ZipFile(path, "r")
            self.dir = None
            self.label = str(path)
            # map lower-case basename -> real name inside the archive
            self._members = {Path(n).name.lower(): n for n in self.zip.namelist()}
        else:
            sys.exit(f"ERROR: {path} not found in zip archive")

    def _raw(self, name: str) -> bytes:
        if self.zip is not None:
            member = self._members.get(name.lower())
            if member is None:
                sys.exit(f"ERROR: {name} not found in {self.label}")
            return self.zip.read(member)
        p = self.dir / name
        if not p.exists():
            sys.exit(f"ERROR: missing {name} in {self.label}")
        return p.read_bytes()

    def load(self, name: str) -> bytes:
        """Read one ROM, checking size and (optionally) CRC32."""
        size, crc = ROMS[name]
        b = self._raw(name)
        if len(b) != size:
            sys.exit(f"ERROR: {name}: expected {size} bytes, got {len(b)}")
        if True:
            got = zlib.crc32(b) & 0xFFFFFFFF
            if got != crc:
                sys.exit(
                    f"ERROR: {name}: CRC32 0x{got:08x}, expected 0x{crc:08x}. "
                    f"Wrong or bad dump (run `mame pbaction -verifyroms`)."
                )
        return b


# --- generic MAME planar gfx decoder --------------------------------------
# planes / xoffs / yoffs are ABSOLUTE BIT offsets into `data` (measured from
# `base_bit`), exactly like gfx_element::decode() in src/emu/drawgfx.cpp.
# planes[0] is the most-significant pen bit, planes[-1] the least.
def mame_decode(data: bytes, width: int, height: int, planes: list[int],
                xoffs: list[int], yoffs: list[int], bits_per_tile: int,
                count: int, base_bit: int = 0) -> list[list[list[int]]]:
    tiles = []
    for t in range(count):
        base = base_bit + t * bits_per_tile
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


def rot_galagino(tile: list[list[int]]) -> list[list[int]]:
    """Rotate a square tile 90 degrees clockwise (ROT90 cabinet), matching
    the galagino blitter convention (rot_galagino() in bnj/bombjack)."""
    n = len(tile)
    return [[tile[n - 1 - x][y] for x in range(n)] for y in range(n)]


# --------------------------------------------------------------------------
def build_main_rom() -> bytes:
    """MAME ROM_REGION 0xc000 "maincpu":
        b-p7.bin @0x0000 (0x4000), b-n7.bin @0x4000 (0x4000),
        b-l7.bin @0x8000 (0x4000).  0xc000-0xffff = RAM / video / I/O."""
    cpu = bytearray(b"\xff" * 0xc000)
    cpu[0x0000:0x4000] = rs_load("b-p7.bin")
    cpu[0x4000:0x8000] = rs_load("b-n7.bin")
    cpu[0x8000:0xA000] = rs_load("b-l7.bin")
    return bytes(cpu)


def build_audio_rom() -> bytes:
    """MAME ROM_REGION 0x2000 "audiocpu": a-e3.bin @0x0000 (0x2000).
    The audio map only uses 0x0000-0x1fff as ROM, so 0x2000 bytes suffice."""
    return rs_load("a-e3.bin")


def build_fg_tiles() -> list[list[list[int]]]:
    """charlayout1 (fgchars), GFXDECODE gfx[0]:
        8,8  RGN_FRAC(1,3)  3 planes
        planeoffset { RGN_FRAC(0,3), RGN_FRAC(1,3), RGN_FRAC(2,3) }
        xoffs STEP8(0,1)   yoffs STEP8(0,8)   charincrement 8*8

    region = a-s6 + a-s7 + a-s8 (0x6000). planeoffset[0]=RGN_FRAC(0,3) is the
    MSB and points at a-s6; planeoffset[2]=RGN_FRAC(2,3) is the LSB -> a-s8.
    count = (0x6000*8) / 3 / (8*8) = 1024 tiles (matches the 0x400 fg codes
    the tile-info handler can address: videoram + 0x10*(attr&0x30))."""
    region = b"".join(rs_load(n) for n in ("a-s6.bin", "a-s7.bin", "a-s8.bin"))
    rf = len(region) * 8 // 3               # RGN_FRAC(1,3) in bits
    planes = [0 * rf, 1 * rf, 2 * rf]       # MSB .. LSB
    xoffs = [i for i in range(8)]
    yoffs = [y * 8 for y in range(8)]
    bpt = 8 * 8
    count = rf // bpt
    assert count == 1024, count
    return mame_decode(region, 8, 8, planes, xoffs, yoffs, bpt, count)


def build_bg_tiles() -> list[list[list[int]]]:
    """charlayout2 (bgchars), GFXDECODE gfx[1]:
        8,8  RGN_FRAC(1,4)  4 planes
        planeoffset { RGN_FRAC(0,4), RGN_FRAC(1,4), RGN_FRAC(2,4), RGN_FRAC(3,4) }
        xoffs STEP8(0,1)   yoffs STEP8(0,8)   charincrement 8*8

    region = a-j5 + a-j6 + a-j7 + a-j8 (0x10000). planeoffset[0]=RGN_FRAC(0,4)
    is the MSB -> a-j5; planeoffset[3]=RGN_FRAC(3,4) is the LSB -> a-j8.
    count = (0x10000*8) / 4 / (8*8) = 2048 tiles (matches the 0x800 bg codes:
    videoram + 0x10*(attr&0x70))."""
    region = b"".join(rs_load(n)
                      for n in ("a-j5.bin", "a-j6.bin", "a-j7.bin", "a-j8.bin"))
    rf = len(region) * 8 // 4
    planes = [0 * rf, 1 * rf, 2 * rf, 3 * rf]   # MSB .. LSB
    xoffs = [i for i in range(8)]
    yoffs = [y * 8 for y in range(8)]
    bpt = 8 * 8
    count = rf // bpt
    assert count == 2048, count
    return mame_decode(region, 8, 8, planes, xoffs, yoffs, bpt, count)


def build_sprites16() -> list[list[list[int]]]:
    """spritelayout1, GFXDECODE gfx[2] (offset 0x00000 into "sprites"):
        16,16  RGN_FRAC(1,3)  3 planes
        planeoffset { RGN_FRAC(0,3), RGN_FRAC(1,3), RGN_FRAC(2,3) }
        xoffs { STEP8(0,1), STEP8(64,1) }
        yoffs { STEP8(0,8), STEP8(128,8) }
        charincrement 32*8

    region = b-c7 + b-d7 + b-f7 (0x6000). planeoffset[0]=MSB -> b-c7,
    planeoffset[2]=LSB -> b-f7. count = (0x6000*8)/3/(32*8) = 256."""
    region = b"".join(rs_load(n) for n in ("b-c7.bin", "b-d7.bin", "b-f7.bin"))
    rf = len(region) * 8 // 3
    planes = [0 * rf, 1 * rf, 2 * rf]
    xoffs = [i for i in range(8)] + [64 + i for i in range(8)]
    yoffs = [8 * i for i in range(8)] + [128 + 8 * i for i in range(8)]
    bpt = 32 * 8
    count = rf // bpt
    assert count == 256, count
    return mame_decode(region, 16, 16, planes, xoffs, yoffs, bpt, count)


def build_sprites32() -> list[list[list[int]]]:
    """spritelayout2, GFXDECODE gfx[3] (offset 0x01000 into "sprites"):
        32,32  RGN_FRAC(1,6)  3 planes
        planeoffset { RGN_FRAC(0,3), RGN_FRAC(1,3), RGN_FRAC(2,3) }
        xoffs { STEP8(0,1), STEP8(64,1), STEP8(256,1), STEP8(320,1) }
        yoffs { STEP8(0,8), STEP8(128,8), STEP8(512,8), STEP8(640,8) }
        charincrement 128*8

    NOTE the plane FRACTIONS here are thirds of the WHOLE 0x6000 region
    (RGN_FRAC(n,3)), while the tile count is RGN_FRAC(1,6) and the GFXDECODE
    entry starts 0x1000 BYTES into the region. So:
        region  = b-c7 + b-d7 + b-f7  (0x6000)
        rf3     = 0x6000*8/3          (plane stride, bits)
        base    = 0x1000*8            (start offset, bits)
        planes  = [0*rf3, 1*rf3, 2*rf3]   MSB..LSB  (b-c7 / b-d7 / b-f7)
        count   = RGN_FRAC(1,6) / (128*8) = (0x6000*8/6) / 1024 = 32
    """
    region = b"".join(rs_load(n) for n in ("b-c7.bin", "b-d7.bin", "b-f7.bin"))
    rf3 = len(region) * 8 // 3
    base = 0x1000 * 8
    planes = [0 * rf3, 1 * rf3, 2 * rf3]
    xoffs = ([i for i in range(8)] + [64 + i for i in range(8)]
             + [256 + i for i in range(8)] + [320 + i for i in range(8)])
    yoffs = ([8 * i for i in range(8)] + [128 + 8 * i for i in range(8)]
             + [512 + 8 * i for i in range(8)] + [640 + 8 * i for i in range(8)])
    bpt = 128 * 8
    count = (len(region) * 8 // 6) // bpt
    assert count == 32, count
    return mame_decode(region, 32, 32, planes, xoffs, yoffs, bpt, count, base_bit=base)


# --- header writers ------------------------------------------------------
BANNER = "// Generated from the MAME pbaction - Pinball Action (set 1) ROM set. Do not edit.\n"


def write_bytes_header(path: Path, guard: str, sym: str, data: bytes, note: str) -> None:
    with open(path, "w") as f:
        f.write(f"#ifndef {guard}\n#define {guard}\n\n")
        f.write(BANNER)
        f.write(f"// {note}\n\n")
        f.write(f"const unsigned char {sym}[{len(data)}] = {{\n")
        for i in range(0, len(data), 16):
            f.write("  " + ",".join(f"0x{b:02X}" for b in data[i:i + 16]) + ",\n")
        f.write("};\n\n#endif\n")
    print(f"  wrote {path} {len(data):#8x} bytes")


def write_tiles_header(path: Path, guard: str, sym: str, tiles: list, dim: int, note: str) -> None:
    with open(path, "w") as f:
        f.write(f"#ifndef {guard}\n#define {guard}\n\n")
        f.write(BANNER)
        f.write(f"// {note}\n")
        f.write(f"// {len(tiles)} tiles, {dim}x{dim} pixels, rotated 90 deg CW (ROT90).\n\n")
        f.write(f"const unsigned char {sym}[{len(tiles)}][{dim}][{dim}] = {{\n")
        for i, t in enumerate(tiles):
            rows = ["{" + ",".join(str(v) for v in t[y]) + "}" for y in range(dim)]
            f.write("  {" + ",".join(rows) + "}")
            f.write(",\n" if i < len(tiles) - 1 else "\n")
        f.write("};\n\n#endif\n")
    print(f"  wrote {path} {len(tiles)} tiles ({dim}x{dim})")


# --- optional PNG preview ----------------------------------------------
def preview(fg, bg, s16, s32, outpng: Path) -> None:
    try:
        from PIL import Image
    except ImportError:
        print("  (PIL not installed - skipping preview)")
        return

    PAL = [(20, 20, 20), (200, 60, 60), (60, 200, 60), (60, 60, 200),
           (200, 200, 60), (200, 60, 200), (60, 200, 200), (230, 230, 230),
           (120, 40, 40), (40, 120, 40), (40, 40, 120), (120, 120, 40),
           (120, 40, 120), (40, 120, 120), (150, 150, 150), (255, 255, 255)]

    def block(tiles, dim, cols):
        rows = (len(tiles) + cols - 1) // cols
        img = Image.new("RGB", (cols * (dim + 1), rows * (dim + 1)), (32, 32, 96))
        px = img.load()
        for i, t in enumerate(tiles):
            ox, oy = (i % cols) * (dim + 1), (i // cols) * (dim + 1)
            for y in range(dim):
                for x in range(dim):
                    px[ox + x, oy + y] = PAL[t[y][x] & 15]
        return img

    parts = [block(fg, 8, 32), block(bg, 8, 32),
             block(s16, 16, 32), block(s32, 32, 16)]
    W = max(p.width for p in parts)
    H = sum(p.height + 6 for p in parts)
    img = Image.new("RGB", (W, H), (0, 0, 0))
    y = 0
    for p in parts:
        img.paste(p, (0, y))
        y += p.height + 6
    img = img.resize((img.width * 2, img.height * 2), Image.NEAREST)
    img.save(outpng)
    print(f"  wrote {outpng.name} (preview)")


# --------------------------------------------------------------------------
def main() -> None:

    print(f"Load ROM from: {os.path.abspath(ROM_SET)}")
    print(f"Target files:  {os.path.abspath(OUT_DIR)}")

    if not os.path.isfile(ROM_SET):
      print(f"ERROR: missing {ROM_SET}", file=sys.stderr)
      sys.exit(1)

    os.makedirs(OUT_DIR, exist_ok=True)

    write_bytes_header(os.path.join(OUT_DIR, "pbaction_main_rom.h"), "PBACTION_MAIN_ROM_H",
                       "pbaction_main_rom", build_main_rom(),
                       "Z80 main CPU space: b-p7 @0x0000, b-n7 @0x4000, b-l7 @0x8000; "
                       "0xa000+ is RAM/video/I/O (filled 0xFF).")
    write_bytes_header(os.path.join(OUT_DIR, "pbaction_audio_rom.h"), "PBACTION_AUDIO_ROM_H",
                       "pbaction_audio_rom", build_audio_rom(),
                       "Z80 audio CPU ROM a-e3.bin (0x0000-0x1fff).")

    fg = [rot_galagino(t) for t in build_fg_tiles()]
    bg = [rot_galagino(t) for t in build_bg_tiles()]
    s16 = [rot_galagino(t) for t in build_sprites16()]
    s32 = [rot_galagino(t) for t in build_sprites32()]

    write_tiles_header(os.path.join(OUT_DIR, "pbaction_fg_tiles.h"), "PBACTION_FG_TILES_H",
                       "pbaction_fg_tiles", fg, 8,
                       "Foreground chars (a-s6/s7/s8), charlayout1, 3bpp (pen 0-7). "
                       "gfx[0]: color = attr&0x0f, transparent pen 0.")
    write_tiles_header(os.path.join(OUT_DIR, "pbaction_bg_tiles.h"), "PBACTION_BG_TILES_H",
                       "pbaction_bg_tiles", bg, 8,
                       "Background chars (a-j5/j6/j7/j8), charlayout2, 4bpp (pen 0-15). "
                       "gfx[1]: color = attr&0x07, opaque, palette base 128.")
    write_tiles_header(os.path.join(OUT_DIR, "pbaction_sprites16.h"), "PBACTION_SPRITES16_H",
                       "pbaction_sprites16", s16, 16,
                       "Normal sprites (b-c7/d7/f7), spritelayout1, 3bpp (pen 0-7). "
                       "gfx[2]: color = spriteram[offs+1]&0x0f, transparent pen 0.")
    write_tiles_header(os.path.join(OUT_DIR, "pbaction_sprites32.h"), "PBACTION_SPRITES32_H",
                       "pbaction_sprites32", s32, 32,
                       "Large sprites (same ROMs, +0x1000), spritelayout2, 3bpp (pen 0-7). "
                       "gfx[3]: color = spriteram[offs+1]&0x0f, transparent pen 0.")

    #preview(fg, bg, s16, s32, "pbaction_gfx_preview.png")

    print("Pinball Action Conversion completed.")


if __name__ == "__main__":
    main()
