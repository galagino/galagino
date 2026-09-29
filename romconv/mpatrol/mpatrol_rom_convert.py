#!/usr/bin/env python3
# ============================================================
# mpatrol_rom_convert.py
#
# ROM converter for the Galagino Plus `mpatrol` machine = MAME `mpatrol`,
# "Moon Patrol" (Irem 1982). Driver: src/mame/irem/m52.cpp (ROM_START(mpatrol),
# gfx layouts, m52_state::init_palette / init_sprite_palette), sound board
# src/mame/irem/irem.cpp (m52_soundc_audio_device).
# Everything below is taken from those MAME files - check them, not these
# comments, if anything looks wrong.
#
# USAGE
#     python mpatrol_rom_convert.py
#
# OUTPUT
#     mpatrol_rom.h      main CPU 0000-3fff, sound CPU 7000-7fff
#     mpatrol_gfx.h      tx chars [512][8][8], sprites [128][16][16],
#                        backgrounds [3][64][256] (pens)
#     mpatrol_palette.h  RGB565 (byte-swapped) palettes + the RGB888 values
#                        (the harness writes snapshots with those)
# ============================================================

import os
import sys
import zipfile
import zlib
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROM_SET = os.path.normpath(os.path.join("..", "..", "romszip", "mpatrol.zip"))
OUT_DIR = os.path.normpath(os.path.join("..", "..", "source", "src", "machines", "mpatrol"))

# ROM_START(mpatrol): name -> (region, offset, size, crc)
ROMS = {
    "mpa-1.3m":     ("maincpu",   0x0000, 0x1000, 0x5873a860),
    "mpa-2.3l":     ("maincpu",   0x1000, 0x1000, 0xf4b85974),
    "mpa-3.3k":     ("maincpu",   0x2000, 0x1000, 0x2e1a598c),
    "mpa-4.3j":     ("maincpu",   0x3000, 0x1000, 0xdd05b587),
    "mp-s1.1a":     ("iremsound", 0x7000, 0x1000, 0x561d3108),
    "mpe-4.3f":     ("tx",        0x0000, 0x1000, 0xcca6d023),
    "mpe-5.3e":     ("tx",        0x1000, 0x1000, 0xe3ee7f75),
    "mpb-2.3m":     ("sp",        0x0000,  0x1000, 0x707ace5e),
    "mpb-1.3n":     ("sp",        0x1000,  0x1000, 0x9b72133a),
    "mpe-1.3l":     ("bg0",       0x0000, 0x1000, 0xc46a7f72),
    "mpe-2.3k":     ("bg1",       0x0000, 0x1000, 0xc7aa1fb0),
    "mpe-3.3h":     ("bg2",       0x0000, 0x1000, 0xa0919392),
    "mpc-4.2a":     ("tx_pal",    0x0000, 0x0200, 0x07f99284),
    "mpc-3.1m":     ("bg_pal",    0x0000, 0x0020, 0x6a57eff2),
    "mpc-1.1f":     ("spr_pal",   0x0000, 0x0020, 0x26979b13),
    "mpc-2.2h":     ("spr_clut",  0x0000, 0x0100, 0x7ae4cd97),
    "mp_7621-5.7h": ("unkprom",   0x0000, 0x0200, 0xcf1fd9d0),
}

# ROM_REGION sizes and fill (ROMREGION_ERASE00 / ERASEFF; plain regions are 0)
REGIONS = {
    "maincpu":   (0x10000, 0x00), 
    "iremsound": (0x8000, 0x00), 
    "tx":        (0x2000, 0x00),
    "sp":        (0x3000, 0x00), 
    "bg0":       (0x2000, 0xff), 
    "bg1":       (0x2000, 0xff), 
    "bg2":       (0x2000, 0xff),
    "tx_pal":    (0x200, 0x00), 
    "bg_pal":    (0x20, 0x00), 
    "spr_pal":   (0x20, 0x00),
    "spr_clut":  (0x100, 0x00), 
    "unkprom":   (0x200, 0x00),
}


def load_regions(src: Path) -> dict:
    files = {}
    with zipfile.ZipFile(src) as z:
        for info in z.infolist():
            base = info.filename.split("/")[-1]
            if base in ROMS:
                files[base] = z.read(info)
    regions = {k: bytearray([fill]) * size for k, (size, fill) in REGIONS.items()}
    for name, (region, offs, size, crc) in ROMS.items():
        if name not in files:
            sys.exit(f"missing ROM {name}")
        data = files[name]
        if len(data) != size:
            sys.exit(f"{name}: size {len(data)}, expected {size}")
        got = zlib.crc32(data) & 0xffffffff
        if got != crc:
            msg = f"{name}: CRC {got:08x}, MAME set has {crc:08x}"
            if verify:
                sys.exit(msg)
            print("WARNING " + msg)
        regions[region][offs:offs + size] = data
    return regions


