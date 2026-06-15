#!/usr/bin/env python3
"""
Pengo ROM converter for GALAGINO

instead of the original pengo2u.zip (from old MAME versions)
this will work for: 
  pengo.zip  - Pengo (World, not encrypted, rev A)
  pengoj.zip - Pengo (Japan, not encrypted)

pengoj is the one that has the popcorn music
"""

import os
import sys
import zipfile
import hashlib

from helper_functions import hex8, hex16, hex32
from helper_functions import load_file
from helper_functions import get_bit, rgb888_to_rgb565_le
from helper_gfx       import tiles_create_preview, sprites_create_preview
from helper_gfx       import rotate_gfx
from helper_gfx       import decode_tile_8x8, dump_tile_packed
from helper_gfx       import decode_sprite_16x16_from_mame, flip_sprite, dump_sprite_packed

ROM_SET_W  = os.path.normpath(os.path.join("..", "..", "romszip", "pengo.zip"))
ROM_SET_J  = os.path.normpath(os.path.join("..", "..", "romszip", "pengoj.zip"))
ROM_SET_2u = os.path.normpath(os.path.join("..", "..", "romszip", "pengo2u.zip"))
OUT_DIR    = os.path.normpath(os.path.join("..", "..", "source", "src", "machines", "pengo"))

PENGO2u_FILES = {
  "romset": {"name": "pengo2u.zip", "description": "Pengo (set 2 not encrypted)"},
  "cpu0":   {"names": ["epr5128.u8",  "pengo.u8"],   "sha1": "a387b72501da77bf38b58619d2099083a0463e1f"},
  "cpu1":   {"names": ["epr5129.u7",  "pengo.u7"],   "sha1": "d1c66bb9cf479e6960dbcd35c820097a81eaa555"},
  "cpu2":   {"names": ["epr5130.u15", "pengo.u15"],  "sha1": "a8a568da68babd0ccb9f2cee4182fc01c3138494"},
  "cpu3":   {"names": ["epr5131.u14", "pengo.u14"],  "sha1": "2ee821b0f6e0f3cfeae7f5ff25a6e9bd977efce0"},
  "cpu4":   {"names": ["ep5124.21",   "ep5124.21"],  "sha1": "fdebc68a6d87f8ecdf52a57a34ae5ae844a13510"},
  "cpu5":   {"names": ["epr5133.u20", "pengo.u20"],  "sha1": "ed814d58318c1055e475ff678609d189727bf9b4"},
  "cpu6":   {"names": ["ep5126.32",   "ep5126.32"],  "sha1": "0ac5ffdad7bdcb32e630b9582e1b1aaece5198c9"},
  "cpu7":   {"names": ["epr5135.u31", "pengo.u31"],  "sha1": "332b484d47c9921ed93432755bb2d7a9d4628939"},
  "gfx1":   {"names": ["ep1640.92",   "ep1640.92"],  "sha1": "e542bcc28f292be9a0a29d949de726e0b55e654a"},
  "gfx2":   {"names": ["ep1695.105",  "ep1695.105"], "sha1": "bdec535e486b43a8f5550334beff423eeace10b2"},
  "prom1":  {"names": ["pr1633.78",   "pr1633.78"],  "sha1": "680eab0e1204c9b74adc11588461651b474021bb"},
  "prom2":  {"names": ["pr1634.88",   "pr1634.88"],  "sha1": "3fcd66610fcaee814953a115bf5e04788923181f"},
  "sound1": {"names": ["pr1635.51",   "pr1635.51"],  "sha1": "563c9770028fe39188e62630711589d6ed242a66"},
  "sound2": {"names": ["pr1636.70",   "pr1636.70"],  "sha1": "0c4d0bee858b97632411c440bea6948a74759746"},
}

