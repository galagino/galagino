#!/usr/bin/env python3
"""
Phoenix ROM converter for GALAGINO.

ROM set: phoenix.zip -> Phoenix (Amstar, set 1) - MAME 0287
"""

import os
import sys
import zipfile
import hashlib

sys.dont_write_bytecode = True
from helper_functions import load_file

ROM_SET = os.path.normpath(os.path.join("..", "..", "romszip", "phoenix.zip"))
OUT_DIR = os.path.normpath(os.path.join("..", "..", "source", "src", "machines", "phoenix"))

def emit_byte_array(name, data, comment=""):
    lines = [
        "// Auto-generated - DO NOT EDIT",
        f"// {comment}" if comment else "",
        f"const unsigned char {name}[{len(data)}] = {{",
    ]
    for i in range(0, len(data), 16):
        chunk = data[i:i + 16]
        lines.append(" " + ", ".join(f"0x{b:02x}" for b in chunk) +
                    ("," if i + 16 < len(data) else ""))
    lines.append("};")
    lines.append("")
    return "\n".join(l for l in lines if l is not None)

def emit_word_array(name, words, comment=""):
  lines = [
    "// Auto-generated - DO NOT EDIT",
    f"// {comment}" if comment else "",
    f"const unsigned short {name}[{len(words)}] = {{",
  ]
  for i in range(0, len(words), 12):
    chunk = words[i:i + 12]
    lines.append(" " + ", ".join(f"0x{w:04x}" for w in chunk) +
                 ("," if i + 12 < len(words) else ""))
  lines.append("};")
  lines.append("")
  return "\n".join(l for l in lines if l is not None)

def to_rgb565_swap(r, g, b):
  rgb565 = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)
  return ((rgb565 >> 8) | (rgb565 << 8)) & 0xFFFF

def decode_palette(prom_lo, prom_hi):
    LEVELS = [0, 0, 202, 255]
    palette = []

    for i in range(256):
        b6 = (i >> 6) & 1
        b5 = (i >> 5) & 1
        b1 = (i >> 1) & 1
        b0 = (i >> 0) & 1
        b4 = (i >> 4) & 1
        b3 = (i >> 3) & 1
        b2 = (i >> 2) & 1

        col = (b6 << 6) | (b5 << 5) | (b1 << 4) | (b0 << 3) | (b4 << 2) | (b3 << 1) | b2

        lo = prom_lo[col]
        hi = prom_hi[col]

        r_bits = ((lo >> 0) & 1) | (((hi >> 0) & 1) << 1)
        g_bits = ((lo >> 2) & 1) | (((hi >> 2) & 1) << 1)
        b_bits = ((lo >> 1) & 1) | (((hi >> 1) & 1) << 1)

        r = LEVELS[r_bits]
        g = LEVELS[g_bits]
        b = LEVELS[b_bits]

        palette.append(to_rgb565_swap(r, g, b))

    return palette

def decode_tile_pens(gfx_p0, gfx_p1):
  out = bytearray(0x4000)
  for code in range(0, 256):
    for py in range(0, 8):
      p0 = gfx_p0[code * 8 + py];
      p1 = gfx_p1[code * 8 + py];
      for px in range(0, 9):
        pen = ((p0 >> px) & 1) | (((p1 >> px) & 1) << 1)
        out[(code << 6) | (py << 3) | px] = pen;
  return out

