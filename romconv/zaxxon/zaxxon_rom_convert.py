#!/usr/bin/env python3
# ============================================================
# Zaxxon (Sega 1982) ROM converter for GALAGINO
# Set MAME "zaxxon" (US rev D). Verified: every ROM's SHA1 matches MAME's
# zaxxon.cpp ROM_START(zaxxon) exactly (silkscreen U-numbers in our dump
# also match the board diagram in that driver's header comment).
# ============================================================

import io
import os
import sys
import wave
import zipfile

sys.dont_write_bytecode = True

sys.path.insert(0, os.path.join("..", "internal", "pyconv"))
from gfxutil import load_file, mame_decode, rot_galagino
from asset_emit import emit_header
from convutil import fatal
from adpcm import ima_encode

ROM_SET = os.path.normpath(os.path.join("..", "..", "romszip", "zaxxon.zip"))
AUDIO_SET = os.path.normpath(os.path.join("..", "..", "samples", "zaxxon_samples.zip"))
OUT_DIR = os.path.normpath(os.path.join("..", "..", "source", "src", "machines", "zaxxon"))

# Discrete-sound sample set (Sega "with samples" board, zaxxon_a.cpp /
# zaxxon_sample_names): 12 wav files, named by MAME's sample index. Same
# source as the Battles/Xevious explosion samples (mamewavs), here the full
# board set incl. missiles/laser/alarms/engine noise.
AUDIO_SAMPLES = [
    ("00.wav", "battleship",       "Battleship (end-of-level boss), looped"),
    ("01.wav", "laser",            "Laser / force field, looped"),
    ("02.wav", "missile_base",     "Base missile, one-shot"),
    ("03.wav", "missile_homing",   "Homing missile, looped"),
    ("04.wav", "noise_asteroid",   "Asteroid field background noise, looped"),
    ("05.wav", "noise_intro",      "Initial background noise, looped"),
    ("08.wav", "cannon",           "Ship cannon fire, one-shot"),
    ("10.wav", "explosion_ship",   "Player ship explosion, one-shot"),
    ("11.wav", "explosion_enemy",  "Enemy explosion, one-shot"),
    ("20.wav", "alarm_fuel",       "Low fuel alarm, one-shot"),
    ("21.wav", "alarm_lock",       "Target lock alarm, one-shot"),
    ("23.wav", "shot",             "Enemy shot, one-shot"),
]
AUDIO_SAMPLE_RATE = 24000  # matches Audio's fixed output rate (audio.cpp)

ZAXXON_FILES = {
  "romset": {"name": "zaxxon.zip", "description": "Zaxxon (US rev D)"},

  "maincpu1": {"names": ["zaxxon_rom3d.u27"], "sha1": "80ac53c554c84226b119cbe3cf3470bcdbcd5762"},  # 0000-1fff
  "maincpu2": {"names": ["zaxxon_rom2d.u28"], "sha1": "0cd259be3fa80f3d53dfa76d5ca06773cdfe5945"},  # 2000-3fff
  "maincpu3": {"names": ["zaxxon_rom1d.u29"], "sha1": "2588be06ea7baca6112d58c78a1eeb98aad8a02e"},  # 4000-4fff

  "gfx_tx_1": {"names": ["zaxxon_rom14.u68"], "sha1": "425157a1625b1bd5169c3218b958010bf6af12bb"},
  "gfx_tx_2": {"names": ["zaxxon_rom15.u69"], "sha1": "f1ded2173eb139f48d2ca86c5ef00acbe6c11cd3"},

  "gfx_bg_1": {"names": ["zaxxon_rom6.u113"], "sha1": "a002f3441b0f0044615ce71ecbd14edadba16270"},
  "gfx_bg_2": {"names": ["zaxxon_rom5.u112"], "sha1": "a86543727389931244ba8a576b543d7ac05a2585"},
  "gfx_bg_3": {"names": ["zaxxon_rom4.u111"], "sha1": "a8cd27dfb4a606bae8bfddcf936e69e980fb1977"},

  "gfx_spr_1": {"names": ["zaxxon_rom11.u77"], "sha1": "194e2ca0a806e0cb6bb7cc8341d1fc6f2ea911f6"},
  "gfx_spr_2": {"names": ["zaxxon_rom12.u78"], "sha1": "af6a5984c3cedfa8c9efcd669f4f205b51a433b2"},
  "gfx_spr_3": {"names": ["zaxxon_rom13.u79"], "sha1": "4ac79cccc30e4adfa878b36101e97e20ac010438"},

  "tilemap_1": {"names": ["zaxxon_rom8.u91"],  "sha1": "e1f90716236c61df61bdc6915a8e390cb4dcbf15"},
  "tilemap_2": {"names": ["zaxxon_rom7.u90"],  "sha1": "d26a9049541479b8b19f5aa0690cf4aaa787c9b5"},
  "tilemap_3": {"names": ["zaxxon_rom10.u93"], "sha1": "a0f8c15ff75affa3532abf8f340811cf415421fd"},
  "tilemap_4": {"names": ["zaxxon_rom9.u92"],  "sha1": "2124363be8f590b74e2b15dd3f90d77dd9ca9528"},

  # mro16.u76 in MAME; loaded first in the "proms" region -> the RGB palette.
  "prom_colors": {"names": ["mro16.u76", "zaxxon.u98"], "sha1": "01ae8450ccc302e1a5ae74230d44f6f531a962e2"},
  # zaxxon.u72 in MAME; loaded second -> foreground tile color-group select.
  "prom_fgsel":  {"names": ["zaxxon.u72"], "sha1": "0cf08fb62f77d93ff7cb883c633e0db35906e11d"},
}

