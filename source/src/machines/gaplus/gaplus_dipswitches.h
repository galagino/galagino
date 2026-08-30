#ifndef GAPLUS_DIPSWITCHES_H
#define GAPLUS_DIPSWITCHES_H

// DSWA (58XX in0/in3), attivo basso, default MAME 0xFF:
// bit0-1 Coin B (0x03=1C1C), bit3 demo sounds (0x08=on, DSWA_LOW)
// bit0-1 Coin A (0x03=1C1C), bit2-3 vite (0x0C=3, DSWA_HIGH)
#define GAPLUS_DSWA_LOW   0xff
#define GAPLUS_DSWA_HIGH  0xff

// DSWB (58XX in1/in2), attivo basso, default MAME 0xFF:
// bit3 round advance (0x08=off), bit0-2 bonus life (0x07, DSWB_LOW)
// bit3 unknown (0x08=off), bit0-2 difficulty (0x07=standard, DSWB_HIGH)
#define GAPLUS_DSWB_LOW   0xff
#define GAPLUS_DSWB_HIGH  0xff

// IN2 (customio_3): bit2 cabinet (1=upright), bit3 service (active LOW)
#define GAPLUS_CABINET_UPRIGHT  0x04

#endif
