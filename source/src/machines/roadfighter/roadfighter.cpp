// ============================================================================
// roadfighter.cpp — Road Fighter (Konami 1984) port SPINNERINO
// FASE 2: CPU M6809 (KONAMI-1) + memory map roadf + audio Z80 + run_frame.
//   Modellato su CONSOLE_QUADRA_NANO/hyperolympic.cpp (stessa famiglia Konami).
//   Video (tilemap+sprite) e mix audio = FASI 3-4 (render_row ancora nero).
// ============================================================================
#include "roadfighter.h"

#ifdef ENABLE_ROADFIGHTER

/*
// ---- callback bus M6809 (puntatori a funzione globali della destinazione) ----
static uint8_t roadf_m6809_read(uint16_t addr) {
  return g_roadfighter_instance ? g_roadfighter_instance->main_read(addr) : 0xFF;
}
static void roadf_m6809_write(uint16_t addr, uint8_t val) {
  if (g_roadfighter_instance) g_roadfighter_instance->main_write(addr, val);
}
static uint8_t roadf_m6809_read_opcode(uint16_t addr) {
  return g_roadfighter_instance ? g_roadfighter_instance->main_read_opcode(addr) : 0xFF;
}

static void roadf_set_m6809_callbacks() {
  m6809_read_fn   = roadf_m6809_read;          // dati/operandi -> RAW
  m6809_write_fn  = roadf_m6809_write;
  m6809_opcode_fn = roadf_m6809_read_opcode;   // opcode-fetch -> DECRYPTED (KONAMI-1)
}
*/

void roadfighter::init(Input *input, unsigned short *framebuffer,
                       sprite_S *spritebuffer, unsigned char *memorybuffer) {
  machineBase::init(input, framebuffer, spritebuffer, memorybuffer);
  //g_roadfighter_instance = this;
  //roadf_set_m6809_callbacks();
}

void roadfighter::reset() {
  machineBase::reset();
  //g_roadfighter_instance = this;
  //roadf_set_m6809_callbacks();

  memset(&main_cpu, 0, sizeof(main_cpu));
  memset(snd_ram, 0, sizeof(snd_ram));
  sound_latch = 0;
  sn_latch = 0;
  sn_latch_reg = 0;
  snd_icnt = 0;
  dac_sample = 0;
  irq_mask = 0;
  flip_screen = 0;
  coin_latch = coin_hold = 0;
  start_latch = start_hold = 0;
  gear_state = 0;
  prev_fire_btn = 0;

  current_cpu = 0;
  ResetZ80(&cpu[0]);          // audio Z80
  m6809_reset(&main_cpu);     // legge reset vector $FFFE (via main_read -> ROM raw)
}

// ============================================================
// Main KONAMI-1 (M6809) memory map
// ============================================================

uint8_t roadfighter::m6809_read(m6809_state *s, uint16_t addr) {
  if (addr >= 0x4000)
    return roadfighter_rom_main_raw[addr - 0x4000];

  // I/O reads (PRIMA del catch-all RAM, altrimenti il gioco legge RAM stantia)
  if (addr == 0x1600) return ROADF_DSW2;
  if (addr >= 0x1680 && addr <= 0x1683) {
    switch (addr & 0x03) {
      case 0: return input_system();
      case 1: return input_p1();
      // P2: bit6 = ACTIVE_HIGH (idle=0), tutti gli altri ACTIVE_LOW (idle=1) -> 0xBF.
      // CRITICO: con 0xFF (bit6=1) il gioco fa il check NVRAM a $3F08 vs firma ROM
      // $96E6 (che fallisce, NVRAM vuota) e va in HANG infinito a $57A5 (sub-state 4).
      case 2: return 0xBF;
      case 3: return ROADF_DSW1;
    }
  }

  // RAM: sprite/scroll $1000-$10FF + VRAM/CRAM/work/NVRAM $2000-$3FFF
  return memory[addr];
}

