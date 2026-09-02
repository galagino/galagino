import os
import zipfile
import hashlib

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

