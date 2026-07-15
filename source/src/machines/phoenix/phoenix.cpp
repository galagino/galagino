// ============================================================================
// SPINNERINO P4 - machines/phoenix/phoenix.cpp
//
// Phoenix (Amstar/Centuri 1980, set "phoenix" base MAME).
// CPU Z80 + 2-layer tilemap (FG+BG con scroll BG) + palette PROM 256 colori.
// NESSUN IRQ vblank: il game polla bit 7 di 0x7800 (DSW0) per sincronizzazione.
// Audio TMS3617 custom NON emulato → silenzio totale.
// ============================================================================
#include "phoenix.h"

#define FB_W            256
// Schermo Phoenix arcade landscape MAME: 256 wide × 208 tall
//   visarea HBSTART=256, HBEND=0, VBSTART=208, VBEND=0
// In user portrait (cabinet ROT90): 208 wide × 256 tall (32 col × 26 row arcade
// landscape = visualizzato come 26 col × 32 row user portrait).
// Render naturale (non trasposto): row_arcade = strip_r (∈ 0..25), col_arcade
// itera 0..31 (32 colonne arcade landscape = 256 px wide).
// Niente ARCADE_X_OFFSET: il game riempie tutta la larghezza fb (32×8=256).
#define ARCADE_COLS     32       // tile colonne arcade landscape (256 wide)
#define ARCADE_ROWS     26       // tile righe arcade landscape (208 tall)

phoenix::phoenix()
  : videoreg(0), scroll_x(0), palette_bank(0),
    rom_cache(nullptr), bgtiles_cache(nullptr), fgtiles_cache(nullptr),
    palette_cache(nullptr), cache_done(false),
    bg_decoded(nullptr), fg_decoded(nullptr) {
  // vblank_active inizializzato a false direttamente in phoenix.h.
  memset(vram, 0, sizeof(vram));
  memset(bg_dirty, 1, sizeof(bg_dirty));
  bg_bitmap[0] = nullptr;
  bg_bitmap[1] = nullptr;
}

// Pre-decode 256 tile × 8 rows × 8 cols → pen 2-bit packed in 1 byte.
// Chiamato dopo cache PROGMEM. ~32 KB DRAM extra.
static void decode_tile_pens(const unsigned char *gfx, unsigned char *out) {
  for (int code = 0; code < 256; code++) {
    for (int py = 0; py < 8; py++) {
      unsigned char p0 = gfx[code * 8 + py];
      unsigned char p1 = gfx[code * 8 + py + 0x800];
      for (int px = 0; px < 8; px++) {
        unsigned char pen = ((p0 >> px) & 1) | (((p1 >> px) & 1) << 1);
        out[(code << 6) | (py << 3) | px] = pen;
      }
    }
  }
}

void phoenix::init(Input *in, unsigned short *fb,
                   sprite_S *sb, unsigned char *mem) {
  machineBase::init(in, fb, sb, mem);
  // ATTENZIONE (fix bisect 2026-05-09): l'allocazione cache (~268 KB SRAM)
  // e' stata SPOSTATA da init() a reset(). init() viene chiamata al boot per
  // TUTTE le machine in machines[], indipendentemente da quale viene scelta.
  // Allocare 268 KB SRAM al boot saturava la SRAM interna e spingeva gli stack
  // dei FreeRTOS task (audio_task, emulation_task) in PSRAM, rallentando di
  // ~5x l'emulazione di tutti gli altri giochi (Arkanoid 2, Galaga, Motorace).
  // Ora alloca solo quando Phoenix viene effettivamente selezionato (reset()
  // = al primo avvio gioco).
}