uint8_t roadfighter::m6809_read_opcode(m6809_state *s, uint16_t addr) {
  if (addr >= 0x4000)
    return roadfighter_rom_main_decrypted[addr - 0x4000];
  return m6809_read(s, addr);
}

void roadfighter::m6809_write(m6809_state *s, uint16_t addr, uint8_t val) {
  if (addr >= 0x4000) return;        // ROM: ignora

  if (addr == 0x1400) return;        // watchdog reset: no-op

  if (addr >= 0x1480 && addr <= 0x1487) {
    // LS259 mainlatch: bit0 di val -> Q[addr&7]
    int bit = val & 1;
    switch (addr & 0x07) {
      case 0: flip_screen = bit; break;                 // Q0
      case 1: if (bit) IntZ80(&cpu[0], INT_RST38); break; // Q1 sound-on -> IRQ audio Z80
      case 3: /* coin counter 1 */ break;               // Q3
      case 4: /* coin counter 2 */ break;               // Q4
      case 7:                                            // Q7 = irq_mask main
        irq_mask = bit;
        if (!irq_mask) main_cpu.irq_pending = 0;
        break;
      default: break;
    }
    return;
  }

  if (addr == 0x1500) { sound_latch = val; return; }

  // RAM
  memory[addr] = val;
  if (addr >= ROADF_VRAM_OFF && addr < 0x3000) game_started = 1;
}

// ============================================================
// Inputs (FASE 5 = sterzo EC11 fine + tarature). Per ora base:
//   $1680 system: bit0 COIN1, bit3 START1 (ACTIVE_LOW)
//   $1681 P1: bit0-3 joystick sx/dx, bit4 BUTTON1 (LOW gear), bit5 BUTTON2 (HIGH gear)
// ============================================================

unsigned char roadfighter::input_system() {
  unsigned char keymask = input->buttons_get();
  unsigned char val = 0xFF;

  if ((keymask & BUTTON_COIN) && !coin_latch) { coin_latch = 1; coin_hold = 45; }
  if (!(keymask & BUTTON_COIN)) coin_latch = 0;

  if ((keymask & BUTTON_START) && !start_latch) { start_latch = 1; start_hold = 45; }
  if (!(keymask & BUTTON_START)) start_latch = 0;

  if (coin_hold)  val &= ~0x01;   // COIN1
  if (start_hold) val &= ~0x08;   // START1
  return val;
}

unsigned char roadfighter::input_p1() {
  unsigned char keymask = input->buttons_get();
  unsigned char val = 0xFF;
  // Mapping provvisorio (rifinito in FASE 5 con EC11->sterzo digitale sx/dx).
  if (keymask & BUTTON_LEFT)  val &= ~0x01;   // EC11 INVERTITO -> sterzo destra
  if (keymask & BUTTON_RIGHT) val &= ~0x02;   // EC11 INVERTITO -> sterzo sinistra
  // Acceleratore a TOGGLE (stato calcolato in run_frame):
  //   gear_state 0 = marcia LENTA (BUTTON1), 1 = marcia VELOCE (BUTTON2)
  if (gear_state == 0) val &= ~0x10;          // BUTTON1 = marcia lenta
  else                 val &= ~0x20;          // BUTTON2 = marcia veloce
  return val;
}

// ============================================================
// Audio Z80 map (roadf sound_map):
//   $0000-$3FFF ROM (8 KB mirror)  $4000-$4FFF RAM
//   $6000 sound latch read         $8000 timer (trackfld_audio)
//   $E000 DAC write   $E001 SN76489A latch   $E002 SN76489A strobe
// ============================================================

unsigned char roadfighter::opZ80(unsigned short Addr) { return rdZ80(Addr); }

