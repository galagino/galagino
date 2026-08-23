#ifndef MAPPY_DIPSWITCHES_H
#define MAPPY_DIPSWITCHES_H

// DSW0 (4 bit, 58XX #1 porte 30-33), active low: bit0 = service mode off
#define MAPPY_DSW0  0x0f

// DSW1 (58XX #1 porte 22-29), active low default MAME 0xff:
// bit0-2 difficolta' (7=Rank A ... 0=Rank H), bit3-4 Coin B (0x18=1C1C),
// bit5 demo sounds (0x20=on), bit6 rack test (0x40=off), bit7 freeze (0x80=off)
#define MAPPY_DSW1  0xff

// DSW2 (multiplexato via LS157), default 0xFF:
// bit0-2 Coin A (0x07=1C1C)
// bit3-5 bonus  (0x38=20k&70k with 3 lives),
// bit6-7 lives  (0xc0=3)
#define MAPPY_DSW2  0xff

#endif