PENGOW_FILES = {
  "romset": {"name": "pengo.zip", "description": "Pengo (World, not encrypted, rev A)"},
  "cpu0":   {"names": ["epr-5128.ic8"],    "sha1": "a387b72501da77bf38b58619d2099083a0463e1f"},
  "cpu1":   {"names": ["epr-5129.ic7"],    "sha1": "d1c66bb9cf479e6960dbcd35c820097a81eaa555"},
  "cpu2":   {"names": ["epr-5130.ic15"],   "sha1": "a8a568da68babd0ccb9f2cee4182fc01c3138494"},
  "cpu3":   {"names": ["epr-5131a.ic14"],  "sha1": "2ee821b0f6e0f3cfeae7f5ff25a6e9bd977efce0"},
  "cpu4":   {"names": ["epr-5132.ic21"],   "sha1": "fdebc68a6d87f8ecdf52a57a34ae5ae844a13510"},
  "cpu5":   {"names": ["epr-5133.ic20"],   "sha1": "ed814d58318c1055e475ff678609d189727bf9b4"},
  "cpu6":   {"names": ["epr-5134.ic32"],   "sha1": "0ac5ffdad7bdcb32e630b9582e1b1aaece5198c9"},
  "cpu7":   {"names": ["epr-5135a.ic31"],  "sha1": "332b484d47c9921ed93432755bb2d7a9d4628939"},
  "gfx1":   {"names": ["epr-1640.ic92"],   "sha1": "e542bcc28f292be9a0a29d949de726e0b55e654a"},
  "gfx2":   {"names": ["epr-1695.ic105"],  "sha1": "bdec535e486b43a8f5550334beff423eeace10b2"},
  "prom1":  {"names": ["pr1633.ic78"],     "sha1": "680eab0e1204c9b74adc11588461651b474021bb"},
  "prom2":  {"names": ["pr1634.ic88"],     "sha1": "3fcd66610fcaee814953a115bf5e04788923181f"},
  "sound1": {"names": ["pr1635.ic51"],     "sha1": "563c9770028fe39188e62630711589d6ed242a66"},
  "sound2": {"names": ["pr1636.ic70"],     "sha1": "0c4d0bee858b97632411c440bea6948a74759746"},
}

PENGOJ_FILES = {
  "romset": {"name": "pengoj.zip", "description": "Pengo (Japan, not encrypted)"},
  "cpu0":   {"names": ["epr-5120.ic8"],   "sha1": "1db732a17a9f79f8f1751f80c77889142928e41b"},
  "cpu1":   {"names": ["epr-5121.ic7"],   "sha1": "d351347f93a3ed01c8b5274ec19352dd611a8dd4"},
  "cpu2":   {"names": ["epr-5122.ic15"],  "sha1": "7b9617d22a9de8d3658abe34b5d2171ce37acc39"}, 
  "cpu3":   {"names": ["epr-5123.ic14"],  "sha1": "444acc08607c892bb20b3a02753169addf5b11de"},
  "cpu4":   {"names": ["epr-5124.ic21"],  "sha1": "fdebc68a6d87f8ecdf52a57a34ae5ae844a13510"},
  "cpu5":   {"names": ["epr-5125.ic20"],  "sha1": "fef20385299a709ee17ed16510ac5702bd5cc257"},
  "cpu6":   {"names": ["epr-5126.ic32"],  "sha1": "0ac5ffdad7bdcb32e630b9582e1b1aaece5198c9"},
  "cpu7":   {"names": ["epr-5127.ic31"],  "sha1": "20e4353208c3803d8879b25f821ea617e9a19cc4"},
  "gfx1":   {"names": ["epr-1640.ic92"],  "sha1": "e542bcc28f292be9a0a29d949de726e0b55e654a"},
  "gfx2":   {"names": ["epr-1695.ic105"], "sha1": "bdec535e486b43a8f5550334beff423eeace10b2"},
  "prom1":  {"names": ["pr1633.ic78"],    "sha1": "680eab0e1204c9b74adc11588461651b474021bb"},
  "prom2":  {"names": ["pr1634.ic88"],    "sha1": "3fcd66610fcaee814953a115bf5e04788923181f"},
  "sound1": {"names": ["pr1635.ic51"],    "sha1": "563c9770028fe39188e62630711589d6ed242a66"},
  "sound2": {"names": ["pr1636.ic70"],    "sha1": "0c4d0bee858b97632411c440bea6948a74759746"},
}

