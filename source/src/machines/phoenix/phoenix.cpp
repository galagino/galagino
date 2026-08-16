// ============================================================================
// galagino - machines/phoenix/phoenix.cpp
//
// Phoenix (Amstar/Centuri 1980, set "phoenix" base MAME).
// CPU Z80 + 2-layer tilemap (FG+BG con scroll BG) + palette PROM 256 colori.
// NESSUN IRQ vblank: il game polla bit 7 di 0x7800 (DSW0) per sincronizzazione.
// Audio TMS3617 custom approssimato in Audio::phoenix_render_buffer() (emulation/audio.cpp).
//
// Monitor VERTICALE (ROT90). Arcade landscape MAME 256x208 (32 col x 26 row);
// reso qui in portrait 208x256 dentro il framebuffer galagino 224x288, con la
// rotazione 90 gradi cablata nella matematica dei tile (come galaxian).
// ============================================================================
#include "phoenix.h"

#define FB_W        224     // larghezza framebuffer galagino
#define PX_OFFSET   8       // (224-208)/2, centratura orizzontale portrait
#define ARCADE_COLS 26      // tile per riga portrait (208 px)

phoenix::phoenix() {
}

void phoenix::init(Input *in, unsigned short *fb,
                   sprite_S *sb, unsigned char *mem) {
  machineBase::init(in, fb, sb, mem);
  vram = (vram_t*)memory;
}

void phoenix::reset() {
  machineBase::reset();

  scroll_x = 0;
  video_page = 0;
  palette_bank = 0;
  vblank_active = false;       // fase iniziale = display attivo
}

const unsigned short *phoenix::logo(void) {
  return phoenix_logo;
}

unsigned char phoenix::opZ80(unsigned short Addr) {
  if (Addr < 0x4000)
    return phoenix_rom[Addr];

  return 0x00;
}

unsigned char phoenix::rdZ80(unsigned short Addr) {
  // ROM 0x0000-0x3FFF
  if (Addr < 0x4000)
    return phoenix_rom[Addr];

  // VRAM 0x4000-0x4FFF (page corrente)
  if (Addr >= 0x4000 && Addr <= 0x4FFF) {
    return (*vram)[video_page][Addr & 0x0FFF];
  }

  // I/O area: 0x5000-0x6FFF write-only (read open bus)
  if (Addr >= 0x5000 && Addr <= 0x6FFF) return 0xFF;

  // IN0 read 0x7000-0x77FF — ACTIVE LOW (idle=1, pressed=0), verificato su
  // MAME phoenix_v.cpp player_input_r(). Mapping bit determinato su cabinet:
  // bit0 COIN, bit1 START, bit4 FIRE, bit5 RIGHT, bit6 LEFT, bit7 BARRIER.
  if (Addr >= 0x7000 && Addr <= 0x77FF) {
    unsigned int  b = input->buttons_get();
    unsigned char v = 0xff;     // idle = all 1s (ACTIVE LOW)
    if (b & BUTTON_COIN)  v &= ~0x01;   // bit 0 COIN1
    if (b & BUTTON_START) v &= ~0x02;   // bit 1 START1
    if (b & BUTTON_LEFT)  v &= ~0x40;   // bit 6 = LEFT (MAME)
    if (b & BUTTON_RIGHT) v &= ~0x20;   // bit 5 = RIGHT (MAME)
    #ifdef GALAGINO_CONTROLLER
    if (b & BUTTON_A)     v &= ~0x10;   // bit 4 = FIRE
    if (b & BUTTON_X)     v &= ~0x10;   // bit 4 = FIRE
    if (b & BUTTON_B)     v &= ~0x80;   // bit 7 = SHIELD/BARRIER
    if (b & BUTTON_Y)     v &= ~0x80;   // bit 7 = SHIELD/BARRIER
    #else
    if (b & BUTTON_FIRE)  v &= ~0x10;   // bit 4 = FIRE
    if (b & BUTTON_UP)    v &= ~0x80;   // bit 7 = SHIELD/BARRIER
    if (b & BUTTON_DOWN)  v &= ~0x80;   // bit 7 = SHIELD/BARRIER
    #endif
    return v;
  }

  if (Addr >= 0x7800 && Addr <= 0x7fff) {
    unsigned char vblank_bit = vblank_active ? 0x00 : 0x80;
    return (PHOENIX_DSW0 & 0x7f) | vblank_bit;
  }

  return 0xff;
}

// ── Z80 memory write ──
void phoenix::wrZ80(unsigned short Addr, unsigned char Value) {
  if (Addr < 0x4000) return; // ROM: ignore

  // VRAM 0x4000-0x4FFF (page corrente)
  if (Addr <= 0x4FFF) {
    (*vram)[video_page][Addr & 0x0FFF] = Value;
    if (!game_started) game_started = 1;
    return;
  }

  // videoreg_w 0x5000-0x57FF (bit0 page sel, bit1 palette bank)
  if (Addr >= 0x5000 && Addr <= 0x57FF) {
    video_page = Value & 0x01;
    palette_bank = (Value >> 1) & 0x01;
    return;
  }

  // scroll_w 0x5800-0x5FFF
  if (Addr >= 0x5800 && Addr <= 0x5FFF) {
    scroll_x = Value;
    return;
  }

  // sound control A 0x6000-0x67FF (effect 2: shoot tone / wing)
  if (Addr >= 0x6000 && Addr <= 0x67FF) {
    soundregs[0] = Value;
    return;
  }

  // sound control B 0x6800-0x6FFF (effect 1: noise/explosion + melody)
  if (Addr >= 0x6800 && Addr <= 0x6FFF) {
    soundregs[1] = Value;
    return;
  }
}