// Helper: alloca le cache (~268 KB) — eseguito una volta sola al primo
// reset() di Phoenix (cioe' quando l'utente seleziona Phoenix dal menu).
static void phoenix_lazy_init_caches(unsigned char  **rom_cache,
                                     unsigned char  **bgtiles_cache,
                                     unsigned char  **fgtiles_cache,
                                     unsigned short **palette_cache,
                                     unsigned char  **bg_decoded,
                                     unsigned char  **fg_decoded,
                                     unsigned short **bg_bitmap0,
                                     unsigned short **bg_bitmap1,
                                     bool           *cache_done) {
  if (*cache_done) return;

  const uint32_t CAPS = MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT;
  *rom_cache     = (unsigned char*) heap_caps_malloc(16384, CAPS);
  *bgtiles_cache = (unsigned char*) heap_caps_malloc(4096,  CAPS);
  *fgtiles_cache = (unsigned char*) heap_caps_malloc(4096,  CAPS);
  *palette_cache = (unsigned short*)heap_caps_malloc(256 * sizeof(unsigned short), CAPS);
  *bg_decoded    = (unsigned char*) heap_caps_malloc(16384, CAPS);
  *fg_decoded    = (unsigned char*) heap_caps_malloc(16384, CAPS);
  *bg_bitmap0    = (unsigned short*)heap_caps_malloc(256 * 208 * 2, CAPS);
  *bg_bitmap1    = (unsigned short*)heap_caps_malloc(256 * 208 * 2, CAPS);

  if (*rom_cache && *bgtiles_cache && *fgtiles_cache && *palette_cache &&
      *bg_decoded && *fg_decoded && *bg_bitmap0 && *bg_bitmap1) {
    for (int i = 0; i < 16384; i++) (*rom_cache)[i]     = pgm_read_byte(&phoenix_rom[i]);
    for (int i = 0; i < 4096;  i++) (*bgtiles_cache)[i] = pgm_read_byte(&phoenix_bgtiles[i]);
    for (int i = 0; i < 4096;  i++) (*fgtiles_cache)[i] = pgm_read_byte(&phoenix_fgtiles[i]);
    for (int i = 0; i < 256;   i++) (*palette_cache)[i] = pgm_read_word(&phoenix_palette[i]);
    decode_tile_pens(*bgtiles_cache, *bg_decoded);
    decode_tile_pens(*fgtiles_cache, *fg_decoded);
    memset(*bg_bitmap0, 0, 256 * 208 * 2);
    memset(*bg_bitmap1, 0, 256 * 208 * 2);
    *cache_done = true;
    Serial.println(F("[PHOENIX] All cached in DRAM internal (~268 KB)"));
  } else {
    // Fallback PSRAM (slower, ma meglio di niente)
    const uint32_t PSCAPS = MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT;
    if (!*rom_cache)     *rom_cache     = (unsigned char*) heap_caps_malloc(16384, PSCAPS);
    if (!*bgtiles_cache) *bgtiles_cache = (unsigned char*) heap_caps_malloc(4096,  PSCAPS);
    if (!*fgtiles_cache) *fgtiles_cache = (unsigned char*) heap_caps_malloc(4096,  PSCAPS);
    if (!*palette_cache) *palette_cache = (unsigned short*)heap_caps_malloc(512,   PSCAPS);
    if (!*bg_decoded)    *bg_decoded    = (unsigned char*) heap_caps_malloc(16384, PSCAPS);
    if (!*fg_decoded)    *fg_decoded    = (unsigned char*) heap_caps_malloc(16384, PSCAPS);
    if (*rom_cache && *bgtiles_cache && *fgtiles_cache && *palette_cache &&
        *bg_decoded && *fg_decoded) {
      for (int i = 0; i < 16384; i++) (*rom_cache)[i]     = pgm_read_byte(&phoenix_rom[i]);
      for (int i = 0; i < 4096;  i++) (*bgtiles_cache)[i] = pgm_read_byte(&phoenix_bgtiles[i]);
      for (int i = 0; i < 4096;  i++) (*fgtiles_cache)[i] = pgm_read_byte(&phoenix_fgtiles[i]);
      for (int i = 0; i < 256;   i++) (*palette_cache)[i] = pgm_read_word(&phoenix_palette[i]);
      decode_tile_pens(*bgtiles_cache, *bg_decoded);
      decode_tile_pens(*fgtiles_cache, *fg_decoded);
      *cache_done = true;
      Serial.println(F("[PHOENIX] Cached in PSRAM (DRAM full!)"));
    }
  }
}

void phoenix::reset() {
  machineBase::reset();
  // Lazy alloc cache (~268 KB) — solo quando Phoenix selezionato dal menu.
  // Skippato se cache_done=true (gia' allocato da reset precedente).
  phoenix_lazy_init_caches(&rom_cache, &bgtiles_cache, &fgtiles_cache,
                           &palette_cache, &bg_decoded, &fg_decoded,
                           &bg_bitmap[0], &bg_bitmap[1], &cache_done);

  memset(vram, 0, sizeof(vram));
  videoreg = 0;
  scroll_x = 0;
  palette_bank = 0;
  vblank_active = false;       // fase iniziale = display attivo
  // Sincronizza paddle (legacy, residuo del vecchio EC11 spinner — non
  // piu' usato dopo il fix IN0 ACTIVE_LOW + joystick digital).
  memset(bg_dirty, 1, sizeof(bg_dirty));
}

const unsigned short *phoenix::logo(void) {
  return phoenix_logo;
}

// ── Z80 op fetch ──
unsigned char phoenix::opZ80(unsigned short Addr) {
  return rdZ80(Addr);
}

