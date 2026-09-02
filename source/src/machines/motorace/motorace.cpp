// ============================================================================
// SPINNERINO P4 - machines/motorace/motorace.cpp
//
// MotoRace USA (Irem 1983)
// CPU: Z80 @ 3.072MHz main + M6803 @ 894KHz sound + 2x AY-3-8910
// Schermo arcade ROT270 (cabinet portrait), tile 8x8 3bpp, sprite 16x16, 64x32
// scrollable tilemap.
//
// PIPELINE RENDERING (trasposto inline, no buffer intermedio):
//   Il sorgente Galagino V3 originale scrive in framebuffer 224x288 portrait
//   con coords gia' ROT270-applicate. SPINNERINO ha framebuffer 256x224
//   landscape -> serve trasposizione. Versione ottimizzata: scriviamo
//   direttamente nel frame_buffer landscape SPINNERINO senza buffer
//   intermedio, calcolando inline le coords arcade per ogni pixel.
//
// Mapping inline:
//   per render_row(strip_r), strip_r ∈ [0..27]:
//     portrait_x = 223 - (strip_r*8 + sr)   [sr=0..7 strip-local row]
//     fb_x ∈ [0..255]:  portrait_y = fb_x + ARCADE_Y_OFFSET (16)
//     landscape_x_arcade = 263 - portrait_y
//     landscape_y_arcade = portrait_x + 16
//   Scrittura: frame_buffer[sr * 256 + fb_x] = pixel
//
// ============================================================================
#include "motorace.h"
#include "../../emulation/input.h"

#ifdef ENABLE_MOTORACE

#define FB_W            256
#define ARCADE_Y_OFFSET 16  // shift portrait_y -> fb_x per centrare il game

// ── Z80 instruction fetch ──
unsigned char motorace::opZ80(unsigned short Addr) {
  if (Addr < 0x8000)
    return motorace_rom[Addr];
  return 0xFF;
}

// ── Z80 memory read ──
unsigned char motorace::rdZ80(unsigned short Addr) {
  if (Addr < 0x8000)
    return motorace_rom[Addr];

  if (Addr >= 0x8000 && Addr <= 0x8FFF)
    return memory[MR_MEM_VRAM + (Addr - 0x8000)];

  if (Addr >= 0xC800 && Addr <= 0xC9FF)
    return memory[MR_MEM_SPRITES + (Addr - 0xC800)];

  if (Addr == 0xD000) {
    unsigned char keymask = input->buttons_get();
    unsigned char val = 0xFF;
    if (keymask & BUTTON_START) val &= ~0x01;
    if (keymask & BUTTON_COIN)  val &= ~0x08;
    return val;
  }
  if (Addr == 0xD001) {
    unsigned char keymask = input->buttons_get();
    unsigned char val = 0xFF;
    if (keymask & BUTTON_RIGHT) val &= ~0x01;
    if (keymask & BUTTON_LEFT)  val &= ~0x02;
    if (keymask & BUTTON_FIRE)  val &= ~0x20;  // accelerate
    if (keymask & BUTTON_DOWN)  val &= ~0x80;  // brake
    return val;
  }
  if (Addr == 0xD002) return 0xFF;
  if (Addr == 0xD003) return MOTORACE_DSW1;
  if (Addr == 0xD004) return MOTORACE_DSW2;

  if (Addr >= 0xE000 && Addr <= 0xEFFF)
    return memory[MR_MEM_WORKRAM + (Addr - 0xE000)];

  return 0xFF;
}

// ── Z80 memory write ──
void motorace::wrZ80(unsigned short Addr, unsigned char Value) {
  if (Addr >= 0x8000 && Addr <= 0x8FFF) {
    memory[MR_MEM_VRAM + (Addr - 0x8000)] = Value;
    return;
  }
  if (Addr == 0x9000) { scroll_x_low  = Value; return; }
  if (Addr == 0xA000) { scroll_x_high = Value; return; }

  if (Addr >= 0xC800 && Addr <= 0xC9FF) {
    memory[MR_MEM_SPRITES + (Addr - 0xC800)] = Value;
    return;
  }

  if (Addr == 0xD000) {
    sound_cmd = Value;
    if ((Value & 0x80) == 0)
      m6803_irq(&snd_cpu);
    return;
  }
  if (Addr == 0xD001) {
    flipscreen = Value & 1;
    if (!game_started && (Value & 0x02)) game_started = 1;
    return;
  }
  if (Addr >= 0xE000 && Addr <= 0xEFFF) {
    memory[MR_MEM_WORKRAM + (Addr - 0xE000)] = Value;
    return;
  }
}

