#!/usr/bin/env python3
# Checks asset_emit.py output without a C++ build: structure and zlib
# round-trip.
import io, re, zlib, struct
from asset_emit import emit_compressed, emit_plain

# 1. compressed byte array: values round-trip through zlib exactly
values = list(range(256)) * 4   # 1024 unsigned char values
f = io.StringIO()
emit_compressed(f, "_test_rom", "unsigned char", "", len(values), values)
out = f.getvalue()

m = re.search(r"_test_rom_packed\[\] = \{(.*?)\};", out, re.S)
packed_bytes = bytes(int(x, 16) for x in re.findall(r"0x[0-9A-Fa-f]{2}", m.group(1)))
restored = zlib.decompress(packed_bytes)
expected = bytes(v & 0xFF for v in values)
assert restored == expected, "compressed round-trip mismatch"
assert "static constexpr FlashAsset<unsigned char, COMPRESSED> _test_rom_blob = { _test_rom_packed, sizeof(_test_rom_packed), 1024 };" in out
assert "static Asset<" not in out
print("1. emit_compressed byte round-trip + FlashAsset line: OK")

# 2. compressed nested type (unsigned short rows): width and byte order
values16 = [0x1234, 0xABCD, 0x0001, 0xFFFF]
f = io.StringIO()
emit_compressed(f, "_test_rows", "unsigned short", "[2]", 2, values16)
out = f.getvalue()
m = re.search(r"_test_rows_packed\[\] = \{(.*?)\};", out, re.S)
packed_bytes = bytes(int(x, 16) for x in re.findall(r"0x[0-9A-Fa-f]{2}", m.group(1)))
restored = zlib.decompress(packed_bytes)
expected = b"".join(struct.pack("<H", v) for v in values16)
assert restored == expected, "16-bit little-endian round-trip mismatch"
assert "static constexpr FlashAsset<unsigned short[2], COMPRESSED> _test_rows_blob = { _test_rows_packed, sizeof(_test_rows_packed), 2 };" in out
print("2. emit_compressed 16-bit row round-trip + declaration: OK")

# 3. plain (uncompressed) emitter just wraps the given body text
f = io.StringIO()
emit_plain(f, "_test_plain", "uint32_t", "[2]", 3, " { 0x1,0x2 },\n { 0x3,0x4 }")
out = f.getvalue()
assert "static const uint32_t _test_plain_data[][2] = {" in out
assert "static constexpr FlashAsset<uint32_t[2], PLAIN> _test_plain_blob = { _test_plain_data, 3 };" in out
print("3. emit_plain wraps body + FlashAsset line: OK")

# 4. compressed array includes size/ratio comment
f = io.StringIO()
emit_compressed(f, "test_arr", "unsigned char", "", 4, [0, 0, 0, 0])
out = f.getvalue()
assert "// test_arr: 4 -> " in out, "missing size/ratio comment"
assert "bytes" in out and "%" in out
print("4. emit_compressed includes size/ratio comment: OK")

# 5. static_object (menu logos) also emits a static Asset from the blob
f = io.StringIO()
emit_compressed(f, "_test_logo", "unsigned short", "", 2, [1, 2], static_object=True)
assert "static Asset<unsigned short, COMPRESSED> _test_logo(_test_logo_blob);" in f.getvalue()
print("5. emit_compressed static_object: OK")

print("\nALL ASSET_EMIT TESTS PASSED")