// ── Z80 memory read ──
unsigned char phoenix::rdZ80(unsigned short Addr) {
  // ROM 0x0000-0x3FFF (cached in DRAM)
  if (Addr < 0x4000) return rom_cache ? rom_cache[Addr]
                                       : pgm_read_byte(&phoenix_rom[Addr]);

  // VRAM 0x4000-0x4FFF (page corrente)
  if (Addr >= 0x4000 && Addr <= 0x4FFF) {
    unsigned char idx = videoreg & 0x01;
    return vram[idx][Addr & 0x0FFF];
  }

  // I/O area: 0x5000-0x6FFF write-only (read open bus)
  if (Addr >= 0x5000 && Addr <= 0x6FFF) return 0xFF;

  // IN0 read 0x7000-0x77FF
  // ── BUG ROOT FIX (2026-05-02): IN0 bit 4-7 sono ACTIVE LOW ──
  // Verificato MAME source phoenix_v.cpp player_input_r():
  //   return (ioport("CTRL")->read() & 0x0f) >> 0;
  // CTRL e' IP_ACTIVE_LOW → ioport->read() raw: 1 idle, 0 pressed.
  // Quindi IN0 bit 4-7 sono ACTIVE LOW: idle=1, pressed=0.
  // Vecchio code (ACTIVE HIGH idle=0) → game vedeva
  // BARRIER+RIGHT+LEFT+FIRE tutti pressed simultaneamente in idle →
  // priorita' interna sceglieva LEFT → DRIFT della nave a sinistra.
  if (Addr >= 0x7000 && Addr <= 0x77FF) {
    unsigned char b = input ? input->buttons_get() : 0;
    unsigned char v = 0xFF;     // idle = tutti i bit 1 (ACTIVE LOW)
    if (b & BUTTON_COIN)  v &= ~0x01;   // bit 0 COIN1
    if (b & BUTTON_START) v &= ~0x02;   // bit 1 START1
    // bit 2 START2, bit 3 UNUSED → restano 1
    // Mapping verificato empiricamente sul cabinet: FIRE bit 4, BARRIER bit 7
    // (MAME etichetta opposto ma il game tratta cosi').
    if (b & BUTTON_FIRE)  v &= ~0x10;   // bit 4 = SPARO
    // Display ROT90 CW: cabinet LEFT/RIGHT user invertiti vs MAME landscape
    if (b & BUTTON_LEFT)  v &= ~0x40;   // bit 6 = LEFT (MAME)
    if (b & BUTTON_RIGHT) v &= ~0x20;   // bit 5 = RIGHT (MAME)
    if (b & BUTTON_EXTRA) v &= ~0x80;   // bit 7 = SCUDO/BARRIER
    return v;
  }

  // DSW0 read 0x7800-0x7FFF + VBLANK live (bit 7 ACTIVE LOW).
  // VBLANK pilotato deterministicamente dalle 2 fasi di run_frame
  // (vblank_active=true → bit 7 = 0; false → bit 7 = 1).
  if (Addr >= 0x7800 && Addr <= 0x7FFF) {
    unsigned char vblank_bit = vblank_active ? 0x00 : 0x80;
    return (PHOENIX_DSW0 & 0x7F) | vblank_bit;
  }

  return 0xFF;
}