unsigned char roadfighter::rdZ80(unsigned short Addr) {
  if (Addr < 0x4000)
    return roadfighter_rom_audio[Addr & 0x1FFF];
  if ((Addr & 0xF000) == 0x4000)
    return snd_ram[Addr & 0x0FFF];
  if (Addr == 0x6000)
    return sound_latch;
  if (Addr == 0x8000)
    return (snd_icnt >> 8) & 0x0F;     // timer (approssimato come hyperolympic)
  return 0xFF;
}

void roadfighter::wrZ80(unsigned short Addr, unsigned char Value) {
  if ((Addr & 0xF000) == 0x4000) { snd_ram[Addr & 0x0FFF] = Value; return; }
  if (Addr == 0xE000) { dac_sample = (int)Value - 128; return; }  // DAC (mix in FASE 4)
  if (Addr == 0xE001) { sn_latch = Value; return; }               // SN76489 data latch
  if (Addr == 0xE002) { sn76489_write(sn_latch); return; }        // SN76489 strobe
}

unsigned char roadfighter::inZ80(unsigned short Port) { return 0xFF; }
void roadfighter::outZ80(unsigned short Port, unsigned char Value) { }

// SN76489 byte-stream -> framework sn_*[0] (mix vero in audio.cpp, FASE 4)
void roadfighter::sn76489_write(unsigned char data) {
  if (data & 0x80) {
    sn_latch_reg = (data >> 4) & 0x07;
    unsigned char val4 = data & 0x0F;
    int ch = sn_latch_reg >> 1;
    if (sn_latch_reg & 1) {
      sn_volume[0][ch] = val4; sn_min_volume[0][ch] = val4;
      if (val4 < 15) sn_hold[0][ch] = 6;
    } else {
      sn_period[0][ch] = (sn_period[0][ch] & 0x3F0) | val4;
    }
  } else {
    int ch = sn_latch_reg >> 1;
    if (sn_latch_reg & 1) {
      unsigned char val4 = data & 0x0F;
      sn_volume[0][ch] = val4; sn_min_volume[0][ch] = val4;
      if (val4 < 15) sn_hold[0][ch] = 6;
    } else {
      sn_period[0][ch] = (sn_period[0][ch] & 0x00F) | ((data & 0x3F) << 4);
    }
  }
}

// ============================================================
// Frame: M6809 @1.536MHz -> 25600 cicli/frame, Z80 audio interleaved.
// IRQ0 main a fine frame (vblank) se irq_mask.
// ============================================================

void roadfighter::run_frame(void) {
  // Acceleratore a TOGGLE su FIRE: ogni tap (premi+lascia il pomello EC11) alterna
  //   marcia LENTA <-> marcia VELOCE (sempre in accelerazione, niente folle).
  // Edge detection sul RILASCIO, 1 volta per frame (qui), cosi' non sfarfalla con
  // le letture multiple di $1681 fatte dalla CPU durante il frame.
  unsigned char fire_now = (input->buttons_get() & BUTTON_FIRE) ? 1 : 0;
  if (prev_fire_btn && !fire_now)
    gear_state ^= 1;
  prev_fire_btn = fire_now;

  static const int M6809_CYCLES_PER_FRAME = 25600;
  int m6809_cycles = 0;
  int safety = 0;
  const int SAFETY_LIMIT = 80000;

  for (int i = 0; i < 720; i++) {
    int slice_target = (M6809_CYCLES_PER_FRAME * (i + 1)) / 720;
    while (m6809_cycles < slice_target && safety < SAFETY_LIMIT) {
      int c = m6809_step(&main_cpu, 1);
      if (c <= 0) c = 1;
      m6809_cycles += c;
      safety++;
    }
    current_cpu = 0;
    // Audio Z80: piu' step/slice = audio piu' veloce (CPU sonora + timer $8000,
    // che deriva da snd_icnt). 20 = ~2x del 10 iniziale (10 troppo lento, 30 troppo veloce).
    for (int z = 0; z < 20; z++) { StepZ80(&cpu[0]); snd_icnt++; }
  }

  if (irq_mask)
    m6809_irq(&main_cpu);

  dbg_pc = main_cpu.PC;        // cattura PC per overlay (se bloccato = indirizzo del loop)

  if (coin_hold)  coin_hold--;
  if (start_hold) start_hold--;
}

