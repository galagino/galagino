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
//                    (resto byte usati come work RAM)
//   0x5000-0x57FF  videoreg_w  (bit 0=page sel, bit 1=palette bank, cocktail)
//   0x5800-0x5FFF  scroll_w    (BG horizontal scroll, 8-bit)
//   0x6000-0x67FF  sound A control (ignorato)
//   0x6800-0x6FFF  sound B control (ignorato)
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

protected:
  void blit_tile(short row, char col)          override { }
  void blit_sprite(short row, unsigned char s) override { }

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

  // VBLANK polling (no IRQ) — fase deterministica per-run_frame:
  // dentro run_frame eseguiamo Z80 in 2 fasi: vblank_active=false per
  // 84% dei loop (bit 7 = 1, display attivo) poi vblank_active=true per
  // 16% dei loop (bit 7 = 0, vblank). Garantisce 1 transizione 1→0
  // per ogni run_frame, visibile al polling Z80 deterministicamente.
  // Soluzione MOLTO piu' affidabile della precedente basata su micros()
  // che sul framework di SPINNERINO non sempre vedeva la transizione.
  bool vblank_active = false;

  // Spinner positional control: ship segue posizione paddle EC11.
  // paddle_committed = target stabile con histeresi anti-drift.
  short ship_polled_x = 128;
  short paddle_committed = 0;

  // Pre-decoded tile pens (1 byte per pixel = pen 0..3).
  // Layout [code][py][px] = 256 × 8 × 8 = 16 KB per layer = 32 KB total.
  unsigned char *bg_decoded;     // 16 KB
  unsigned char *fg_decoded;     // 16 KB

  // BG tilemap cached bitmap 256x208 RGB565 — UNO per page (2 pages totali).
  // Phoenix usa double-buffering VRAM (videoreg bit 0 switcha page). Senza
  // bitmap separati per page, il switch invalida tutto e si ricostruisce full
  // ogni frame -> blocco. Con 2 bitmap separati il switch e' istantaneo.
  unsigned short *bg_bitmap[2];     // 2 × 256 × 208 ≈ 213 KB
  unsigned char  bg_dirty[2][832];  // dirty per page
};

#endif