// ── Z80 memory write ──
void phoenix::wrZ80(unsigned short Addr, unsigned char Value) {
  // ROM 0x0000-0x3FFF: ignora
  if (Addr < 0x4000) return;

  // VRAM 0x4000-0x4FFF (page corrente). Mark dirty solo se BG tile cambia.
  if (Addr >= 0x4000 && Addr <= 0x4FFF) {
    unsigned char idx = videoreg & 0x01;
    unsigned int offset = Addr & 0x0FFF;
    if (vram[idx][offset] != Value) {
      vram[idx][offset] = Value;
      // BG tile dirty per la PAGE corrente (offset 0x800-0xB3F)
      if ((offset & 0x800) && (offset & 0x7FF) < 0x340) {
        bg_dirty[idx][offset & 0x3FF] = 1;
      }
    }
    if (!game_started) game_started = 1;
    return;
  }

  // videoreg_w 0x5000-0x57FF (mirror 0x400). NO invalidate al page switch
  // (ogni page ha il suo bitmap). Solo palette_bank cambio invalida entrambi.
  if (Addr >= 0x5000 && Addr <= 0x57FF) {
    unsigned char old_pb = palette_bank;
    videoreg = Value;
    palette_bank = (Value >> 1) & 0x01;
    if (old_pb != palette_bank) {
      memset(bg_dirty, 1, sizeof(bg_dirty));   // both pages
    }
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
  current_cpu = 0;
  // Fase display attivo (bit 7 = 1)
  vblank_active = false;
  for (int i = 0; i < PHOENIX_DISPLAY_PHASE; i++) {
    StepZ80(cpu); StepZ80(cpu); StepZ80(cpu); StepZ80(cpu);
  }
  // Fase vblank (bit 7 = 0) — il polling Z80 vede la transizione 1→0 qui
  vblank_active = true;
  for (int i = PHOENIX_DISPLAY_PHASE; i < PHOENIX_LOOPS_PER_FRAME; i++) {
    StepZ80(cpu); StepZ80(cpu); StepZ80(cpu); StepZ80(cpu);
  }
  // NESSUN IRQ — Phoenix usa solo polling VBLANK (vedi rdZ80 0x7800)
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
//   FG tile = vram[idx][tile_index]        + GFX fgtiles (gfx_set 1)
//   BG tile = vram[idx][tile_index + 0x800] + GFX bgtiles (gfx_set 0)
// ============================================================================

// Render BG con scroll horizontal (= scroll verticale per user portrait).
// Loop strutturato: per ogni col_tilemap (32), pre-fetch tile data (4 plane
// × 8 rows = 32 byte cache locale), poi scrive 8 pixel in cada strip-row.
void phoenix::blit_tile_t(short strip_r, char col_arcade) {
  // Helper non più usato come prima: tutto inline in render_row.
  (void)strip_r; (void)col_arcade;
}

void phoenix::render_row(short strip_r) {
  if (strip_r < 0 || strip_r >= 28) return;
  if (strip_r >= ARCADE_ROWS) {
    memset(frame_buffer, 0, 8 * FB_W * sizeof(unsigned short));
    return;
  }

  unsigned char idx = videoreg & 0x01;
  unsigned char *vp = vram[idx];

  // ──── BG layer (con scroll horizontal, decoded direct) ────
  for (int t = 0; t < 32; t++) {
    int tile_index = strip_r * 32 + t;
    unsigned char bg_code = vp[tile_index + 0x800];
    unsigned char bg_col = ((bg_code >> 5) & 0x07) | (palette_bank << 4);
    const unsigned short *bg_pal = &palette_cache[bg_col * 4];
    const unsigned char  *pens   = &bg_decoded[bg_code << 6];

    int fb_x_start = (t * 8 - scroll_x) & 0xFF;

    if (fb_x_start <= 248) {
      // Sequenziale (cache friendly)
      for (int sr = 0; sr < 8; sr++) {
        const unsigned char *rp = &pens[sr << 3];
        unsigned short *p = frame_buffer + sr * FB_W + fb_x_start;
        p[0] = bg_pal[rp[0]]; p[1] = bg_pal[rp[1]];
        p[2] = bg_pal[rp[2]]; p[3] = bg_pal[rp[3]];
        p[4] = bg_pal[rp[4]]; p[5] = bg_pal[rp[5]];
        p[6] = bg_pal[rp[6]]; p[7] = bg_pal[rp[7]];
      }
    } else {
      // Wrap split
      int n_left  = 256 - fb_x_start;
      for (int sr = 0; sr < 8; sr++) {
        const unsigned char *rp = &pens[sr << 3];
        unsigned short *line = frame_buffer + sr * FB_W;
        for (int px = 0; px < n_left; px++)
          line[fb_x_start + px] = bg_pal[rp[px]];
        for (int px = n_left; px < 8; px++)
          line[px - n_left] = bg_pal[rp[px]];
      }
    }
  }

  // ──── FG layer overlay (no scroll, pen 0 trasparente) ────
  for (int t = 0; t < 32; t++) {
    int tile_index = strip_r * 32 + t;
    unsigned char fg_code = vp[tile_index];
    unsigned char fg_col = ((fg_code >> 5) & 0x07) | 0x08 | (palette_bank << 4);
    const unsigned short *fg_pal = &palette_cache[fg_col * 4];
    const unsigned char  *pens   = &fg_decoded[fg_code << 6];

    int fb_x_base = t * 8;
    for (int sr = 0; sr < 8; sr++) {
      const unsigned char *rp = &pens[sr << 3];
      unsigned short *p = frame_buffer + sr * FB_W + fb_x_base;
      if (rp[0]) p[0] = fg_pal[rp[0]];
      if (rp[1]) p[1] = fg_pal[rp[1]];
      if (rp[2]) p[2] = fg_pal[rp[2]];
      if (rp[3]) p[3] = fg_pal[rp[3]];
      if (rp[4]) p[4] = fg_pal[rp[4]];
      if (rp[5]) p[5] = fg_pal[rp[5]];
      if (rp[6]) p[6] = fg_pal[rp[6]];
      if (rp[7]) p[7] = fg_pal[rp[7]];
    }
  }
}

