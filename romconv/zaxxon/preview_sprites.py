#!/usr/bin/env python3
import os
import sys

sys.path.insert(0, os.path.join("..", "internal", "pyconv"))
from gfxutil import load_file, render_tile_grid
from zaxxon_rom_convert import ROM_SET, ZAXXON_FILES, decode_sprite_tiles

OUTPUT_IMAGE_FILE = "zaxxon_sprites_preview.png"
TILES_PER_ROW = 8
PREVIEW_PALETTE = {
    0: (0, 0, 0), 1: (255, 85, 85), 2: (85, 255, 85), 3: (255, 255, 85),
    4: (85, 85, 255), 5: (255, 85, 255), 6: (85, 255, 255), 7: (255, 255, 255),
}

def main():
    spr1 = load_file(ROM_SET, ZAXXON_FILES["gfx_spr_1"]["names"], ZAXXON_FILES["gfx_spr_1"]["sha1"])
    spr2 = load_file(ROM_SET, ZAXXON_FILES["gfx_spr_2"]["names"], ZAXXON_FILES["gfx_spr_2"]["sha1"])
    spr3 = load_file(ROM_SET, ZAXXON_FILES["gfx_spr_3"]["names"], ZAXXON_FILES["gfx_spr_3"]["sha1"])
    tiles = decode_sprite_tiles(spr1 + spr2 + spr3)
    render_tile_grid(tiles, 32, 32, PREVIEW_PALETTE, TILES_PER_ROW, OUTPUT_IMAGE_FILE)

if __name__ == "__main__":
    main()