GALAGINO_FILES = {
  "file_cpu_rom":    "pengo_rom.h",
  "array_cpu_rom":   "pengo_rom",
  "file_colormap":   "pengo_colormap.h",
  "array_colormap":  "pengo_colormap", 
  "file_tilemap":    "pengo_tiles.h",
  "array_tilemap":   "pengo_tiles",
  "file_spritemap":  "pengo_spritemap.h",
  "array_spritemap": "pengo_sprites",
  "file_wavetable":  "pengo_wavetable.h",
  "array_wavetable": "pengo_wavetable",

  "preview_tiles":   "pengo_tiles_preview.png",
  "preview_sprites": "pengo_sprites_preview.png",
}

# -------------------------------------------------------------------
# ---- Write C header files ----
# -------------------------------------------------------------------
def write_rom(romset_name, filename, array_name, data):
  with open(filename, 'w') as f:
    f.write("// {} cpu rom ({} bytes)\n".format(romset_name, len(data)))
    f.write("const unsigned char {}[] = {{\n".format(array_name))
    for i in range(0, len(data), 16):
      line = ", ".join(hex8(data[j]) for j in range(i, min(i+16, len(data))))
      f.write("  " + line)
      if i + 16 < len(data):
        f.write(",")
      f.write("\n")
    f.write("};\n")
  print("Wrote: {} ({} bytes)".format(os.path.abspath(filename), len(data)))

# -------------------------------------------------------------------
def write_wavetable(romset_name, filename, array_name, wave_data):

  if len(wave_data) != 256:
    print("ERROR: wave data should be 256 bytes.")
    sys.exit(1)
    return

  with open(filename, 'w') as f:
    f.write(f"// Wavetable audio for {romset_name}\n")
    f.write("#pragma once\n\n")
    f.write(f"const signed char {array_name}[][32] = {{\n")
    for w in range(8):
      f.write(f"  // Waveform #{w}\n")
      
      # (Opzionale) Disegna una rappresentazione ASCII della forma d'onda
      for y in range(8):
        f.write("  //")
        for s in range(32):
          val = wave_data[32*w+s]
          if val == 15 - 2*y:
            f.write("---")
          elif val == 15 - (2*y+1):
            f.write("___")
          else:
            f.write("   ")
        f.write("\n")
      # write 32 values for waveform
      f.write("  {")
      for s in range(32):
        # Convert from 0-15 (unsigned) into -7 to +8 (signed)
        # centered around 0
        sample_value = wave_data[32*w+s] - 7
        f.write(f"{sample_value:3d}")
        if s != 31:
          f.write(",")
      f.write(" }")
      if w != 7:
        f.write(",")   
      f.write("\n\n")
    f.write("};\n")
  print("Wrote: {} ({} bytes)".format(os.path.abspath(filename), len(wave_data)))

# -------------------------------------------------------------------

def process_pengo_colormaps(palette_prom_data, lookup_prom_data):

  base_palette_rgb565 = []
  for i in range(32):
    prom_byte = palette_prom_data[i]
    r = 0x21 * get_bit(prom_byte, 0) + 0x47 * get_bit(prom_byte, 1) + 0x97 * get_bit(prom_byte, 2)
    g = 0x21 * get_bit(prom_byte, 3) + 0x47 * get_bit(prom_byte, 4) + 0x97 * get_bit(prom_byte, 5)
    b = 0x55 * get_bit(prom_byte, 6) + 0xae * get_bit(prom_byte, 7)
    base_palette_rgb565.append(rgb888_to_rgb565_le(r, g, b))
  print(f"Decoded {len(base_palette_rgb565)} base colors.")

  final_colormap = []
  # Itera sui 2 'palette_bank' (selezionati dal gioco via I/O 0x9042)
  for bank in range(2):
    bank_data = []

    # L'hardware applica un offset di 0x10 agli indici colore quando il bank è 1
    color_offset = 0x10 if bank == 1 else 0x00

    # Itera sui 256 possibili "gruppi" di palette
    # Un gruppo è selezionato da 2 bit di 'colortable_bank' + 6 bit di 'color_attr'
    for group_index in range(256):
      group_data = []
      # L'indirizzo base nella PROM di lookup per questo gruppo
      lookup_base_addr = group_index * 4

      # Itera sui 4 colori del gruppo (pen 0, 1, 2, 3)
      for pen in range(4):
        # Il colore 0 (pen 0) è sempre trasparente
        if pen == 0:
          group_data.append(0x0000)
          continue

        # Legge l'indice a 5 bit dalla lookup table PROM
        raw_color_index = lookup_prom_data[lookup_base_addr + pen]

        # Applica l'offset del banco e una maschera per assicurarsi che resti a 5 bit (0-31)
        final_color_index = (raw_color_index + color_offset) & 0x1F

        # Ottiene il colore RGB565 finale dalla palette base e lo aggiunge
        final_color = base_palette_rgb565[final_color_index]
        group_data.append(final_color)
      bank_data.append(group_data)
    final_colormap.append(bank_data)

  return final_colormap