// ============================================================================
// M6803 Sound CPU (Irem M52 sound board)
// ============================================================================
motorace *g_motorace_instance = nullptr;

static uint8_t motorace_snd_read(uint16_t addr)            { return g_motorace_instance->snd_read(addr); }
static void    motorace_snd_write(uint16_t addr, uint8_t v){ g_motorace_instance->snd_write(addr, v); }
static uint8_t motorace_snd_port_read(uint8_t port)        { return g_motorace_instance->snd_port_read(port); }
static void    motorace_snd_port_write(uint8_t port, uint8_t v){ g_motorace_instance->snd_port_write(port, v); }

void motorace::init(Input *inp, unsigned short *fb, sprite_S *sb, unsigned char *mb) {
  machineBase::init(inp, fb, sb, mb);
  g_motorace_instance = this;
}

void motorace::reset() {
  machineBase::reset();
  scroll_x_low = 0;
  scroll_x_high = 0;
  flipscreen = 0;
  sound_cmd = 0;
  snd_port1 = 0;
  snd_port2 = 0;
  memset(ay_addr, 0, sizeof(ay_addr));
  memset(ay_regs, 0, sizeof(ay_regs));

  g_motorace_instance = this;
  m6803_ext_read_fn   = motorace_snd_read;
  m6803_ext_write_fn  = motorace_snd_write;
  m6803_port_read_fn  = motorace_snd_port_read;
  m6803_port_write_fn = motorace_snd_port_write;

  m6803_reset(&snd_cpu);
  snd_cpu.irq_pending = 1;  // MAME: IRQ asserted at reset
}

uint8_t motorace::snd_read(uint16_t addr) {
  if (addr == 0x0800) return sound_cmd;
  if (addr >= 0xC000)
    return motorace_snd_rom[addr - 0xC000];
  return 0xFF;
}

void motorace::snd_write(uint16_t addr, uint8_t val) {
  if (addr == 0x0800) {
    if ((sound_cmd & 0x80) != 0) snd_cpu.irq_pending = 0;
    return;
  }
  // ADPCM (MSM5205, non emulato)
}

uint8_t motorace::snd_port_read(uint8_t port) {
  if (port == 1) {
    if (snd_port2 & 0x08) {
      unsigned char reg = ay_addr[0] & 0x0F;
      if (reg == 14) return sound_cmd;
      return ay_regs[0][reg];
    }
    if (snd_port2 & 0x10)
      return ay_regs[1][ay_addr[1] & 0x0F];
    return 0xFF;
  }
  if (port == 2) return 0x00;
  return 0xFF;
}

void motorace::snd_port_write(uint8_t port, uint8_t val) {
  if (port == 1) { snd_port1 = val; return; }
  if (port == 2) {
    if ((snd_port2 & 0x01) && !(val & 0x01)) {
      if (snd_port2 & 0x04) {
        if (snd_port2 & 0x08) ay_addr[0] = snd_port1 & 0x0F;
        if (snd_port2 & 0x10) ay_addr[1] = snd_port1 & 0x0F;
      } else {
        if (snd_port2 & 0x08) {
          unsigned char reg = ay_addr[0] & 0x0F;
          ay_regs[0][reg] = snd_port1;
          if (reg < 14) soundregs[reg] = snd_port1;
        }
        if (snd_port2 & 0x10) {
          unsigned char reg = ay_addr[1] & 0x0F;
          ay_regs[1][reg] = snd_port1;
          if (reg < 14) soundregs[16 + reg] = snd_port1;
        }
      }
      m6803_internal_ram[0x3D] = 1;
    }
    snd_port2 = val;
    return;
  }
}