// ── Frame loop ──
// Phoenix MASTER_CLOCK = 11 MHz, CPU_CLOCK = 5.5 MHz. 60 fps ≈ 92K cicli Z80.
// VBLANK pilotato in 2 fasi DENTRO run_frame:
//   - fase display: vblank_active=false (bit 7 = 1), 84% dei loop (~14 ms)
//   - fase vblank:  vblank_active=true  (bit 7 = 0), 16% dei loop (~2.6 ms)
// Garantisce 1 transizione 1→0 per ogni run_frame, visibile al polling Z80.
// Soluzione molto piu' affidabile della precedente basata su micros() che
// soffriva di timing race con il main loop.
#define PHOENIX_LOOPS_PER_FRAME  2500
#define PHOENIX_DISPLAY_PHASE    2100   // 84% di 2500

void phoenix::run_frame() {
  vblank_active = false;
  for (int i = 0; i < PHOENIX_LOOPS_PER_FRAME; i++) {
    if (i == PHOENIX_DISPLAY_PHASE)
      vblank_active = true;
    StepZ80(cpu); StepZ80(cpu); StepZ80(cpu); StepZ80(cpu);
  }
}

void phoenix::prepare_frame() {}

// ============================================================================
// Render NATURALE (non trasposto). Mapping arcade landscape 256x208 → fb
// SPINNERINO 256x224. Display con MV rotation mostra game in portrait 208x256.
//
// Pipeline ottimizzata:
//   1. BG layer renderizzato per primo CON scroll (m_bg_tilemap->set_scrollx).
//      Loop per col_arcade tilemap, posizione fb shiftata di -scroll_x con
//      wrap modulo 256.
//   2. FG layer overlay sopra SENZA scroll (pen 0 trasparente).
//
// MAME convention:
//   FG tile = vram[idx][tile_index]         + GFX fgtiles (gfx_set 1)
//   BG tile = vram[idx][tile_index + 0x800] + GFX bgtiles (gfx_set 0)
// ============================================================================

void phoenix::render_row(short row) {
  if (row < 2 || row > 33) return;

  unsigned char *vp = (*vram)[video_page];
  int prow = row - 2;                    // 0..31 = colonna arcade tx (FG)

  for (int ry = 0; ry < 8; ry++) {
    unsigned short *line = frame_buffer + ry * FB_W + PX_OFFSET;
    int py = (prow << 3) + ry;           // portrait Y globale (0..255) = fx MAME

    // ─── BG layer (opaco, scroll verticale) ───
    int fx = (py + scroll_x) & 0xFF;
    int bg_tx = fx >> 3;
    int bg_lx = fx & 7;
    for (int pcol = 0; pcol < ARCADE_COLS; pcol++) {
      int ty = 25 - pcol;
      unsigned char code = vp[(ty << 5) + bg_tx + 0x800];
      unsigned char col  = ((code >> 5) & 0x07) | (palette_bank << 4);
      const unsigned short *pal  = &phoenix_palette[col << 2];
      const unsigned char  *pens = &phoenix_bgtiles[(code << 6) + bg_lx];
      unsigned short *p = line + (pcol << 3);
      // rx 0..7 -> ly = 7-rx ; pen = pens[ly*8]
      p[0] = pal[pens[7 << 3]]; p[1] = pal[pens[6 << 3]];
      p[2] = pal[pens[5 << 3]]; p[3] = pal[pens[4 << 3]];
      p[4] = pal[pens[3 << 3]]; p[5] = pal[pens[2 << 3]];
      p[6] = pal[pens[1 << 3]]; p[7] = pal[pens[0]];
    }

    // ─── FG layer (overlay, pen 0 trasparente, no scroll) ───
    int fg_lx = ry;                      // fx = py -> lx = py&7 = ry ; tx = prow
    for (int pcol = 0; pcol < ARCADE_COLS; pcol++) {
      int ty = 25 - pcol;
      unsigned char code = vp[(ty << 5) + prow];
      unsigned char col  = ((code >> 5) & 0x07) | 0x08 | (palette_bank << 4);
      const unsigned short *pal  = &phoenix_palette[col << 2];
      const unsigned char  *pens = &phoenix_fgtiles[(code << 6) + fg_lx];
      unsigned short *p = line + (pcol << 3);
      unsigned char pen;
      pen = pens[7 << 3]; if (pen) p[0] = pal[pen];
      pen = pens[6 << 3]; if (pen) p[1] = pal[pen];
      pen = pens[5 << 3]; if (pen) p[2] = pal[pen];
      pen = pens[4 << 3]; if (pen) p[3] = pal[pen];
      pen = pens[3 << 3]; if (pen) p[4] = pal[pen];
      pen = pens[2 << 3]; if (pen) p[5] = pal[pen];
      pen = pens[1 << 3]; if (pen) p[6] = pal[pen];
      pen = pens[0];      if (pen) p[7] = pal[pen];
    }
  }
}