// ============================================================
// Video — roadf (ROT90 = SPINNERINO → NESSUN flip software, render nativo).
// Tilemap 64x32, scroll PER-RIGA (32 righe), visarea y 16..239 (tile row +2).
//   tile code = vram | (cram&0x80)<<1 | (cram&0x60)<<4; color=cram&0x0f; flipx=cram&0x10
// Sprite (48, draw_sprites base_state): code=byte2+8*(flags&0x20), color=flags&0x0f,
//   flipx=~flags&0x40, flipy=flags&0x80, sx=byte3, sy=241-byte1, doppio draw a sx e sx-256.
// ============================================================

void roadfighter::prepare_frame(void) {
  memcpy(vram_snap,   memory + ROADF_VRAM_OFF,   sizeof(vram_snap));
  memcpy(cram_snap,   memory + ROADF_CRAM_OFF,   sizeof(cram_snap));
  memcpy(scroll_snap, memory + ROADF_SCROLL_OFF, sizeof(scroll_snap));
  memcpy(spr_snap,    memory + ROADF_SPRRAM_OFF, sizeof(spr_snap));

  // Popola sprite[] in coordinate buffer (y in 0..223 = game_y - 16).
  active_sprites = 0;
  for (int offs = 0xC0 - 4; offs >= 0 && active_sprites < 96; offs -= 4) {
    unsigned char flags  = spr_snap[offs + 0];
    unsigned char sy_raw = spr_snap[offs + 1];
    unsigned char code_l = spr_snap[offs + 2];
    unsigned char sx     = spr_snap[offs + 3];
    if ((flags | sy_raw | code_l | sx) == 0) continue;   // slot vuoto

    // flip_screen MAME: sy = 240-(240-sy_raw)=sy_raw, poi +1; flipy invertito. flipx invariato.
    int flipy = (flags & 0x80) ? 1 : 0;
    int sy;
    if (flip_screen) { sy = (int)sy_raw + 1; flipy = !flipy; }
    else             { sy = 241 - (int)sy_raw; }
    sprite_S &sp = sprite[active_sprites];
    sp.code  = (unsigned short)((code_l + 8 * (flags & 0x20)) & (ROADF_NSPRITES - 1));
    sp.color = flags & 0x0F;
    sp.flags = (((flags & 0x40) == 0) ? 1 : 0)        // flipx (~flags & 0x40), NON cambia con flip_screen
             | (flipy ? 2 : 0);
    sp.x = (short)sx;
    sp.y = (short)(sy - 16);                           // buffer coords
    // La tilemap (sfondo/banner/HUD percorso) e' corretta, ma gli sprite risultano
    // ruotati 180° rispetto ad essa (auto "verso il basso", HUD laterale/menu sprite
    // specchiati). Ruoto 180° SOLO gli sprite nel buffer (256x224).
    sp.x = (short)(ROADF_SCREEN_W - 16 - (int)sp.x);   // 256-16-x  (flip orizzontale)
    sp.y = (short)(ROADF_SCREEN_H - 16 - (int)sp.y);   // 224-16-y  (flip verticale)
    sp.flags ^= 0x03;                                  // inverti flipx + flipy
    active_sprites++;
  }
}

// Overlay di debug (P4 senza seriale): top 5 righe mostrano diagnostica.
//   riga 0: barra VERDE = #byte VRAM non-zero (0..2048 scalato a 256px)
//   riga 1: barra CIANO = #byte CRAM non-zero
//   riga 2: PC del M6809 (16 px, bianco = bit 1) — deve CAMBIARE se la CPU gira
//   riga 3: ROSSO se irq_mask=1, GIALLO se game_started
//   riga 4: nera (separatore)
// Disattivare mettendo a 0 dopo la diagnosi.
#define ROADF_DEBUG_OVERLAY 0