// ── Z80 + M6803 frame execution ──
void motorace::run_frame(void) {
  current_cpu = 0;
  for (int i = 0; i < INST_PER_FRAME; i++) {
    StepZ80(&cpu[0]); StepZ80(&cpu[0]);
    StepZ80(&cpu[0]); StepZ80(&cpu[0]);
    if (i == INST_PER_FRAME - 1)
      IntZ80(&cpu[0], INT_IRQ);
  }

  // M6803 batch: ~14900 cycles per frame (894 KHz / 60 fps)
  static int nmi_cycle_cnt = 0;
  int snd_budget = 14900;
  while (snd_budget > 0) {
    int cyc = m6803_step(&snd_cpu);
    snd_budget -= cyc;
    nmi_cycle_cnt += cyc;
    if (nmi_cycle_cnt >= 224) {
      nmi_cycle_cnt -= 224;
      m6803_nmi(&snd_cpu);
    }
    uint16_t old_tc = snd_cpu.timer_counter;
    snd_cpu.timer_counter += cyc;
    if (snd_cpu.timer_counter < old_tc) snd_cpu.tcsr |= 0x20;
    if (old_tc <= snd_cpu.timer_output_compare &&
        snd_cpu.timer_counter >= snd_cpu.timer_output_compare)
      snd_cpu.tcsr |= 0x40;
  }
}

// ── Sprite preparation (identico al sorgente, scrive in sprite[]) ──
void motorace::prepare_frame(void) {
  active_sprites = 0;
  for (int offs = 0x1FF; offs >= 0; offs -= 4) {
    unsigned char *spr_base = &memory[MR_MEM_SPRITES + offs - 3];

    unsigned char sy_raw = spr_base[0];
    unsigned char attr   = spr_base[1];
    unsigned char code   = spr_base[2];
    unsigned char sx_raw = spr_base[3];

    if (sy_raw == 0 && sx_raw == 0) continue;

    struct sprite_S spr;
    spr.code  = code;
    spr.color = attr & 0x0F;
    bool flipx = (attr & 0x40) != 0;
    bool flipy = (attr & 0x80) != 0;

    int sx = ((sx_raw + 8) & 0xFF) - 8;
    int sy = 240 - sy_raw;
    if (sy > 191) continue;

    spr.x = sy - 16;
    spr.y = 263 - sx - 15;
    spr.flags = (flipy ? 1 : 0) | (flipx ? 2 : 0);

    if ((spr.y > -16) && (spr.y < 288) &&
        (spr.x > -16) && (spr.x < 224)) {
      sprite[active_sprites++] = spr;
    }
    if (active_sprites >= 124) break;
  }
}

// ============================================================================
// Render BG scroll trasposto inline (zero buffer intermedio).
// Per ogni strip_r ∈ [0..27], copre 8 valori di portrait_x (= 8 fb_y) e
// tutti 256 fb_x. Ottimizzazione: per ogni fb_y_strip (= sr) lx_arcade resta
// costante, quindi scroll_x e tile_col cambiano solo in funzione di fb_x.
// ============================================================================
void motorace::blit_scroll_strip_t(short strip_r) {
  int scroll = scroll_x_low + (scroll_x_high << 8);

  for (int sr = 0; sr < 8; sr++) {
    int portrait_x = 223 - (strip_r * 8 + sr);
    if (portrait_x < 0 || portrait_x >= 224) continue;

    // landscape_y arcade fissato per la riga corrente
    int ly = portrait_x + 16;       // 16..239
    int tile_row = (ly >> 3) & 31;
    int rom_row_base = ly & 7;      // 0..7 (riga dentro al tile)

    bool scroll_active = (ly < 192);

    unsigned short *fb_dst = frame_buffer + sr * FB_W;

    for (int fb_x = 0; fb_x < FB_W; fb_x++) {
      int portrait_y = fb_x + ARCADE_Y_OFFSET;  // 16..271
      if (portrait_y < 0 || portrait_y >= 288) {
        fb_dst[fb_x] = 0;
        continue;
      }

      int lx = 263 - portrait_y;                // -8..247
      int lx_scrolled = scroll_active ? (lx + scroll) : lx;

      int tile_col = (lx_scrolled >> 3) & 63;
      int tile_index = tile_row * 64 + tile_col;

      unsigned char tile_code_lo = memory[MR_MEM_VRAM + 2 * tile_index];
      unsigned char tile_attr    = memory[MR_MEM_VRAM + 2 * tile_index + 1];

      int tile_code = tile_code_lo + ((tile_attr & 0xC0) << 2);
      int palette = tile_attr & 0x0F;
      int flip_x = (tile_attr >> 5) & 1;
      int flip_y = (tile_attr >> 4) & 1;

      int rom_row = flip_y ? (7 - rom_row_base) : rom_row_base;
      int pixel_col = lx_scrolled & 7;
      if (flip_x) pixel_col = 7 - pixel_col;

      unsigned long tile_row_data = motorace_tilemap[tile_code][rom_row];
      unsigned char pixel = (tile_row_data >> (pixel_col * 4)) & 7;

      fb_dst[fb_x] = motorace_char_cmap[palette][pixel];
    }
  }
}

