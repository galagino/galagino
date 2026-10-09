import os
import zipfile
import hashlib

from convutil import fatal

# -------------------------------------------------------------------
# load file from zipfile and check hash
# -------------------------------------------------------------------
def load_file(rom_set, names, sha1):
  for name in names:
    with zipfile.ZipFile(rom_set) as z:
      if name in z.namelist():
        with z.open(name, 'r') as file:
          rom = bytearray(file.read())
          check_file(name, rom, sha1)
          return rom
  for name in names:
    fatal(f"File '{name}' not found in {os.path.abspath(rom_set)}")

def check_file(name, b, h):
  if h is None:
    return True
  digest = hashlib.sha1(b).hexdigest()
  if h != digest:
    fatal(f"bad hash for {name} {h}!={digest}.")
  return True

# -------------------------------------------------------------------
# generic gfx decoder MAME (planes/xoffs/yoffs as absolute OFFSET BIT)
# -------------------------------------------------------------------
def mame_decode(data, width, height, planes, xoffs, yoffs, bits_per_tile, count, base_bit=0):
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

# -------------------------------------------------------------------
# 90 deg rotation for galagino's portrait framebuffer
# -------------------------------------------------------------------
def rot_galagino(tile):
    n = len(tile)
    return [[tile[n - 1 - x][y] for x in range(n)] for y in range(n)]

# -------------------------------------------------------------------
def get_bit(value, bit):
  return (value >> bit) & 1

def rgb888_to_rgb565_le(r, g, b):
  r, g, b = [max(0, min(255, c)) for c in (r, g, b)]
  val_be = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3)
  return ((val_be & 0x00FF) << 8) | ((val_be & 0xFF00) >> 8)

# -------------------------------------------------------------------
# render a list of decoded tiles (list[y][x] pixel value) as a labeled
# grid PNG, for eyeballing a ROM converter's decode output
def render_tile_grid(tiles, tile_w, tile_h, palette, tiles_per_row, output_file, bg=(30, 30, 30)):
  from PIL import Image, ImageDraw, ImageFont

  num_rows = (len(tiles) + tiles_per_row - 1) // tiles_per_row
  cell_w, cell_h = tile_w + 4, tile_h + 14
  img = Image.new('RGB', (cell_w * tiles_per_row, cell_h * num_rows), bg)
  draw = ImageDraw.Draw(img)
  font = ImageFont.load_default()

  for idx, tile in enumerate(tiles):
    row_idx, col_idx = divmod(idx, tiles_per_row)
    x0, y0 = col_idx * cell_w + 2, row_idx * cell_h + 2
    for y in range(tile_h):
      for x in range(tile_w):
        img.putpixel((x0 + x, y0 + y), palette.get(tile[y][x], bg))
    draw.text((x0, y0 + tile_h), str(idx), font=font, fill=(200, 200, 200))
    draw.rectangle([x0 - 1, y0 - 1, x0 + tile_w, y0 + tile_h], outline=(80, 80, 80))

  img.save(output_file)
  print(f"Preview saved as '{output_file}' ({len(tiles)} tiles)")

# -------------------------------------------------------------------
def hex8(v):
  return "0x{:02x}".format(v & 0xFF)

def hex16(v):
  return "0x{:04x}".format(v & 0xFFFF)

def hex32(v):
  return "0x{:08x}".format(v & 0xFFFFFFFF)
