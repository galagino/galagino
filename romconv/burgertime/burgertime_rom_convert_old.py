#!/usr/bin/env python3
"""
Burger Time ROM converter for GALAGINO


Main CPU:
  0xc000-0xffff - main cpu rom - aa04.9b + aa06.13b + aa05.10b + aa07.15b

Sound CPU:
  0xe000-0xefff - sound rom - ab14.12h
  0xf000-0xffff - sound rom - ab14.12h

"""

import os
import sys
import zipfile
import hashlib

sys.dont_write_bytecode = True

ROM_SET = os.path.normpath(os.path.join("..", "..", "romszip", "btime.zip"))
OUT_DIR = os.path.normpath(os.path.join("..", "..", "source", "src", "machines", "burgertime"))

from helper_functions import hex8, hex16, hex32
from helper_functions import load_file
from helper_functions import get_bit, rgb888_to_rgb565_le
from helper_functions import RGN_FRAC, STEP8, STEP16
from helper_functions import GfxDecode
from helper_functions import color3bits

BURGERTIME_FILES = {
  "romset": {"name": "btime.zip", "description": "Burger Time (Data East set 1)"},
 
  "maincpu01": {"names": ["aa04.9b"],  "sha1": "ed3f3712423979dcb351941fa85dce6a0a7bb16b"},
  "maincpu02": {"names": ["aa06.13b"], "sha1": "8c77397e934907bc47a739f263196a0f2f81ba3d"},
  "maincpu03": {"names": ["aa05.10b"], "sha1": "d0da4e360039f6a8d8142a4e8e05c1f90c0af68a"},
  "maincpu04": {"names": ["aa07.15b"], "sha1": "4a32bc92f8ff5fbe112f56e62d2c03da8851a7b9"},
  
  "audiocpu1": {"names": ["ab14.12h"], "sha1": "27940026d0c6212d1138d2fd88880df697218627"},
  
  "gfx1_1":    {"names": ["aa12.7k"],  "sha1": "24204d591aa2c264a852ee9ba8c4be63efd97728"},
  "gfx1_2":    {"names": ["ab13.9k"],  "sha1": "e64b6381a9298eaf74e79fa5f1ea8e9596c58a49"},
  "gfx1_3":    {"names": ["ab10.10k"], "sha1": "3d2ecfd54a5a9d68b53cf4b4ee1f2daa6aef2123"},
  "gfx1_4":    {"names": ["ab11.12k"], "sha1": "0a55b091cd4e7f317c35defe13d5051b26042eee"},
  "gfx1_5":    {"names": ["aa8.13k"],  "sha1": "d9b1ee2d1f2fd66705d497c80252861b49aa9254"},
  "gfx1_6":    {"names": ["ab9.15k"],  "sha1": "b72633de6268ce16742bba4dcba835df860d6c2f"},
  "gfx2_1":    {"names": ["ab00.1b"],  "sha1": "6a0a8e6b7860859f22daa33634e34fbf91387659"},
  "gfx2_2":    {"names": ["ab01.3b"],  "sha1": "4abdcbd4f3362c3e4463a1274731289f1a72d2e6"},
  "gfx2_3":    {"names": ["ab02.4b"],  "sha1": "4a03bf011dc1fb2902f42587b1174b880cf06df1"},
  "bg_map":    {"names": ["ab03.6b"],  "sha1": "737af6e264183a1f151f277a07cf250d6abb3fd8"},
}

GALAGINO_FILES = {
  "file_cpu_rom":    "burgertime_rom.h",
  "array_cpu_rom":   "burgertime_rom",
  "file_colormap":   "burgertime_colormap.h",
  "array_colormap":  "burgertime_colormap",
  "file_tilemap":    "burgertime_tiles.h",
  "array_tilemap":   "burgertime_tiles",
  "file_spritemap":  "burgertime_spritemap.h",
  "array_spritemap": "burgertime_sprites",

  "preview_tiles":   "burgertime_tiles_preview.png",
  "preview_sprites": "burgertime_sprites_preview.png",
}