void roadfighter::render_row(short row) {
#if ROADF_DEBUG_OVERLAY
  if (row < 5) {
    int vnz = 0;
    for (int i = 0; i < 0x800; i++) if (vram_snap[i]) vnz++;
    unsigned short pc = dbg_pc;          // PC catturato STABILE in run_frame
    unsigned char hi = pc >> 8, lo = pc & 0xFF;
    for (int line = 0; line < 8; line++) {
      unsigned short *p = frame_buffer + line * ROADF_SCREEN_W;
      for (int x = 0; x < ROADF_SCREEN_W; x++) p[x] = 0x0000;
      if (row == 0) { int w = vnz >> 3; for (int x = 0; x < w && x < 256; x++) p[x] = 0xE007; }   // verde = VRAM
      // PC byte ALTO (riga 1) e BASSO (riga 2): 8 quadrati SEMPRE visibili,
      // bianco = bit 1, grigio scuro = bit 0 (MSB a sinistra/in alto).
      else if (row == 1) { for (int b = 0; b < 8; b++) { unsigned short c = (hi & (0x80 >> b)) ? 0xFFFF : 0x0841;
                           for (int x = 0; x < 28; x++) p[b * 32 + x] = c; } }
      else if (row == 2) { for (int b = 0; b < 8; b++) { unsigned short c = (lo & (0x80 >> b)) ? 0xFFFF : 0x0841;
                           for (int x = 0; x < 28; x++) p[b * 32 + x] = c; } }
      else if (row == 3) { unsigned short c = irq_mask ? 0x00F8 : 0x0000; for (int x = 0; x < 128; x++) p[x] = c;
                           unsigned short g = game_started ? 0xE0FF : 0x0000; for (int x = 128; x < 256; x++) p[x] = g; }
    }
    return;
  }
#endif
#define ROADF_TEST_TILES 0
#if ROADF_TEST_TILES
  // TEST: riempi lo schermo con tile consecutivi (IGNORA la VRAM) per validare
  // la pipeline di render. Se vedi una griglia di caratteri/grafica leggibile e
  // colorata -> render OK (e il viola del gioco e' contenuto VRAM, non bug mio).
  // Se vedi viola/garbage -> pipeline rotta. Mettere a 0 dopo la diagnosi.
  for (int line = 0; line < 8; line++) {
    unsigned short *ptr = frame_buffer + line * ROADF_SCREEN_W;
    for (int x = 0; x < ROADF_SCREEN_W; x++) {
      int tcol = x >> 3;
      int code = (row * 32 + tcol) % ROADF_NTILES;     // tile consecutivi
      const unsigned short *cm = roadfighter_tile_colormap[(tcol + 1) & 0x0F];
      uint32_t pr = roadfighter_tiles[code][line];
      unsigned char pix = (pr >> ((x & 7) * 4)) & 0x0F;
      ptr[x] = cm[pix];
    }
  }
  return;
#endif

  // Contenuto MAME-fedele (flip_screen: scroll negato + sprite sy/flipy gia' gestiti),
  // poi flip 180° GLOBALE dell'immagine composta (display montato ruotato): rendo la
  // strip speculare eff_row e inverto righe+colonne. Tile e sprite ruotano INSIEME.
  short eff_row = 27 - row;
  // --- Background tilemap (per-pixel con scroll per-riga) ---
  int tile_row = eff_row + 2;                          // visarea: tile row 2..29
  int srow = tile_row * 2;
  int scrollx = scroll_snap[srow] | ((scroll_snap[srow + 1] & 0x01) << 8);
  if (flip_screen) scrollx = -scrollx;                 // MAME: flip_screen nega lo scroll

  for (int line = 0; line < 8; line++) {
    unsigned short *ptr = frame_buffer + line * ROADF_SCREEN_W;
    int tile_r = line;                                 // game_y & 7 = line
    int prev_col = -1;
    uint32_t prow = 0;
    const unsigned short *cmap = nullptr;
    int flipx = 0;
    for (int x = 0; x < ROADF_SCREEN_W; x++) {
      int game_x   = (x + scrollx) & 0x1FF;            // tilemap 512 wide wrap
      int tile_col = (game_x >> 3) & 63;
      if (tile_col != prev_col) {                      // ricarica GFX 1x ogni 8 px
        int ti = (tile_row * 64 + tile_col) & 0x7FF;
        unsigned char v = vram_snap[ti];
        unsigned char a = cram_snap[ti];
        unsigned int code = (unsigned)v | (((unsigned)a & 0x80) << 1) | (((unsigned)a & 0x60) << 4);
        if (code >= ROADF_NTILES) code %= ROADF_NTILES;
        prow  = roadfighter_tiles[code][tile_r];
        cmap  = roadfighter_tile_colormap[a & 0x0F];
        flipx = a & 0x10;
        prev_col = tile_col;
      }
      int tile_c = game_x & 7;
      int src_c  = flipx ? (7 - tile_c) : tile_c;
      unsigned char pix = (prow >> (src_c * 4)) & 0x0F;
      ptr[x] = cmap[pix];
    }
  }

  // --- Sprites --- (gia' ruotati 180° in prepare_frame; blittati su eff_row)
  for (unsigned char s = 0; s < active_sprites; s++)
    blit_sprite_strip(eff_row, s);

  // Reverse 8 righe × 256 col = completa il flip 180° globale del display.
  for (int r0 = 0; r0 < 4; r0++) {
    int r1 = 7 - r0;
    unsigned short *p0 = frame_buffer + r0 * ROADF_SCREEN_W;
    unsigned short *p1 = frame_buffer + r1 * ROADF_SCREEN_W;
    for (int c = 0; c < ROADF_SCREEN_W / 2; c++) {
      unsigned short a = p0[c], b = p0[ROADF_SCREEN_W - 1 - c];
      unsigned short cc = p1[c], d = p1[ROADF_SCREEN_W - 1 - c];
      p0[c] = d; p0[ROADF_SCREEN_W - 1 - c] = cc;
      p1[c] = b; p1[ROADF_SCREEN_W - 1 - c] = a;
    }
  }
}