# -------------------------------------------------------------------

def write_colormap(romset_name, filename, array_name, colormap):

  with open(filename, 'w') as f:
    f.write(f"// colormap for {romset_name}\n")
    f.write("#pragma once\n")
    f.write("#include <stdint.h>\n\n")
    f.write(f"// Colormap finale [palette_bank][group_index][pen] -> [2][256][4]\n")
    f.write(f"// Questa tabella contiene già i colori finali in RGB565 Little Endian.\n")
    f.write(f"const uint16_t {array_name}[2][256][4] = {{\n")
    
    for bank_idx, bank_data in enumerate(colormap):
      f.write(f"  {{ // --- PALETTE BANK {bank_idx} (I/O 0x9042 = {bank_idx}) ---\n")
      for group_idx, group_data in enumerate(bank_data):
                f.write(f"    {{ ") # Un gruppo di 4 colori
                f.write(", ".join([f"0x{color:04x}" for color in group_data]))
                f.write(f" }}, // Gruppo {group_idx}\n")
      f.write("  },\n")
    f.write("};\n")

  print("Wrote: {} ({} banks)".format(os.path.abspath(filename), len(colormap)))

# -------------------------------------------------------------------

def write_tilemap(romset_name, filename, array_name, tilemap):

  with open(filename, 'w') as f:
    f.write(f"// tilemap for {romset_name}\n")
    f.write("#pragma once\n")
    f.write("#include <stdint.h>\n\n")
    f.write(f"const uint16_t {array_name}[512][8] = {{\n")
    f.write(",\n".join([f"  /* T{i} */ {{ {dump_tile_packed(t)} }}" for i, t in enumerate(tilemap)]))
    f.write("\n};\n")

  print("Wrote: {}".format(os.path.abspath(filename)))

# -------------------------------------------------------------------

def write_spritemap(romset_name, filename, array_name, spritemap):

  with open(filename, 'w') as f:
    f.write(f"// spritemap for {romset_name}\n")
    f.write("#pragma once\n")
    f.write("#include <stdint.h>\n\n")
    f.write(f"const unsigned long {array_name}[2][4][64][16] = {{\n")
    bank_lines = []
    for bank in range(2):
      flip_lines = []
      for flip_val in range(4):
        flip_y = (flip_val & 1) != 0; flip_x = (flip_val & 2) != 0
        sprite_lines = []
        for i in range(64):
          sprite_idx = bank * 64 + i
          flipped = flip_sprite(spritemap[sprite_idx], flip_x, flip_y)
          sprite_lines.append(f"    /* S{i} F{flip_val} */ {{ {dump_sprite_packed(flipped)} }}")
        flip_lines.append("  {\n" + ",\n".join(sprite_lines) + "\n  }")
      bank_lines.append(" {\n" + ",\n".join(flip_lines) + "\n }")
    f.write(",\n".join(bank_lines) + "\n};\n")

# -------------------------------------------------------------------

