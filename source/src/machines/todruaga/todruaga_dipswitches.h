#ifndef TODRUAGA_DIPSWITCHES_H
#define TODRUAGA_DIPSWITCHES_H

// DSW1 (56XX #1 port 22-29), actve low - default MAME 0xff:
// bit0-1 lives (0x03=3, 0x02=2, 0x01=1, 0x00=5), bit2-3 Coin A (0x0c=1C1C),
// bit4 freeze (0x10=off), bit5 service mode (0x20=off),
// bit6-7 Coin B (0xc0=1C1C)
#define TODRUAGA_DSW1  0xfc

// DSW2 (multiplexed via LS157): no bits used on todruaga
#define TODRUAGA_DSW2  0xff

// DSW0 (4 bit, 56XX #1 port 30-33), active low:
// bit0-1 unused, bit2 cabinet (1=upright), bit3 service (1=off)
#define TODRUAGA_DSW0  0x0f

#endif
