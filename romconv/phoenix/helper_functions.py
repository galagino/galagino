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

def STEP2(start: int, step: int) -> [int, ...]:
  return [(start) + ((step)*0), ((start) + (step)*1)]

def STEP4(start: int, step: int) -> [int, ...]:
  return STEP2(start, step) + STEP2((start)+((step)*2), step)

def STEP8(start: int, step: int) -> [int, ...]:
  return STEP4(start, step) + STEP4((start)+((step)*4), step)

def STEP16(start: int, step: int) -> [int, ...]:
  return STEP8(start, step) + STEP8((start)+((step)*8), step)

def STEP32(start: int, step: int) -> [int, ...]:
  return STEP16(start, step), STEP16((start)+((step)*16), step)

def STEP64(start: int, step: int) -> [int, ...]:
  STEP32(start, step) + STEP32((start)+((step)*32), step)

def RGN_FRAC(length: int, numerator: int, denominator: int) -> int:
  return ((((length) * 8) * (numerator)) / (denominator))

def readbit(src: bytearray, bitnum: int) -> int:
  bit_addr = int(bitnum // 8)
  bit_mask = int(0x80 >> int(int(bitnum) % 8))
  return int(src[bit_addr] & bit_mask)

# -------------------------------------------------------------------

def GfxDecode(*, num: int, numPlanes: int, xSize: int, ySize: int, 
              planeOffsets, xoffsets, yoffsets,
              modulo: int, rom_data):
  all_gfx = [int(0)] * num * xSize * ySize
  for c in range(0, num):
    for plane in range(0, numPlanes):
      planebit  = int(1 << (numPlanes - 1 - plane))
      planeoffs = int((c * modulo) + planeOffsets[plane])

      for y in range(0, ySize):
        yoffs = int(planeoffs + yoffsets[y])
        dp = int((c * xSize * ySize) + (y * xSize))

        for x in range(0, xSize):
          if readbit(rom_data, yoffs + xoffsets[x]) > 0:
            all_gfx[dp+ x] = int(all_gfx[dp + x]) | int(planebit)
  return all_gfx

# -------------------------------------------------------------------
# load file from zipfile and check hash
# -------------------------------------------------------------------
# load file from zipfile and check hash
def load_file(rom_set, names, sha1=None):
  for name in names:
    with zipfile.ZipFile(rom_set) as z:
      if name in z.namelist():
        with z.open(name, 'r') as file:
          rom = bytearray(file.read())
          if check_file(name, rom, sha1):
            print(f"Loaded {name} from romset.")
            return rom
          return None
  for name in names:
    print(f"ERROR: File '{name}' not found in {os.path.abspath(rom_set)}")
    return None

# -------------------------------------------------------------------
def check_file(name, b, h):
  if h is None:
    return True
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

BLACK   = "\033[30m"
RED     = "\033[31m"
GREEN   = "\033[32m"
YELLOW  = "\033[33m"
BLUE    = "\033[34m"
MAGENTA = "\033[35m"
CYAN    = "\033[36m"
WHITE   = "\033[37m"

BRIGHT_BLACK   = "\033[90m"
BRIGHT_RED     = "\033[91m"
BRIGHT_GREEN   = "\033[92m"
BRIGHT_YELLOW  = "\033[93m"
BRIGHT_BLUE    = "\033[94m"
BRIGHT_MAGENTA = "\033[95m"
BRIGHT_CYAN    = "\033[96m"
BRIGHT_WHITE   = "\033[97m"

RESET = "\033[0m"

pallete_color3 = [
BLACK,
BRIGHT_WHITE,   
GREEN,
YELLOW,
RED,  
BRIGHT_BLUE,
BRIGHT_BLACK,
BRIGHT_CYAN,
]

def color3bits(idx):
  if idx == 0:
    return pallete_color3[idx] + "\u25AA" + RESET
  return pallete_color3[idx] + "\u2588" + RESET
  #return pallete_color3[idx] + "\u25A0"  + RESET

# -------------------------------------------------------------------