void roadfighter::blit_sprite_strip(short row, unsigned char s) {
  sprite_S &sp = sprite[s];
  short top = row * 8;
  if (sp.y + 16 <= top || sp.y >= top + 8) return;     // sprite non in questo strip

  int code = sp.code;
  int flipx = sp.flags & 1;
  int flipy = sp.flags & 2;
  const uint32_t *gfx = roadfighter_sprites[code];

  for (int dy = 0; dy < 16; dy++) {
    int y_buf = sp.y + dy;
    int strip_y = y_buf - top;
    if (strip_y < 0 || strip_y >= 8) continue;
    int sy_src = flipy ? (15 - dy) : dy;
    unsigned short *ptr = frame_buffer + strip_y * ROADF_SCREEN_W;

    for (int dx = 0; dx < 16; dx++) {
      int sx_src = flipx ? (15 - dx) : dx;
      uint32_t gw = gfx[sy_src * 2 + (sx_src >= 8 ? 1 : 0)];
      unsigned char pix = (gw >> ((sx_src & 7) * 4)) & 0x0F;
      if (!pix) continue;                              // pen 0 = trasparente
      unsigned short c = roadfighter_sprite_colormap[sp.color][pix];
      int x0 = sp.x + dx;
      if (x0 >= 0 && x0 < ROADF_SCREEN_W) ptr[x0] = c;
      int x1 = sp.x - 256 + dx;                         // wrap (2o draw MAME)
      if (x1 >= 0 && x1 < ROADF_SCREEN_W) ptr[x1] = c;
    }
  }
}

#endif // ENABLE_ROADFIGHTER
