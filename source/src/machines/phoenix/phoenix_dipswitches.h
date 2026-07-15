#ifndef PHOENIX_DIPSWITCHES_H
#define PHOENIX_DIPSWITCHES_H

// MAME default DSW0:
// bit 0-1 = lives (0=3, 1=4, 2=5, 3=6) -> default 0x00 (3 lives)
// bit 2-3 = bonus (0=3K/30K, 1=4K/40K, ...) -> default 0x00
// bit 4   = coinage (0=2c1c, 1=1c1c) -> default 0x10
// bit 5,6 = unknown -> 0
// bit 7   = VBLANK (managed in rdZ80)
//
#define PHOENIX_DSW0 0x10

#endif
