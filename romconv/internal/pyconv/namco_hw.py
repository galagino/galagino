#!/usr/bin/env python3
# Decode helpers for Namco-family hardware (Galaga/Mappy/Xevious/
# Tower of Druaga lineage).


def flip_tile(tile, fx, fy):
    out = tile
    if fy: out = list(reversed(out))
    if fx: out = [list(reversed(r)) for r in out]
    return out


def nudge(v):
    # 0 is the transparency marker; map true black to near-black.
    return v if v != 0 else 0x2000


def rgb565_swapped_rgb(r, g, b):
    # r,g,b already 0..255.
    rgb = ((r * 31 // 255) << 11) + ((g * 63 // 255) << 5) + (b * 31 // 255)
    return ((rgb & 0xff00) >> 8) + ((rgb & 0xff) << 8)


def rgb565_swapped_packed(c):
    # c is a bbgggrrr packed byte.
    b = 31 * ((c >> 6) & 0x3) // 3
    g = 63 * ((c >> 3) & 0x7) // 7
    r = 31 * ((c >> 0) & 0x7) // 7
    rgb = (r << 11) + (g << 5) + b
    return ((rgb & 0xff00) >> 8) + ((rgb & 0xff) << 8)


def pal_rgb(c):
    return (255 * ((c >> 0) & 7) // 7, 255 * ((c >> 3) & 7) // 7, 255 * ((c >> 6) & 3) // 3)


def parse_chr_ref(data):
    # Reference decode for self-tests against a known Galaga ROM.
    char = []
    for y in range(8):
        row = []
        for x in range(8):
            byte = data[15 - x - 2 * (y & 4)]
            c0 = 1 if byte & (0x08 >> (y & 3)) else 0
            c1 = 2 if byte & (0x80 >> (y & 3)) else 0
            row.append(c0 + c1)
        char.append(row)
    return char


def parse_sprite_ref(data):
    # Reference decode for self-tests.
    sprite = []
    for y in range(16):
        row = []
        for x in range(16):
            idx = ((y & 8) << 1) + (((x & 8) ^ 8) << 2) + (7 - (x & 7)) + 2 * (y & 4)
            c0 = 1 if data[idx] & (0x08 >> (y & 3)) else 0
            c1 = 2 if data[idx] & (0x80 >> (y & 3)) else 0
            row.append(c0 + c1)
        sprite.append(row)
    return sprite