// ============================================================================
// Sprite trasposto: rendering nella strip se sprite tocca portrait_x range.
// Sprite portrait coords: x ∈ [spr.x .. spr.x+15], y ∈ [spr.y .. spr.y+15]
// Strip portrait_x range: [216-strip_r*8 .. 223-strip_r*8] (8 valori, sr-mapped)
// ============================================================================
void motorace::blit_sprite_t(short strip_r, unsigned char s) {
  int spr_x = sprite[s].x;
  int spr_y = sprite[s].y;

  int strip_pX_lo = 216 - strip_r * 8;
  int strip_pX_hi = strip_pX_lo + 7;

  // r ∈ [0..15]: offset dentro al sprite (portrait_x direction)
  int r_min = strip_pX_lo - spr_x;
  int r_max = strip_pX_hi - spr_x;
  if (r_min < 0)  r_min = 0;
  if (r_max > 15) r_max = 15;
  if (r_min > r_max) return;

  int orientation = sprite[s].flags & 3;
  const unsigned long  *spr_data = motorace_spritemap[orientation][sprite[s].code];
  const unsigned short *colors   = motorace_spr_cmap[sprite[s].color];

  for (int r = r_min; r <= r_max; r++) {
    int portrait_x = spr_x + r;
    int sr = 223 - portrait_x - strip_r * 8;  // 0..7

    unsigned long row_lo = spr_data[r * 2];
    unsigned long row_hi = spr_data[r * 2 + 1];

    unsigned short *fb_dst = frame_buffer + sr * FB_W;

    for (int c = 0; c < 16; c++) {
      int portrait_y = spr_y + 15 - c;
      int fb_x = portrait_y - ARCADE_Y_OFFSET;
      if (fb_x < 0 || fb_x >= FB_W) continue;

      unsigned char px = (c < 8)
                       ? ((row_lo >> (c * 4)) & 7)
                       : ((row_hi >> ((c - 8) * 4)) & 7);

      if (px) {
        fb_dst[fb_x] = colors[px];
      }
    }
  }
}

void motorace::render_row(short strip_r) {
  if (strip_r < 0 || strip_r >= 28) return;

  // BG scroll (riempie tutta la strip 256x8)
  blit_scroll_strip_t(strip_r);

  // Sprite: filtraggio veloce per strip portrait_x range
  int strip_pX_lo = 216 - strip_r * 8;
  int strip_pX_hi = strip_pX_lo + 7;
  for (unsigned char s = 0; s < active_sprites; s++) {
    int spr_x = sprite[s].x;
    if (((spr_x + 15) >= strip_pX_lo) && (spr_x <= strip_pX_hi)) {
      blit_sprite_t(strip_r, s);
    }
  }
}

const unsigned short *motorace::logo(void) {
  return motorace_logo;
}

#endif // ENABLE_MOTORACE