def readbit(region: bytes, bit: int) -> int:
    # MAME gfx decode: bit offset 0 is the MSB of byte 0
    return (region[bit >> 3] >> (7 - (bit & 7))) & 1


def decode(region, planes, xoffs, yoffs, w, h, charincrement, count):
    out = []
    for c in range(count):
        base = c * charincrement
        img = []
        for y in range(h):
            row = []
            for x in range(w):
                pen = 0
                for p in planes:          # first plane = most significant bit
                    pen = (pen << 1) | readbit(region, base + p + yoffs[y] + xoffs[x])
                row.append(pen)
            img.append(row)
        out.append(img)
    return out


def step(start, inc, n):
    return [start + i * inc for i in range(n)]


def build_tx(regions):
    # gfx_8x8x2_planar (emu/video/generic.cpp): RGN_FRAC(1,2) chars,
    # planes { RGN_FRAC(1,2), RGN_FRAC(0,2) }, x STEP8(0,1), y STEP8(0,8), 8*8
    r = regions["tx"]
    frac = len(r) * 8 // 2
    return decode(r, [frac, 0], step(0, 1, 8), step(0, 8, 8), 8, 8, 64, frac // 64)


def build_sprites(regions):
    # spritelayout: 16x16, RGN_FRAC(1,3), 3 planes { 2/3, 0/3, 1/3 },
    # x { STEP8(0,1), STEP8(16*8,1) }, y STEP16(0,8), 32*8
    r = regions["sp"]
    frac = len(r) * 8 // 3
    xo = step(0, 1, 8) + step(16 * 8, 1, 8)
    return decode(r, [2 * frac, 0, frac], xo, step(0, 8, 16), 16, 16, 32 * 8, frac // 256)


def build_bg(regions, name):
    # bgcharlayout: 256x128, 1 image, 2bpp planes { 4, 0 }
    # xoffset STEP4(0x000,1), STEP4(0x008,1) ... ; yoffset STEP32(0x0000,0x200),
    # STEP32(0x4000,0x200), STEP32(0x8000,0x200), STEP32(0xc000,0x200)
    xo = []
    for g in range(64):
        xo += step(g * 8, 1, 4)
    yo = step(0x0000, 0x200, 32) + step(0x4000, 0x200, 32) + step(0x8000, 0x200, 32) + step(0xc000, 0x200, 32)
    img = decode(regions[name], [4, 0], xo, yo, 256, 128, 0, 1)[0]
    # rows 64-127 come from the 0xff fill (ROMREGION_ERASEFF): all pen 3.
    # The header only stores rows 0-63; the renderer uses pen 3 below them.
    for y in range(64, 128):
        assert all(p == 3 for p in img[y]), f"{name} row {y} is not all pen 3"
    return img[:64]


# ---- emu/video/resnet.cpp compute_resistor_weights() / combine_weights() ----
def compute_resistor_weights(minval, maxval, scaler, nets):
    # nets: list of (resistances, pulldown, pullup); returns (scale, [weights])
    w = []
    for res, pd, pu in nets:
        ww = []
        for n in range(len(res)):
            R0 = 1.0 / 1e12 if pd == 0 else 1.0 / pd
            R1 = 1.0 / 1e12 if pu == 0 else 1.0 / pu
            for j in range(len(res)):
                if j == n:
                    if res[j] != 0.0:
                        R1 += 1.0 / res[j]
                else:
                    if res[j] != 0.0:
                        R0 += 1.0 / res[j]
            R0 = 1.0 / R0
            R1 = 1.0 / R1
            vout = (maxval - minval) * R0 / (R1 + R0) + minval
            ww.append(min(max(vout, float(minval)), float(maxval)))
        w.append(ww)
    maxv, jmax = 0.0, 0
    max_out = []
    for i, ww in enumerate(w):
        s = 0.0
        for v in ww:
            s += v
        max_out.append(s)
        if maxv < s:
            maxv, jmax = s, i
    scale = float(maxval) / max_out[jmax] if scaler < 0.0 else scaler
    return scale, [[v * scale for v in ww] for ww in w]


def combine_weights(tab, *bits):
    # T(sum(tab[i] * w_i) + 0.5), summed in the same order as the template
    acc = None
    for i in reversed(range(len(bits))):
        term = tab[i] * bits[i]
        acc = term if acc is None else term + acc
    return int(acc + 0.5)


def bit(v, n):
    return (v >> n) & 1


def build_palettes(regions):
    res3 = [1000, 470, 220]
    res2 = [470, 220]
    # init_palette(): characters / backgrounds
    scale, (wr, wg, wb) = compute_resistor_weights(0, 255, -1.0,
        [(res3, 0, 0), (res3, 0, 0), (res2, 0, 0)])

    def rgb_cb(v):
        return (combine_weights(wr, bit(v, 0), bit(v, 1), bit(v, 2)),
                combine_weights(wg, bit(v, 3), bit(v, 4), bit(v, 5)),
                combine_weights(wb, bit(v, 6), bit(v, 7)))

    tx = [rgb_cb(v) for v in regions["tx_pal"]]
    bg_colors = [rgb_cb(v) for v in regions["bg_pal"]]
    # set_pen_indirect() table for the 3 background images x 4 pens
    bg_indirect = [0, 4, 8, 12, 0, 1, 2, 3, 0, 16 + 1, 16 + 2, 16 + 3]
    bg = [bg_colors[i] for i in bg_indirect]

    # init_sprite_palette(): scaler = the one returned above, pulldowns 470
    _, (swr, swg, swb) = compute_resistor_weights(0, 255, scale,
        [(res2, 470, 0), (res3, 470, 0), (res3, 470, 0)])
    sp_colors = []
    for v in regions["spr_pal"]:
        sp_colors.append((combine_weights(swr, bit(v, 6), bit(v, 7)),
                          combine_weights(swg, bit(v, 3), bit(v, 4), bit(v, 5)),
                          combine_weights(swb, bit(v, 0), bit(v, 1), bit(v, 2))))
    clut = list(regions["spr_clut"])
    assert all(c < 32 for c in clut), "sprite clut entry outside the 32 indirect colours"
    # PALETTE(config, m_sp_palette).set_entries(256, 32): pen i -> clut[i].
    # gfx(0) has 16 colours of granularity 8, so only pens 0-127 are reached
    # (drawgfx: colorbase + granularity * (color % colors())).
    sp = [sp_colors[clut[i]] for i in range(256)]
    return tx, bg, sp, clut


def rgb565_swapped(r, g, b):
    c = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)
    return ((c >> 8) | (c << 8)) & 0xFFFF


def c_array(f, ctype, name, data, per_line=16, fmt="0x{:02x}"):
    f.write(f"{ctype} {name}[{len(data)}] = {{\n")
    for i in range(0, len(data), per_line):
        f.write("  " + ",".join(fmt.format(v) for v in data[i:i + per_line]) + ",\n")
    f.write("};\n\n")


def write_rom(outdir, regions):
    with open(outdir / "mpatrol_rom.h", "w", newline="\n") as f:
        f.write("#ifndef MPATROL_ROM_H\n#define MPATROL_ROM_H\n\n")
        f.write("// Generated by mpatrol_rom_convert.py - do not edit.\n")
        f.write("// mpatrol_rom: maincpu 0000-3fff (mpa-1.3m, mpa-2.3l, mpa-3.3k, mpa-4.3j)\n")
        f.write("// mpatrol_snd: irem_audio:iremsound 7000-7fff (mp-s1.1a); 2000-6fff is\n")
        f.write("//              an empty part of the region (reads 0x00)\n\n")
        c_array(f, "const unsigned char", "mpatrol_rom", regions["maincpu"][:0x4000])
        c_array(f, "const unsigned char", "mpatrol_snd", regions["iremsound"][0x7000:0x8000])
        f.write("#endif\n")


def write_gfx(outdir, tx, sp, bgs):
    with open(outdir / "mpatrol_gfx.h", "w", newline="\n") as f:
        f.write("#ifndef MPATROL_GFX_H\n#define MPATROL_GFX_H\n\n")
        f.write("// Generated by mpatrol_rom_convert.py - do not edit. One byte per pixel = pen.\n")
        f.write("// mpatrol_tx  [code][y][x]  gfx_8x8x2_planar, 512 chars\n")
        f.write("// mpatrol_spr [code][y][x]  spritelayout, 128 x 16x16 (pens 0-3: plane 2 is\n")
        f.write("//                           the 0x00 fill of region \"sp\")\n")
        f.write("// mpatrol_bg  [image][y][x] bgcharlayout rows 0-63; rows 64-127 are all pen 3\n\n")
        f.write(f"const unsigned char mpatrol_tx[{len(tx)}][8][8] = {{\n")
        for c in tx:
            f.write("  {" + ",".join("{" + ",".join(str(p) for p in row) + "}" for row in c) + "},\n")
        f.write("};\n\n")
        f.write(f"const unsigned char mpatrol_spr[{len(sp)}][16][16] = {{\n")
        for c in sp:
            f.write("  {" + ",".join("{" + ",".join(str(p) for p in row) + "}" for row in c) + "},\n")
        f.write("};\n\n")
        f.write("const unsigned char mpatrol_bg[3][64][256] = {\n")
        for img in bgs:
            f.write("  {\n")
            for row in img:
                f.write("   {" + ",".join(str(p) for p in row) + "},\n")
            f.write("  },\n")
        f.write("};\n\n#endif\n")


def write_palette(outdir, tx, bg, sp, clut):
    with open(outdir / "mpatrol_palette.h", "w", newline="\n") as f:
        f.write("#ifndef MPATROL_PALETTE_H\n#define MPATROL_PALETTE_H\n\n")
        f.write("// Generated by mpatrol_rom_convert.py - do not edit.\n")
        f.write("// m52_state::init_palette() / init_sprite_palette(): resistor weights\n")
        f.write("// (resnet.cpp compute_resistor_weights) of the colour PROMs.\n")
        f.write("// *_565: RGB565 byte-swapped for the display DMA. *_888: 0xRRGGBB.\n")
        f.write("// tx: colour * 4 + pen (512). bg: image * 4 + pen (12).\n")
        f.write("// sp: colour * 8 + pen (256, via the spr_clut PROM). mpatrol_sp_clut: the PROM\n")
        f.write("// itself - a sprite pen whose entry is 0 is transparent (transpen_mask).\n\n")
        c_array(f, "const unsigned short", "mpatrol_tx_565", [rgb565_swapped(*c) for c in tx], 8, "0x{:04x}")
        c_array(f, "const unsigned short", "mpatrol_bg_565", [rgb565_swapped(*c) for c in bg], 8, "0x{:04x}")
        c_array(f, "const unsigned short", "mpatrol_sp_565", [rgb565_swapped(*c) for c in sp], 8, "0x{:04x}")
        c_array(f, "const unsigned char", "mpatrol_sp_clut", clut)
        f.write("#ifdef MPATROL_PALETTE_888\n")
        c_array(f, "const unsigned int", "mpatrol_tx_888", [(r << 16) | (g << 8) | b for r, g, b in tx], 8, "0x{:06x}")
        c_array(f, "const unsigned int", "mpatrol_bg_888", [(r << 16) | (g << 8) | b for r, g, b in bg], 8, "0x{:06x}")
        c_array(f, "const unsigned int", "mpatrol_sp_888", [(r << 16) | (g << 8) | b for r, g, b in sp], 8, "0x{:06x}")
        f.write("#endif\n\n#endif\n")


def preview(outpng, tx, sp, bgs, txpal, bgpal, sppal):
    try:
        from PIL import Image
    except ImportError:
        print("PIL not installed - no preview")
        return
    im = Image.new("RGB", (512, 448))
    # tx chars with colour 0x10, 32 per row
    for c, img in enumerate(tx):
        ox, oy = (c % 32) * 8, (c // 32) * 8
        for y in range(8):
            for x in range(8):
                im.putpixel((ox + x, oy + y), txpal[0x10 * 4 + img[y][x]])
    # sprites colour 1, 16 per row, at y 128
    for c, img in enumerate(sp):
        ox, oy = 256 + (c % 16) * 16, (c // 16) * 16
        for y in range(16):
            for x in range(16):
                im.putpixel((ox + x, oy + y), sppal[1 * 8 + img[y][x]])
    for i, img in enumerate(bgs):
        for y in range(64):
            for x in range(256):
                im.putpixel((x, 256 + i * 64 + y), bgpal[i * 4 + img[y][x]])
    im.save(outpng)
    print("preview:", outpng)


def main():

    outdir = Path(OUT_DIR)
    regions = load_regions(Path(ROM_SET))
    tx = build_tx(regions)
    sp = build_sprites(regions)
    bgs = [build_bg(regions, n) for n in ("bg0", "bg1", "bg2")]
    txpal, bgpal, sppal, clut = build_palettes(regions)
    assert len(tx) == 512 and len(sp) == 128
    assert all(p < 4 for c in sp for row in c for p in row), "sprite plane 2 not empty"
    write_rom(outdir, regions)
    write_gfx(outdir, tx, sp, bgs)
    write_palette(outdir, txpal, bgpal, sppal, clut)
    print(f"wrote mpatrol_rom.h, mpatrol_gfx.h, mpatrol_palette.h to {outdir}")
    #preview(HERE.parents[2] / "debug" / "mpatrol_gfx_preview.png", tx, sp, bgs, txpal, bgpal, sppal)


if __name__ == "__main__":
    main()
