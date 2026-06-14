import os
import zipfile
import hashlib

# -------------------------------------------------------------------
def hex8(v):
  return "0x{:02x}".format(v & 0xFF)

def hex16(v):
  return "0x{:04x}".format(v & 0xFFFF)

def hex32(v):
  return "0x{:08x}".format(v & 0xFFFFFFFF)

# -------------------------------------------------------------------
# load file from zipfile and check hash
# -------------------------------------------------------------------
# load file from zipfile and check hash
def load_file(rom_set, names, sha1):
  for name in names:
    with zipfile.ZipFile(rom_set) as z:
      if name in z.namelist():
        with z.open(name, 'r') as file:
          rom = bytearray(file.read())
          if check_file(name, rom, sha1):
            #print(f"Loaded {name} from romset.")
            return rom
          return None
  for name in names:
    print(f"ERROR: File '{name}' not found in {os.path.abspath(rom_set)}")
    return None

# -------------------------------------------------------------------
def check_file(name, b, h):
  digest = hashlib.sha1(b).hexdigest()
  if h != digest:
    print(f"bad hash for {name} {h}!={digest}.")
    return False
  return True

# -------------------------------------------------------------------
def get_bit(value, bit):
  return (value >> bit) & 1

# -------------------------------------------------------------------
def rgb888_to_rgb565_le(r, g, b):
  """
  Convert RGB 888 into RGB565 Little Endian.
  """
  r, g, b = [max(0, min(255, c)) for c in (r, g, b)]
  val_be = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3)
  return ((val_be & 0x00FF) << 8) | ((val_be & 0xFF00) >> 8)






# -------------------------------------------------------------------

