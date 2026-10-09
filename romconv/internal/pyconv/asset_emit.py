#!/usr/bin/env python3
# Asset<T> C-source emitter shared by the romconv converters.
#
# Emits the array plus a constexpr FlashAsset `<name>_blob` describing it. The
# machine owns an Asset member built from the blob, so the unpack cache
# dies with the machine. Only menu logos (no machine instance) get a
# program-lifetime static Asset object (static_object=True).
import struct
import zlib

_WIDTH = {"unsigned char": 1, "signed char": 1, "unsigned short": 2, "uint32_t": 4}
_PACK  = {"unsigned char": "B", "signed char": "B", "unsigned short": "<H", "uint32_t": "<L"}

def _hex_block(data, per_line=16):
    hexs = ["0x{:02X}".format(b) for b in data]
    rows = [",".join(hexs[i:i + per_line]) for i in range(0, len(hexs), per_line)]
    return ",\n  ".join(rows)

def emit_compressed(f, name, ctype, inner_dims, count, flat_values, static_object=False):
    """Write a zlib-compressed FlashAsset<ctype inner_dims> definition.

    flat_values: ints in C initializer (row-major) order.
    """
    width = _WIDTH[ctype]
    raw = b"".join(struct.pack(_PACK[ctype], v & ((1 << (8 * width)) - 1)) for v in flat_values)
    packed = zlib.compress(raw, 9)
    ratio = 100.0 * (1.0 - len(packed) / len(raw)) if raw else 0.0

    print('#include "../../emulation/asset.h"', file=f)
    print(f"// {name}: {len(raw)} -> {len(packed)} bytes ({ratio:.1f}% smaller)", file=f)
    print(f"static const unsigned char {name}_packed[] = {{\n  " + _hex_block(packed) + "\n};", file=f)
    print(f"static constexpr FlashAsset<{ctype}{inner_dims}, COMPRESSED> {name}_blob = {{ {name}_packed, sizeof({name}_packed), {count} }};", file=f)
    if static_object:
        print(f"static Asset<{ctype}{inner_dims}, COMPRESSED> {name}({name}_blob);", file=f)

def emit_plain(f, name, ctype, inner_dims, count, array_body):
    """Write a plain (flash-resident) FlashAsset<ctype inner_dims> definition.

    array_body: formatted `{ ... }` initializer text from the dump_* helpers.
    """
    print('#include "../../emulation/asset.h"', file=f)
    print(f"static const {ctype} {name}_data[]{inner_dims} = {{\n{array_body}\n}};", file=f)
    print(f"static constexpr FlashAsset<{ctype}{inner_dims}, PLAIN> {name}_blob = {{ {name}_data, {count} }};", file=f)

def emit_header(f, name, ctype, inner_dims, count, array_body):

    print(f"static const {ctype} {name}{inner_dims} = {{\n{array_body}\n}};", file=f)