def main():
  print(f"Load ROM from: {os.path.abspath(ROM_SET)}")
  print(f"Target files:  {os.path.abspath(OUT_DIR)}")

  if not os.path.isfile(ROM_SET):
    print(f"ERROR: missing {ROM_SET}", file=sys.stderr)
    sys.exit(1)
    
  os.makedirs(OUT_DIR, exist_ok=True)

  cpu_rom = bytearray(0x4000)
  rom45 = load_file(ROM_SET, ["ic45"],       "fc3cef299bf03bf0586c4047c6b96ca666846220")
  rom46 = load_file(ROM_SET, ["ic46"],       "6f3019a074e73ff50ceb92f655fcf15659f34919")
  rom47 = load_file(ROM_SET, ["ic47"],       "6e69f8f0d537fe89140cd95d2398531d7e93d102")
  rom48 = load_file(ROM_SET, ["ic48"],       "49bcf55a5721cfcc02c3b811a4b601e35ea576db")
  rom49 = load_file(ROM_SET, ["h5-ic49.5a"], "b35142a91b6b7fdf7535202671793393c9f4685f")
  rom50 = load_file(ROM_SET, ["h6-ic50.6a"], "0402e5241d99759d804291998efd43f37ce99917")
  rom51 = load_file(ROM_SET, ["h7-ic51.7a"], "849bf8273317cc869bdd67e50c68399ee8ece81d")
  rom52 = load_file(ROM_SET, ["h8-ic52.8a"], "e4164f85ec12d4d9bcbffba27ab1f51b3599f6d0")

  cpu_rom[0x0000:0x0800] = rom45
  cpu_rom[0x0800:0x1000] = rom46
  cpu_rom[0x1000:0x1800] = rom47
  cpu_rom[0x1800:0x2000] = rom48
  cpu_rom[0x2000:0x2800] = rom49
  cpu_rom[0x2800:0x3000] = rom50
  cpu_rom[0x3000:0x3800] = rom51
  cpu_rom[0x3800:0x4000] = rom52

  # background_tiles
  bg_p0 = load_file(ROM_SET, ["ic23.3d"], "e7ff5fc371664af44785c079e92eeb2d8530187b")
  bg_p1 = load_file(ROM_SET, ["ic24.4d"], "71aec70a8e096ed1f0c2297b3ae7dca1b8ecc38d")
  bg_data = bg_p0 + bg_p1

  # foreground_tiles
  fg_p0 = load_file(ROM_SET, ["b1-ic39.3b"], "d772358505b973b10da840d204afb210c0c746ec")
  fg_p1 = load_file(ROM_SET, ["b2-ic40.4b"], "af9243ee23377b632b9b7d0b84d341d06bf22480")
  fg_data = fg_p0 + fg_p1

  # palette prom
  prom_lo = load_file(ROM_SET, ["mmi6301.ic40"], "57411be4c1d89677f7919ae295446da90612c8a8")
  prom_hi = load_file(ROM_SET, ["mmi6301.ic41"], "e2184dd495ed579f10b6da0b78379e02d7a6229f")
  palette_rgb = decode_palette(prom_lo, prom_hi)

  bg_decoded = decode_tile_pens(bg_p0, bg_p1)
  fg_decoded = decode_tile_pens(fg_p0, fg_p1)

  with open(os.path.join(OUT_DIR, "phoenix_rom.h"), "w", encoding="utf-8", newline="\n") as f:
    f.write(emit_byte_array("phoenix_rom", cpu_rom, "Z80 CPU ROM 16 KB (ic45..ic52)"))
  print(f"[OK] phoenix_rom.h ({len(cpu_rom)} bytes)")

  with open(os.path.join(OUT_DIR, "phoenix_bgtiles.h"), "w", encoding="utf-8", newline="\n") as f:
    #f.write(emit_byte_array("phoenix_bgtiles",      bg_data, "BG tiles 4 KB (256 char × 8x8 × 2bpp), plane0 + plane1"))
    f.write(emit_byte_array("phoenix_bgtiles", bg_decoded, "BG tiles decoded"))
  print(f"[OK] phoenix_bgtiles.h ({len(bg_decoded)} bytes)")

  with open(os.path.join(OUT_DIR, "phoenix_fgtiles.h"), "w", encoding="utf-8", newline="\n") as f:
    #f.write(emit_byte_array("phoenix_fgtiles",      fg_data,    "FG tiles 4 KB (256 char × 8x8 × 2bpp), plane0 + plane1"))
    f.write(emit_byte_array("phoenix_fgtiles", fg_decoded, "FG tiles decoded"))
  print(f"[OK] phoenix_fgtiles.h ({len(fg_decoded)} bytes)")

  with open(os.path.join(OUT_DIR, "phoenix_palette.h"), "w", encoding="utf-8", newline="\n") as f:
    f.write(emit_word_array("phoenix_palette", palette_rgb, "256 colori RGB565 byte-swapped (mmi6301 ic40 + ic41)"))
  print(f"[OK] phoenix_palette.h (256 colors)")

  print("\n[DONE] files in ", os.path.abspath(OUT_DIR))

if __name__ == "__main__":
    main()
