#!/usr/bin/env python3
# Tile/sprite decode for Galaxian-family hardware (2bpp tile planes,
# Frogger-style 16x16 sprite layout).


def parse_chr_2(data0, data1):
    # Parse 8x8 tile from two separate plane ROMs.
    char = []
    for y in range(8):
        row = []
        for x in range(8):
            c0 = 1 if data0[7 - x] & (0x80 >> y) else 0
            c1 = 2 if data1[7 - x] & (0x80 >> y) else 0
            row.append(c0 + c1)
        char.append(row)
    return char


def dump_chr(data):
    vals = []
    for y in range(8):
        val = 0
        for x in range(8):
            val = (val >> 2) + (data[y][x] << (16 - 2))
        vals.append(val)
    return vals


def convert_tiles(plane0, plane1):
    num_tiles = len(plane0) // 8
    tiles = []
    for t in range(num_tiles):
        d0 = plane0[t * 8 : t * 8 + 8]
        d1 = plane1[t * 8 : t * 8 + 8]
        char_data = parse_chr_2(d0, d1)
        tiles.append(dump_chr(char_data))
    return tiles


def parse_sprite_galaxian(data0, data1):
    # Parse 16x16 sprite from Galaxian-family hardware (same as Frogger
    # without D0/D1 swap).
    sprite = []
    for y in range(16):
        row = []
        for x in range(16):
            ym = (y & 7) | ((x & 8) ^ 8)
            xm = (x & 7) | (y & 8)
            byte_idx = (xm ^ 7) + ((ym & 8) << 1)
            bit_mask = 0x80 >> (ym & 7)
            c0 = 1 if data0[byte_idx] & bit_mask else 0
            c1 = 2 if data1[byte_idx] & bit_mask else 0
            row.append(c0 + c1)
        sprite.append(row)
    return sprite


def dump_sprite(data, flip_x, flip_y):
    vals = []
    y_range = range(16) if not flip_y else reversed(range(16))
    for y in y_range:
        val = 0
        for x in range(16):
            if not flip_x:
                val = (val >> 2) + (data[y][x] << (32 - 2))
            else:
                val = (val << 2) + data[y][x]
        vals.append(val)
    return vals


def convert_sprites(plane0, plane1):
    num_sprites = len(plane0) // 32
    sprites = []
    for s in range(num_sprites):
        d0 = plane0[32 * s : 32 * (s + 1)]
        d1 = plane1[32 * s : 32 * (s + 1)]
        sprites.append(parse_sprite_galaxian(d0, d1))
    all_orientations = []
    for flip_x, flip_y in [(False, False), (False, True), (True, False), (True, True)]:
        orientation = [dump_sprite(s, flip_x, flip_y) for s in sprites]
        all_orientations.append(orientation)
    return all_orientations
