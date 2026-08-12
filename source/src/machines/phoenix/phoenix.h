#ifndef PHOENIX_H
#define PHOENIX_H

#include "../machineBase.h"
#include "phoenix_logo.h"
#include "phoenix_rom.h"
#include "phoenix_bgtiles.h"
#include "phoenix_fgtiles.h"
#include "phoenix_palette.h"
#include "phoenix_dipswitches.h"

// ============================================================================
// Phoenix (Amstar/Centuri 1980) — driver MAME phoenix/phoenix.cpp
// CPU Z80, schermo 208x208 portrait (cabinet ROT270), 2 layer tilemap
// (FG + BG), 256 colori palette PROM, no IRQ (game polla VBLANK su DSW0 bit 7),
// audio TMS3617 custom NON emulato (silenzio).
//
// Memory map Z80:
//   0x0000-0x3FFF  ROM (16 KB)
//   0x4000-0x4FFF  VRAM 4 KB con 2 PAGINE (page index = videoreg bit 0)
//                    FG tilemap = vram[idx][0x000..0x3FF]
//                    BG tilemap = vram[idx][0x800..0xBFF]
//   0x5000-0x57FF  videoreg_w  (bit 0=page sel, bit 1=palette bank, cocktail)
//   0x5800-0x5FFF  scroll_w    (BG horizontal scroll, 8-bit)
//   0x6000-0x67FF  sound A control (soundregs[0]: FIRE/WING/SWOOP + boom ship)
//   0x6800-0x6FFF  sound B control (soundregs[1]: HIT enemies + melody select)
//   0x7000-0x77FF  IN0 read
//   0x7800-0x7FFF  DSW0 read (bit 7 = VBLANK live)
//
// Tile decode: 8x8, 2 bitplanes, 256 char per layer.
//   plane0 = bgtiles/fgtiles[code * 8 + py]
//   plane1 = bgtiles/fgtiles[code * 8 + py + 0x800]
//   pen    = bit(p0, 7-px) | (bit(p1, 7-px) << 1)
//
// Color attribute (MAME phoenix_v.cpp):
//   col_raw = (code >> 5) & 0x07
//   col = ((col_raw & 0x01) << 2) | (col_raw & 0x06) | ((col_raw & 0x06) >> 1)
//   final_col = col | (palette_bank ? 8 : 0)            // 4-bit, 0..15
//   pen index in palette[256] = final_col * 4 + raw_pen
// ============================================================================

class phoenix : public machineBase {
public:
  phoenix();

  void init(Input *input, unsigned short *framebuffer,
            sprite_S *spritebuffer, unsigned char *memorybuffer) override;
  void reset() override;

  signed char machineType()      override { return MCH_PHOENIX; }
  signed char videoFlipY()       override { return 0; }
  signed char videoFlipX()       override { return 0; }

  unsigned char rdZ80(unsigned short Addr) override;
  void          wrZ80(unsigned short Addr, unsigned char Value) override;
  unsigned char opZ80(unsigned short Addr) override;

  void run_frame()      override;
  void prepare_frame()  override;
  void render_row(short row) override;

  const unsigned short *logo(void) override;

private:
  // Render NATURALE (non trasposto): row_arcade=strip_r, col_arcade variabile.
  // Display SPINNERINO con MV rotation mostra il game ruotato 90° CW per l'utente
  // → vedrà arcade portrait nativo.
  void blit_tile_t(short strip_r, char col_arcade);

  // VRAM 4 KB × 2 pagine: page index = bit 0 di videoreg (write a 0x5000)
  // FG = vram[idx][0..0x3FF], BG = vram[idx][0x800..0xBFF]
  unsigned char vram[2][0x1000];

  unsigned char videoreg;        // bit 0 = page select, bit 1 = palette bank
  unsigned char scroll_x;        // BG horizontal scroll
  unsigned char palette_bank;    // bit 1 di videoreg

  // VBLANK polling (no IRQ): pilotato deterministicamente in 2 fasi dentro
  // run_frame (vblank_active=true → bit 7 = 0; false → bit 7 = 1).
  bool vblank_active = false;

  // Pre-decoded tile pens (1 byte per pixel = pen 0..3).
  // Layout [code][py][px] = 256 × 8 × 8 = 16 KB per layer = 32 KB total.
  const unsigned char *bg_decoded;     // 16 KB
  const unsigned char *fg_decoded;     // 16 KB
  const unsigned short *palette_cache; // 256 colori x 2 byte
  bool            cache_done;
};

#endif