def convert_pengo(romset, files, galagino):

  os.makedirs(OUT_DIR, exist_ok=True)

  print(f"Load ROM from: {os.path.abspath(romset)}")
  print(f"Target files:  {os.path.abspath(OUT_DIR)}")

  cpu0  = load_file(romset, files["cpu0"]["names"], files["cpu0"]["sha1"])
  cpu1  = load_file(romset, files["cpu1"]["names"], files["cpu1"]["sha1"])
  cpu2  = load_file(romset, files["cpu2"]["names"], files["cpu2"]["sha1"])
  cpu3  = load_file(romset, files["cpu3"]["names"], files["cpu3"]["sha1"])
  cpu4  = load_file(romset, files["cpu4"]["names"], files["cpu4"]["sha1"])
  cpu5  = load_file(romset, files["cpu5"]["names"], files["cpu5"]["sha1"])
  cpu6  = load_file(romset, files["cpu6"]["names"], files["cpu6"]["sha1"])
  cpu7  = load_file(romset, files["cpu7"]["names"], files["cpu7"]["sha1"])

  gfx1  = load_file(romset, files["gfx1"]["names"], files["gfx1"]["sha1"])
  gfx2  = load_file(romset, files["gfx2"]["names"], files["gfx2"]["sha1"])

  prom1 = load_file(romset, files["prom1"]["names"], files["prom1"]["sha1"])
  prom2 = load_file(romset, files["prom2"]["names"], files["prom2"]["sha1"])

  sound1 = load_file(romset, files["sound1"]["names"], files["sound1"]["sha1"])
  sound2 = load_file(romset, files["sound2"]["names"], files["sound2"]["sha1"])

  files_ok = all(v is not None for v in [cpu0, cpu1, cpu2, cpu3, cpu4, cpu5, cpu6, cpu7,
                                         gfx1, gfx2,
                                         prom1, prom2,
                                         sound1, sound2])
  if not files_ok:
    print("ERROR: Not all files have been loaded")
    sys.exit(1)
    return

  main_cpu = cpu0 + cpu1 + cpu2 + cpu3 + cpu4 + cpu5 + cpu6 + cpu7
  write_rom(files["romset"]["description"],
            os.path.join(OUT_DIR, galagino["file_cpu_rom"]),
            galagino["array_cpu_rom"],
            main_cpu)

  write_wavetable(files["romset"]["description"],
                  os.path.join(OUT_DIR, galagino["file_wavetable"]),
                  galagino["array_wavetable"],
                  sound1)

  final_colormap = process_pengo_colormaps(prom1, prom2)
  write_colormap(files["romset"]["description"],
                 os.path.join(OUT_DIR, galagino["file_colormap"]),
                 galagino["array_colormap"],
                 final_colormap)

  tile_data_raw = gfx1[:4096] + gfx2[:4096]
  decoded_tiles = [decode_tile_8x8(tile_data_raw[i*16:(i+1)*16]) for i in range(512)]
  final_tiles = [rotate_gfx(t, 8, 8) for t in decoded_tiles]
  write_tilemap(files["romset"]["description"],
                os.path.join(OUT_DIR, galagino["file_tilemap"]),
                galagino["array_tilemap"],
                final_tiles)
  tiles_create_preview(final_tiles, 8, 8, galagino["preview_tiles"])

  sprite_data_raw = gfx1[4096:] + gfx2[4096:]
  num_sprites = len(sprite_data_raw) // 64

  print(f"Processing {num_sprites} sprites...")
  decoded_sprites = [decode_sprite_16x16_from_mame(sprite_data_raw[i*64:(i+1)*64]) for i in range(num_sprites)]
  final_sprites_unflipped = [rotate_gfx(s, 16, 16) for s in decoded_sprites]
  write_spritemap(files["romset"]["description"],
                  os.path.join(OUT_DIR, galagino["file_spritemap"]),
                  galagino["array_spritemap"],
                  final_sprites_unflipped)
  sprites_create_preview(final_sprites_unflipped, 16, 16, galagino["preview_sprites"])

# -------------------------------------------------------------------

def main():
  if os.path.isfile(ROM_SET_J):
    convert_pengo(ROM_SET_J, PENGOJ_FILES, GALAGINO_FILES)
  elif os.path.isfile(ROM_SET_W):
    convert_pengo(ROM_SET_W, PENGOW_FILES, GALAGINO_FILES)
  elif os.path.isfile(ROM_SET_2u):
    convert_pengo(ROM_SET_2u, PENGO2u_FILES, GALAGINO_FILES)
  else:
    print("ERROR: No roms.")
    sys.exit(1)

# -------------------------------------------------------------------

if __name__ == "__main__":
    main()