def planes3(region_bits):
    return [2*(region_bits//3), 1*(region_bits//3), 0*(region_bits//3)]

def planes2(region_bits):
    return [1*(region_bits//2), 0*(region_bits//2)]

# ------------------------------------------------------------
# Layout: gfx_8x8x2_planar (MAME standard) -- used for gfx_tx (chars)
CHAR_XOFFS = [0,1,2,3,4,5,6,7]
CHAR_YOFFS = [y*8 for y in range(8)]
CHAR_BITS_PER_TILE = 8*8

# Layout: gfx_8x8x3_planar (MAME standard) -- used for gfx_bg (bg tiles)
BG_XOFFS = [0,1,2,3,4,5,6,7]
BG_YOFFS = [y*8 for y in range(8)]
BG_BITS_PER_TILE = 8*8

# Layout: zaxxon_spritelayout (zaxxon.cpp, hand-verified against driver
# source), 32x32 3bpp, RGN_FRAC(1,3) planes. Sprite is a 4x4 grid of 8x8
# blocks; xoffs walks the 4 horizontal blocks, yoffs the 4 vertical ones.
SPR_XOFFS = ([x for x in range(8)] +
             [8*8 + x for x in range(8)] +
             [16*8 + x for x in range(8)] +
             [24*8 + x for x in range(8)])
SPR_YOFFS = ([y*8 for y in range(8)] +
             [32*8 + y*8 for y in range(8)] +
             [64*8 + y*8 for y in range(8)] +
             [96*8 + y*8 for y in range(8)])
SPR_BITS_PER_TILE = 128*8

# ------------------------------------------------------------
def decode_char_tiles(gfx_tx):
    assert len(gfx_tx) == 0x1000
    tx_bits = len(gfx_tx) * 8
    char_count = tx_bits // 2 // CHAR_BITS_PER_TILE
    return [rot_galagino(t) for t in
            mame_decode(gfx_tx, 8, 8, planes2(tx_bits), CHAR_XOFFS, CHAR_YOFFS,
                        CHAR_BITS_PER_TILE, char_count)]

def decode_bg_tiles(gfx_bg):
    assert len(gfx_bg) == 0x6000
    bg_bits = len(gfx_bg) * 8
    bg_count = bg_bits // 3 // BG_BITS_PER_TILE
    return [rot_galagino(t) for t in
            mame_decode(gfx_bg, 8, 8, planes3(bg_bits), BG_XOFFS, BG_YOFFS,
                        BG_BITS_PER_TILE, bg_count)]

def decode_sprite_tiles(gfx_spr):
    assert len(gfx_spr) == 0x6000
    spr_bits = len(gfx_spr) * 8
    spr_count = spr_bits // 3 // SPR_BITS_PER_TILE
    return [rot_galagino(t) for t in
            mame_decode(gfx_spr, 32, 32, planes3(spr_bits), SPR_XOFFS, SPR_YOFFS,
                        SPR_BITS_PER_TILE, spr_count)]

# ------------------------------------------------------------
def load_wav_from_zip(zip_path, wav_name):
    with zipfile.ZipFile(zip_path) as z:
        if wav_name not in z.namelist():
            fatal(f"'{wav_name}' not found in {os.path.abspath(zip_path)}")
        with z.open(wav_name, "r") as entry:
            return io.BytesIO(entry.read())

# Drastically shrinks the source WAVs (44100/22050 Hz, 16 bit) down to the
# 24000 Hz signed-8bit format the other machines' samples already use (see
# xevious_rom_convert.py write_sample_boom): resample to the fixed audio
# output rate (no runtime resampling needed) and requantize to 8 bit,
# peak-normalized per clip.
def resample_to_8bit(wav_file):
    import numpy as np
    w = wave.open(wav_file, "rb")
    nch, sw, fr, nframes = w.getnchannels(), w.getsampwidth(), w.getframerate(), w.getnframes()
    frames = w.readframes(nframes)
    dtype = {1: np.int8, 2: "<i2", 4: "<i4"}[sw]
    data = np.frombuffer(frames, dtype=dtype).astype(np.float64)
    if nch > 1:
        data = data.reshape(-1, nch).mean(axis=1)

    n_out = int(len(data) * AUDIO_SAMPLE_RATE / fr)
    x_old = np.linspace(0, 1, len(data), endpoint=False)
    x_new = np.linspace(0, 1, n_out, endpoint=False)
    resampled = np.interp(x_new, x_old, data)

    peak = np.abs(resampled).max()
    scale = 127.0 / peak if peak > 0 else 1.0
    return np.clip(np.round(resampled * scale), -128, 127).astype(np.int8)

def write_samples(audio_set):
    # PLAIN/flash-resident, not Asset<COMPRESSED>: the existing gfx assets
    # already fill Arena to ~13KB spare (see ARENA BUDGET note in
    # zaxxon_rom_main.h), and up to 12 of these can play concurrently, so
    # heap-decompressing even one (let alone several at once) doesn't fit --
    # same tradeoff as zaxxon_tilemap.h. Encoded as 4-bit IMA-ADPCM (see
    # romconv/pyconv/adpcm.py), decoded inline per-sample by
    # Audio::zaxxon_render_buffer (ima_adpcm_decode in audio.cpp) -- halves
    # flash usage vs raw 8-bit PCM with no heap/RAM cost.
    total_pcm = total_encoded = 0
    for wav_name, suffix, comment in AUDIO_SAMPLES:
        sym = f"zaxxon_sample_{suffix}"
        samples8 = resample_to_8bit(load_wav_from_zip(audio_set, wav_name))
        encoded = ima_encode(samples8)
        flat = [int(b) for b in encoded]
        with open(os.path.join(OUT_DIR, f"{sym}.h"), "w") as f:
            print(f"// {comment} (zaxxon-audio.zip:{wav_name}), resampled at", file=f)
            print(f"// {AUDIO_SAMPLE_RATE} Hz and encoded as 4-bit IMA-ADPCM: a 2-byte", file=f)
            print("// little-endian initial predictor followed by one nibble per", file=f)
            print("// remaining sample. PLAIN/flash-resident: see ARENA BUDGET note in", file=f)
            print("// zaxxon_rom_main.h -- up to 12 of these play concurrently and Arena", file=f)
            print("// has no headroom left for heap decompression; the Asset count", file=f)
            print("// below is the decoded sample count, not the encoded byte count.", file=f)
            print(f"// {len(samples8)} samples ({len(samples8)/AUDIO_SAMPLE_RATE:.3f}s)", file=f)
            emit_header(f, sym, "unsigned char", "[]", len(samples8), _plain_hex_block(flat, per_line=16))
        total_pcm += len(samples8)
        total_encoded += len(encoded)
        print(f"{sym}.h: {len(samples8)} samples ({len(samples8)/AUDIO_SAMPLE_RATE:.3f}s @{AUDIO_SAMPLE_RATE}Hz) "
              f"{len(samples8)} -> {len(encoded)} bytes PLAIN")
    print(f"zaxxon samples total: {total_pcm} -> {total_encoded} bytes "
          f"({100.0 * (1.0 - total_encoded / total_pcm):.1f}% smaller)")

def write_rom(name, sym, data, comment):
  with open(os.path.join(OUT_DIR, name), "w") as f:
    for line in comment.splitlines():
      print(f"// {line}" if line else "//", file=f)
    #emit_compressed(f, sym, "unsigned char", "", len(data), list(data))
    emit_header(f, sym, "unsigned char", "[]", len(data), _plain_hex_block(data, per_line=16))
    

def write_char_tiles(tiles):
    with open(os.path.join(OUT_DIR, "zaxxon_chartiles.h"), "w") as f:
        print("// Zaxxon foreground char set (gfx_tx). 256 tile 8x8 2bpp", file=f)
        print("// (pixel values 0-3). Color group comes from the prom_fgsel", file=f)
        print("// lookup table (zaxxon_fgcolor_codes), not from tile data.", file=f)
        #flat = [v for t in tiles for y in range(8) for v in t[y]]
        rows = []
        for t in tiles:
          trows = []
          for y in range(8):
            trows.append("{" + ",".join(str(v) for v in t[y]) + "}")
          rows.append(" {" + ",".join(trows) + "}")
        flat = ",\n".join(rows)
        emit_header(f, "zaxxon_chartiles", "unsigned char", "[][8][8]", len(tiles), flat)

def write_bg_tiles(tiles):
    with open(os.path.join(OUT_DIR, "zaxxon_bgtiles.h"), "w") as f:
        print("// Zaxxon background tile graphics (gfx_bg). 1024 tile 8x8", file=f)
        print("// 3bpp (pixel values 0-7). Which tile/color goes where on the", file=f)
        print("// 32x512 playfield comes from zaxxon_tilemap.h, not here.", file=f)
        #flat = [v for t in tiles for y in range(8) for v in t[y]]
        rows = []
        for t in tiles:
          trows = []
          for y in range(8):
            trows.append("{" + ",".join(str(v) for v in t[y]) + "}")
          rows.append(" {" + ",".join(trows) + "}")
        flat = ",\n".join(rows)
        emit_header(f, "zaxxon_bgtiles", "unsigned char", "[][8][8]", len(tiles), flat)

def write_sprite_tiles(tiles):
    with open(os.path.join(OUT_DIR, "zaxxon_spritetiles.h"), "w") as f:
        print("// Zaxxon sprites (gfx_spr). 64 sprite 32x32 3bpp (pixel", file=f)
        print("// values 0-7). COMPRESSED: see ARENA BUDGET note in", file=f)
        print("// zaxxon_rom_main.h -- this is one of the two 65536-byte", file=f)
        print("// assets that must each anchor its own Arena block.", file=f)
        #flat = [v for t in tiles for y in range(32) for v in t[y]]
        rows = []
        for t in tiles:
          trows = []
          for y in range(32):
            trows.append("{" + ",".join(str(v) for v in t[y]) + "}")
          rows.append(" {" + ",\n  ".join(trows) + "}")
        flat = ",\n".join(rows)
        emit_header(f, "zaxxon_spritetiles", "unsigned char", "[][32][32]", len(tiles), flat)

def _plain_hex_block(values, per_line=32):
    hexs = ["0x{:02x}".format(v) for v in values]
    rows = [",".join(hexs[i:i + per_line]) for i in range(0, len(hexs), per_line)]
    return ",\n".join(rows)

def write_tilemap(tile_code, tile_color):
    with open(os.path.join(OUT_DIR, "zaxxon_tilemap.h"), "w") as f:
        print("// Zaxxon background playfield map: fixed 32 col x 512 row", file=f)
        print("// tilemap (16384 cells, row-major, TILEMAP_SCAN_ROWS order).", file=f)
        print("// tile_code indexes zaxxon_bgtiles (0-1023); tile_color is", file=f)
        print("// the 4-bit bg color group (0-15).", file=f)
        print("// PLAIN/flash-resident, like 1942's level tilemap: this is", file=f)
        print("// static level-layout data, not artwork, and at 32768+16384", file=f)
        print("// bytes decompressed it's the piece that has to give for the", file=f)
        print("// rest (both 65536-byte tile sets + program ROM + cmaps) to", file=f)
        print("// bin-pack into Arena's two blocks. See ARENA BUDGET note in", file=f)
        print("// zaxxon_rom_main.h.", file=f)
        code_body = _plain_hex_block(tile_code, per_line=16)
        color_body = _plain_hex_block(tile_color, per_line=32)
        emit_header(f, "zaxxon_tilemap_code", "unsigned short", "[]", len(tile_code), code_body)
        emit_header(f, "zaxxon_tilemap_color", "unsigned char", "[]", len(tile_color), color_body)

# ---- Palette PROM (mro16/u76, 256 colors) -> RGB565 byte-swapped ----
# zaxxon_state::zaxxon_palette: R = bit0-2 (1k/470/220), G = bit3-5
# (1k/470/220), B = bit6-7 (470/220). Same resistor network/weights as
# pooyan/rocnrope: compute_resistor_weights(1k/470/220) -> 33,71,151;
# (470/220) -> 81,174.
def convert_palette(prom):
    w3 = [33, 71, 151]
    w2 = [81, 174]
    rgb565 = []
    for v in prom:
        r = min(w3[0]*((v>>0)&1) + w3[1]*((v>>1)&1) + w3[2]*((v>>2)&1), 255)
        g = min(w3[0]*((v>>3)&1) + w3[1]*((v>>4)&1) + w3[2]*((v>>5)&1), 255)
        b = min(w2[0]*((v>>6)&1) + w2[1]*((v>>7)&1), 255)
        val = (((r >> 3) & 0x1F) << 11) | (((g >> 2) & 0x3F) << 5) | ((b >> 3) & 0x1F)
        rgb565.append(((val & 0xFF) << 8) | ((val >> 8) & 0xFF))  # byte-swap for SPI
    return rgb565

def write_palette(rgb565, fgsel):
    with open(os.path.join(OUT_DIR, "zaxxon_palette.h"), "w") as f:
        print("// Zaxxon palette: 256 entries, RGB565 byte-swapped for SPI.", file=f)
        emit_header(f, "zaxxon_palette", "unsigned short", "[]", len(rgb565), _plain_hex_block(rgb565, per_line=16))
        print("", file=f)
        print("// Foreground char color-group select (prom zaxxon.u72, raw).", file=f)
        print("// Indexed [sx + 32*(sy/4)] by fg tile screen column/quadrant;", file=f)
        print("// low nibble * 2 is the palette group for that character.", file=f)
        emit_header(f, "zaxxon_fgcolor_codes", "unsigned char", "[]", len(fgsel), _plain_hex_block(fgsel, per_line=16))

# ------------------------------------------------------------
def convert_zaxxon(romset, files):
  os.makedirs(OUT_DIR, exist_ok=True)

  print(f"Load ROM from: {os.path.abspath(romset)}")
  print(f"Target files:  {os.path.abspath(OUT_DIR)}")

  cpu1 = load_file(romset, files["maincpu1"]["names"], files["maincpu1"]["sha1"])
  cpu2 = load_file(romset, files["maincpu2"]["names"], files["maincpu2"]["sha1"])
  cpu3 = load_file(romset, files["maincpu3"]["names"], files["maincpu3"]["sha1"])

  tx1 = load_file(romset, files["gfx_tx_1"]["names"], files["gfx_tx_1"]["sha1"])
  tx2 = load_file(romset, files["gfx_tx_2"]["names"], files["gfx_tx_2"]["sha1"])

  bg1 = load_file(romset, files["gfx_bg_1"]["names"], files["gfx_bg_1"]["sha1"])
  bg2 = load_file(romset, files["gfx_bg_2"]["names"], files["gfx_bg_2"]["sha1"])
  bg3 = load_file(romset, files["gfx_bg_3"]["names"], files["gfx_bg_3"]["sha1"])

  spr1 = load_file(romset, files["gfx_spr_1"]["names"], files["gfx_spr_1"]["sha1"])
  spr2 = load_file(romset, files["gfx_spr_2"]["names"], files["gfx_spr_2"]["sha1"])
  spr3 = load_file(romset, files["gfx_spr_3"]["names"], files["gfx_spr_3"]["sha1"])

  tm1 = load_file(romset, files["tilemap_1"]["names"], files["tilemap_1"]["sha1"])
  tm2 = load_file(romset, files["tilemap_2"]["names"], files["tilemap_2"]["sha1"])
  tm3 = load_file(romset, files["tilemap_3"]["names"], files["tilemap_3"]["sha1"])
  tm4 = load_file(romset, files["tilemap_4"]["names"], files["tilemap_4"]["sha1"])

  prom_colors = load_file(romset, files["prom_colors"]["names"], files["prom_colors"]["sha1"])
  prom_fgsel  = load_file(romset, files["prom_fgsel"]["names"],  files["prom_fgsel"]["sha1"])

  # ---- program ROM (0000-4fff, not encrypted on this set) ----
  maincpu = cpu1 + cpu2 + cpu3
  assert len(maincpu) == 0x5000
  write_rom("zaxxon_rom_main.h", "zaxxon_rom_main", maincpu,
            "Zaxxon main CPU ROM 0x0000-0x4fff (20KB), not encrypted (US rev D).\n"
            "\n"
            "ARENA BUDGET (see arena.h): every COMPRESSED asset this machine\n"
            "uses stays resident for the machine's whole lifetime (rendering\n"
            "reads all of them every frame, nothing is releasable early), so\n"
            "they all have to bin-pack into Arena's two ~91000-byte blocks\n"
            "at once -- decompressed sizes: rom_main 20480, chartiles 16384,\n"
            "bgtiles 65536, spritetiles 65536, palette 512, fgcolor_codes 256\n"
            "= 168704 total, fits with ~13KB spare. zaxxon_tilemap.h (49152\n"
            "decompressed) is PLAIN instead: it's level-layout data, not\n"
            "artwork, same role as 1942's PLAIN tilemap, and it's the piece\n"
            "that has to give since both 65536-byte tile sets can't share a\n"
            "block with much else.\n"
            "\n"
            "zaxxon::zaxxon() unpacks every COMPRESSED asset eagerly, largest\n"
            "first: Arena fills block A before B, so access order is\n"
            "placement order:\n"
            "  zaxxon_bgtiles      (65536) -> A (A: 65536)\n"
            "  zaxxon_spritetiles  (65536) -> B (B: 65536)\n"
            "  zaxxon_rom_main     (20480) -> A (A: 86016)\n"
            "  zaxxon_chartiles    (16384) -> B (B: 81920)\n"
            "  zaxxon_palette        (512) -> A (A: 86528)\n"
            "  zaxxon_fgcolor_codes  (256) -> A (A: 86784)\n"
            "Any other order risks a block overflow (e.g. rom_main before\n"
            "spritetiles would push spritetiles past A's remaining space and\n"
            "into a B that then can't fit chartiles).")

  # ---- gfx_tx: chars, 2bpp planar 8x8 ----
  char_tiles = decode_char_tiles(tx1 + tx2)
  write_char_tiles(char_tiles)
  print(f"char tiles: {len(char_tiles)}")

  # ---- gfx_bg: background tile graphics, 3bpp planar 8x8 ----
  bg_tiles = decode_bg_tiles(bg1 + bg2 + bg3)
  write_bg_tiles(bg_tiles)
  print(f"bg tiles: {len(bg_tiles)}")

  # ---- gfx_spr: sprites, 32x32 3bpp, zaxxon_spritelayout ----
  sprite_tiles = decode_sprite_tiles(spr1 + spr2 + spr3)
  write_sprite_tiles(sprite_tiles)
  print(f"sprite tiles: {len(sprite_tiles)}")

  # ---- tilemap_dat: fixed 32x512 bg playfield map, not gfx data ----
  # zaxxon_state::get_bg_tile_info: source split in half; low byte = tile
  # code, high half byte = (code_hi<<8 | color<<4). size = region/2.
  tilemap_dat = tm1 + tm2 + tm3 + tm4
  assert len(tilemap_dat) == 0x8000
  size = len(tilemap_dat) // 2
  assert size == 32 * 512
  tile_code  = [tilemap_dat[i] + 256 * (tilemap_dat[i + size] & 3) for i in range(size)]
  tile_color = [tilemap_dat[i + size] >> 4 for i in range(size)]
  assert max(tile_code) < len(bg_tiles)
  write_tilemap(tile_code, tile_color)
  print(f"tilemap: {size} cells (32x512)")

  # ---- palette ----
  assert len(prom_colors) == 256 and len(prom_fgsel) == 256
  rgb565 = convert_palette(prom_colors)
  write_palette(rgb565, prom_fgsel)
  print("palette: 256 colors + 256 fg color-select entries")

  print("Zaxxon (ROM/gfx) conversion done.")

def main():
  if not os.path.isfile(ROM_SET):
    print(f"ERROR: No roms. Expected {os.path.abspath(ROM_SET)}")
    sys.exit(1)
  if not os.path.isfile(AUDIO_SET):
    print(f"ERROR: No audio samples. Expected {os.path.abspath(AUDIO_SET)}")
    sys.exit(1)

  convert_zaxxon(ROM_SET, ZAXXON_FILES)

  print(f"Load audio samples from: {os.path.abspath(AUDIO_SET)}")
  write_samples(AUDIO_SET)
  print("Zaxxon (audio) conversion done.")

if __name__ == "__main__":
    main()