def burgertime_gfx_decode(gfx0, gfx1):
  Plane_t8     = [ RGN_FRAC(len(gfx0), 2,3), RGN_FRAC(len(gfx0), 1,3), RGN_FRAC(len(gfx0), 0,3) ]
  Plane_t16spr = [ RGN_FRAC(len(gfx0), 2,3), RGN_FRAC(len(gfx0), 1,3), RGN_FRAC(len(gfx0), 0,3) ]
  Plane_t16    = [ RGN_FRAC(len(gfx1), 2,3), RGN_FRAC(len(gfx1), 1,3), RGN_FRAC(len(gfx1), 0,3) ]

  t8XOffs = STEP8(0,1) # assert len 8
  t8YOffs = STEP8(0,8) # assert len 8

  t16XOffs = STEP8(16*8,1) + STEP8(0,1) # assert len 16
  t16YOffs = STEP16(0,8)                # assert len 16

  if len(t8XOffs) != 8 or len(t8YOffs) != 8 or len(t16XOffs) != 16 or len(t16YOffs) != 16:
   print("Error:", len(t8XOffs), len(t8YOffs), len(t16XOffs), len(t16YOffs))
   os.exit(1)

  print("gfx0: tiles")
  tiles = GfxDecode(num=0x0400, numPlanes=3, xSize=8, ySize=8,
            planeOffsets=Plane_t8, xoffsets=t8XOffs, yoffsets=t8YOffs,
            modulo=0x40, rom_data=gfx0)

  print("gfx0: sprites")
  sprites1 = GfxDecode(num=0x0100, numPlanes=3, xSize=16, ySize=16,
            planeOffsets=Plane_t16spr, xoffsets=t16XOffs, yoffsets=t16YOffs,
            modulo=0x100, rom_data=gfx0)

  print("gfx1: sprites")
  sprites2 = GfxDecode(num=0x0040, numPlanes=3, xSize=16, ySize=16,
            planeOffsets=Plane_t16, xoffsets=t16XOffs, yoffsets=t16YOffs,
            modulo=0x100, rom_data=gfx1)

  print(len(tiles), len(sprites1), len(sprites2))

  for sprite_num in range(0, 0x100):
    for x in reversed(range(0, 16)):
      s = ""
      for y in range(0, 16):
        addr = sprite_num * 0x100 + y * 16 + x
        s += color3bits(sprites1[ addr ])
      print(s)
    print("")

  for tile_num in range(0, 0x400):
    for x in reversed(range(0, 8)):
      s = ""
      for y in range(0, 8):
        addr = tile_num * 0x40 + y * 8 + x
        #s += "{:01x} ".format(tiles[ addr ])
        s += color3bits(tiles[ addr ])
        
      print(s)
    print("")
    
    
  
  
  

  
  



# -------------------------------------------------------------------

def convert_burgertime(romset, files, galagino):

  os.makedirs(OUT_DIR, exist_ok=True)

  print(f"Load ROM from: {os.path.abspath(romset)}")
  print(f"Target files:  {os.path.abspath(OUT_DIR)}")

  cpu01 = load_file(romset, files["maincpu01"]["names"], files["maincpu01"]["sha1"])
  cpu02 = load_file(romset, files["maincpu02"]["names"], files["maincpu02"]["sha1"])
  cpu03 = load_file(romset, files["maincpu03"]["names"], files["maincpu03"]["sha1"])
  cpu04 = load_file(romset, files["maincpu04"]["names"], files["maincpu04"]["sha1"])

  audio = load_file(romset, files["audiocpu1"]["names"], files["audiocpu1"]["sha1"])

  gfx1_1 = load_file(romset, files["gfx1_1"]["names"], files["gfx1_1"]["sha1"])
  gfx1_2 = load_file(romset, files["gfx1_2"]["names"], files["gfx1_2"]["sha1"])
  gfx1_3 = load_file(romset, files["gfx1_3"]["names"], files["gfx1_3"]["sha1"])
  gfx1_4 = load_file(romset, files["gfx1_4"]["names"], files["gfx1_4"]["sha1"])
  gfx1_5 = load_file(romset, files["gfx1_5"]["names"], files["gfx1_5"]["sha1"])
  gfx1_6 = load_file(romset, files["gfx1_6"]["names"], files["gfx1_6"]["sha1"])

  gfx2_1 = load_file(romset, files["gfx2_1"]["names"], files["gfx2_1"]["sha1"])
  gfx2_2 = load_file(romset, files["gfx2_2"]["names"], files["gfx2_2"]["sha1"])
  gfx2_3 = load_file(romset, files["gfx2_3"]["names"], files["gfx2_3"]["sha1"])

  bg_map = load_file(romset, files["bg_map"]["names"], files["bg_map"]["sha1"])

  files_ok = all(v is not None for v in [cpu01, cpu02, cpu03, cpu04, audio,
                                         gfx1_1, gfx1_2, gfx1_3, gfx1_4, gfx1_5, gfx1_6,
                                         gfx2_1, gfx2_2, gfx2_3,
                                         bg_map])
  if not files_ok:
    print("ERROR: Not all files have been loaded")
    sys.exit(1)
    return

  main_cpu_rom  = cpu01 + cpu02 + cpu03 + cpu04
  if len(main_cpu_rom) != 0x4000:
    print("ERROR: Not all files have been loaded")
    sys.exit(1)

  audio_cpu_rom = audio
  if len(audio_cpu_rom) != 0x1000:
    print("ERROR: Not all files have been loaded")
    sys.exit(1)

  burgertime_gfx_decode(gfx1_1 + gfx1_2 + gfx1_3 + gfx1_4 + gfx1_5 + gfx1_6,
                        gfx2_1 + gfx2_2 + gfx2_3)
  





# -------------------------------------------------------------------

def main():
  if os.path.isfile(ROM_SET):
    convert_burgertime(ROM_SET, BURGERTIME_FILES, GALAGINO_FILES)
  else:
    print("ERROR: No roms.")
    sys.exit(1)

# -------------------------------------------------------------------

if __name__ == "__main__":
    main()










