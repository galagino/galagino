#include "zaxxon.h"
#ifdef ENABLE_ZAXXON
#include "zaxxon_dipswitches.h"
#include "zaxxon_sprite_geom.h"

// ZAXXON
// see https://github.com/mamedev/mame/blob/master/src/mame/sega/zaxxon.cpp
// https://github.com/mamedev/mame/blob/master/src/mame/sega/zaxxon_a.cpp
// https://github.com/mamedev/mame/blob/master/src/mame/sega/zaxxon_v.cpp
//
//  Zaxxon memory map
//
// 0000-1fff ROM 3
// 2000-3fff ROM 2
// 4000-4fff ROM 1
// 6000-67ff RAM 1
// 6800-6fff RAM 2
// 8000-83ff Video RAM
// a000-a0ff sprites
//
// read:
// c000      IN0
// c001      IN1
// c002      DSW0
// c003      DSW1
// c100      IN2
// see the input_ports definition below for details on the input bits
//
// write:
// c000      coin A enable
// c001      coin B enable
// c002      coin aux enable
// c003-c004 coin counters
// c006      flip screen
// ff3c-ff3f sound (see below)
// fff0      interrupt enable
// fff1      character color bank (not used during the game, but used in test
//           mode)
// fff8-fff9 background playfield position (11 bits)
// fffa      background color bank (0 = standard  1 = reddish)
// fffb      background enable
//
// interrupts:
// VBlank triggers IRQ, handled with interrupt mode 1
// NMI enters the test mode.
//
// Original resolution: 224 x 256 pixel (width x height)

static bool is_blank(const void *bitmap, uint16_t size) {
  const uint8_t *p = (const uint8_t *)bitmap;
  for (uint16_t i = 0; i < size; i++) {
    if (p[i]) {
      return false;
    }
  }
  return true;
}

zaxxon::zaxxon() {
  for (uint16_t c = 0; c < CHAR_CODES; c++) {
    char_blank[c] = is_blank(zaxxon_chartiles[c], sizeof(zaxxon_chartiles[c]));
  }
  for (uint8_t c = 0; c < SPRITE_CODES; c++) {
    sprite_blank[c] = is_blank(zaxxon_spritetiles[c], sizeof(zaxxon_spritetiles[c]));
  }
}

// memory is only valid after machineBase::init() runs, which happens after
// construction (see main.cpp) -- so these RAM pointers can't be set in the
// ctor (garbage `memory` there means garbage sprite_ram etc, crashing on
// first prepare_frame). start() runs post-init, same as vanguard::start().
void zaxxon::start(void) {
  work_ram = memory + WORK_RAM_OFFSET;
  video_ram = memory + VIDEORAM_OFFSET;
  sprite_ram = memory + SPRITERAM_OFFSET;
}

unsigned char zaxxon::opZ80(unsigned short Addr) {
  if (Addr < 0x5000)
    return zaxxon_rom_main[Addr];
  return 0xff;
}

unsigned char zaxxon::rdZ80(unsigned short Addr) {
  // ROM
  // 0000-1fff ROM 3
  // 2000-3fff ROM 2
  // 4000-4fff ROM 1
  if (Addr < 0x5000) {
    return zaxxon_rom_main[Addr];
  }

  // RAM
  // 6000-67ff RAM 1
  // 6800-6fff RAM 2
  if ((Addr & 0xf000) == 0x6000) {
    return work_ram[Addr - 0x6000];
  }
  // 8000-83ff Video RAM
  if ((Addr & 0xfc00) == 0x8000) {
    return video_ram[Addr - 0x8000];
  }
  // a000-a0ff sprites
  if ((Addr & 0xff00) == 0xa000) {
    return sprite_ram[Addr - 0xa000];
  }

  // INPUTS
  // c000      IN0 (active high: bit set = pressed)
  if (Addr == 0xc000) {
    const uint32_t keymask = input->buttons_get();
    uint8_t retval = 0x00;
    if (keymask & BUTTON_RIGHT) {
      retval |= 0x01;
    }
    if (keymask & BUTTON_LEFT) {
      retval |= 0x02;
    }
    if (keymask & BUTTON_DOWN) {
      retval |= 0x04;
    }
    if (keymask & BUTTON_UP) {
      retval |= 0x08;
    }
    if ((keymask & BUTTON_FIRE) || (keymask & BUTTON_L1)) {
      retval |= 0x10;
    } // ABXY  + L1 fire
    return retval;
  }
  // c001      IN1 -> inputs in "cocktail mode", not supported
  if (Addr == 0xc001) {
    return 0x00;
  }
  // c002      DSW0
  if (Addr == 0xc002) {
    return ZAXXON_DIP_A;
  }
  // c003      DSW1
  if (Addr == 0xc003) {
    return ZAXXON_DIP_B;
  }
  // c100      IN2 (active high: bit set = pressed)
  if (Addr == 0xc100) {
    const uint8_t keymask = input->buttons_get();
    uint8_t retval = 0x00;
    if ((keymask & BUTTON_COIN) && (last_coin == 0))
      retval |= 0x20; // coin status 1 (0x80 = service coin, unused)
    if (keymask & BUTTON_START)
      retval |= 0x04; // start 1
    last_coin = keymask & BUTTON_COIN;
    return retval;
  }

  return 0xff;
}

void zaxxon::wrZ80(unsigned short Addr, unsigned char Value) {
  // RAM
  // 6000-67ff RAM 1
  // 6800-6fff RAM 2
  if ((Addr & 0xf000) == 0x6000) {
    work_ram[Addr - 0x6000] = Value;
    return;
  }
  // 8000-83ff Video RAM
  if ((Addr & 0xfc00) == 0x8000) {
    video_ram[Addr - 0x8000] = Value;
    return;
  }
  // a000-a0ff sprites
  if ((Addr & 0xff00) == 0xa000) {
    sprite_ram[Addr - 0xa000] = Value;
    return;
  }

  // c000      coin A enable
  // c001      coin B enable
  // c002      coin aux enable
  // c003-c004 coin counters
  // c006      flip screen (cocktail mode)

  // i8255 PPI sound triggers (zaxxon_a.cpp zaxxon_sound_a/b/c_w). Real
  // device sits at e03c-e03f, mirrored across bit 0x1f00 -- 0xe03c|0x1f00
  // == 0xff3c, so this range is the SAME PPI, not a different one. Latch
  // the raw port byte in soundregs[0..2] (port A loops are level-decoded
  // by Audio::zaxxon_render_buffer()) and count one-shot falling edges
  // here, see ZAXXON_TRIG_BASE.
  if ((Addr & 0xfffc) == 0xff3c) {
    const uint8_t port = Addr & 3;
    if (port < 3) {
      const uint8_t fall = soundregs[port] & ~Value;
      if (port == 1) {
        if (fall & 0x10) { // S-Exp
          soundregs[ZAXXON_TRIG_BASE + 4]++;
        }
        if (fall & 0x20) { // M-Exp
          soundregs[ZAXXON_TRIG_BASE + 5]++;
        }
        if (fall & 0x80) { // Cannon
          soundregs[ZAXXON_TRIG_BASE + 6]++;
        }
      } else if (port == 2) {
        if (fall & 0x01) { // Shot
          soundregs[ZAXXON_TRIG_BASE + 7]++;
        }
        if (fall & 0x04) { // Alarm2
          soundregs[ZAXXON_TRIG_BASE + 8]++;
        }
        if (fall & 0x08) { // Alarm3
          soundregs[ZAXXON_TRIG_BASE + 9]++;
        }
      }
      soundregs[port] = Value;
    }
    return; // port 3 = 8255 control byte (mode set), nothing to latch
  }
  // fff0      interrupt enable
  if (Addr == 0xfff0) {
    irq_enable[0] = Value & 1;
    return;
  }
  // fff1      character color bank (not used during the game, but used in test
  // mode)
  if (Addr == 0xfff1) {
  }
  // fff8-fff9 background playfield position, 11 bits split low/high byte
  if (Addr == 0xfff8) {
    // set 8 lower bits keep 3 upper bits
    bg_position = (bg_position & 0x0700) | Value;
    return;
  }
  if (Addr == 0xfff9) {
    // set 3 upper bits, keep the 8 lower bits
    bg_position = (bg_position & 0x00ff) | ((uint16_t)(Value & 0x07) << 8);
    return;
  }
  // fffa      background color bank (0 = standard  1 = reddish)
  if (Addr == 0xfffa) {
    bg_color_bank = (Value & 1) ? 0x80 : 0x00;
    return;
  }
  // fffb      background enable
  if (Addr == 0xfffb) {
    bg_enable = Value & 1;
    return;
  }
}

void zaxxon::run_frame(void) {
  game_started = 1;

  for (uint16_t i = 0; i < INST_PER_FRAME; i++) {
    StepZ80(&cpu[0]);
    StepZ80(&cpu[0]);
    StepZ80(&cpu[0]);
    StepZ80(&cpu[0]);
  }

  vblank.publish([this](VideoState &s) {
    memcpy(s.video_ram, video_ram, sizeof(s.video_ram));
    memcpy(s.sprite_ram, sprite_ram, sizeof(s.sprite_ram));
    s.bg_position = bg_position;
    s.bg_color_bank = bg_color_bank;
    s.bg_enable = bg_enable;
  });

  if (irq_enable[0]) {
    IntZ80(&cpu[0], INT_IRQ);
  }
}

// MAME zaxxon_state::draw_sprites (zaxxon_v.cpp): 32 sprites x 4 bytes,
// only the lower half of sprite RAM is read (offs 0x7c..0x00 step -4).
//   byte0 = Y pos (raw dst_x axis, run through find_minimum_y)
//   byte1 = code (bits 0-5) | flipX (0x40) | flipY (0x80)
//   byte2 = color group (bits 0-4)
//   byte3 = X pos (raw band axis, run through find_minimum_x)
void zaxxon::prepare_frame(void) {
  vblank.read(video);

  // See bg_tile_row_snapshot's declaration: dst_x-axis (column) terms are
  // frame-constant, blit_bg_row must not redo them per band/sub_y.
  // j = 223 has the lowest bg row: the window's first tile row
  const uint16_t srcy_base = ((video.bg_position << 1) ^ 0xfff) + 1;
  const uint16_t first_tile_row = ((239 - 223 + srcy_base) & 0xfff) >> 3;
  for (uint16_t j = 0; j < 224; j++) {
    const uint8_t ry = 239 - j;
    const uint16_t bg_row = (ry + srcy_base) & 0xfff; // 4096-row plane
    const uint16_t tile_row = bg_row >> 3;            // 0-511
    bg_cell_base_snapshot[j] = ((tile_row - first_tile_row) & 511) * 32;
    bg_sub_bg_y_snapshot[j] = 7 - (bg_row & 7);
  }

  const uint16_t *codes = zaxxon_tilemap_code;
  const uint8_t *groups = zaxxon_tilemap_color;
  for (uint16_t r = 0; r < BG_WINDOW_ROWS; r++) {
    const uint16_t src = ((first_tile_row + r) & 511) * 32;
    memcpy(bg_code + r * 32, codes + src, 32 * sizeof(uint16_t));
    for (uint8_t c = 0; c < 32; c++) {
      bg_pal[r * 32 + c] = groups[src + c] * 8 + video.bg_color_bank;
    }
  }

  active_sprites = 0;

  for (int16_t offs = (SPRITE_COUNT - 1) * 4; offs >= 0; offs -= 4) {
    const uint8_t attr = video.sprite_ram[offs + 1];

    struct sprite_S spr;
    spr.code = attr & 0x3f;
    spr.color = video.sprite_ram[offs + 2] & 0x1f;
    spr.flip_x = (attr & 0x40) ? 1 : 0;
    spr.flip_y = (attr & 0x80) ? 1 : 0;
    spr.y = find_minimum_x(video.sprite_ram[offs + 3]); // MAME sx -> band axis
    spr.x = find_minimum_y(video.sprite_ram[offs + 0]); // MAME sy -> dst_x axis

    sprite[active_sprites++] = spr;
  }
}

uint8_t zaxxon::find_minimum_x(uint8_t value) { return (value + 0xf0) & 0xff; }

uint8_t zaxxon::find_minimum_y(uint8_t value) {
  int16_t y;
  for (y = 0; y < 256; y += 16) {
    if (((value + 0xf2 + y) & 0xe0) == 0xe0) {
      break;
    }
  }
  while (((value + 0xf2 + (y - 1)) & 0xe0) == 0xe0) {
    y--;
  }
  return (y + 1) & 0xff;
}

// Hardware is native landscape (MAME ROT90 for this driver), like the
// Pacman/Galaxian address-remap trick: only the tile *grid position* is
// rotated, individual tile bitmaps are stored/decoded upright and blitted
// unrotated (see blit_tile). Physical row "bands" (8 scanlines each)
// therefore step over raw tile *columns* (rx = tcol*8 + band-local
// scanline) rather than raw tile rows. Hardware cliprect VBEND=16/
// VBSTART=240 (28 tile rows, trow 2..29) maps to the dst_x axis, giving
// the full 224px screen width -- no further cropping needed. All 32 raw
// tile columns are visible (32 bands, 256px), so with 36 physical bands
// available we get a 2-band letterbox top and bottom.
void zaxxon::render_row(short row) {
  const int8_t tcol = row - 2;
  if (tcol < 0 || tcol >= 32) {
    return;
  }

  blit_bg_row(row);

  for (uint8_t s = 0; s < active_sprites; s++) {
    blit_sprite(row, s);
  }

  for (uint8_t trow = 2; trow < 30; trow++) {
    blit_tile(trow, tcol);
  }
}

// MAME's draw_background xstage term depends only on the dst_x-axis column
// (raw Y = 239-j), never on frame state or the band/sub_y blit_bg_row loops
// over -- a true constant, computed once ever instead of once per (sub_y, j)
// pair (was 57344x/frame). See zaxxon.h's bg_tile_row_snapshot comment for
// the frame-constant (but not fully constant) counterpart.
static uint16_t zaxxon_bg_xstage[224];
static bool zaxxon_bg_xstage_ready = false;

static void zaxxon_init_bg_xstage() {
  for (uint16_t j = 0; j < 224; j++) {
    const uint8_t ry = 239 - j;
    zaxxon_bg_xstage[j] = ((ry >> 1) ^ 0xff) + 1 + 0x3f;
  }
  zaxxon_bg_xstage_ready = true;
}

// Reproduces zaxxon_state::draw_background(bitmap, cliprect, skew=true) from
// MAME's zaxxon_v.cpp: a per-pixel diagonal skew (2 discrete 4-bit adders)
// against a 32x512-tile background plane, giving the isometric parallax look.
// No flip-screen support (cocktail mode isn't wired up on this port).
void zaxxon::blit_bg_row(uint8_t row) {
  if (!zaxxon_bg_xstage_ready) {
    zaxxon_init_bg_xstage();
  }

  // frame_buffer holds one 8-scanline band per render_row() call, not the
  // full frame (renderBuffer() == 224*2*8) -- same convention blit_tile uses.
  uint16_t *ptr = frame_buffer;
  const uint8_t tcol =
      row - 2; // raw tile column (0-31): the band axis is raw X

  // Hoisted out of the per-pixel loop: Asset::data() branch-checks its
  // decompression cache on every call: not free to leave inside a loop that
  // runs up to 1792 times per band call.
  const auto *bgtiles = zaxxon_bgtiles;
  const auto *palette = zaxxon_palette;

  if (!video.bg_enable) {
    memset(ptr, 0, 8 * 224 * sizeof(uint16_t));
    return;
  }

  // locals, not members: keeps the per-pixel loop in registers
  const uint16_t *cell_base = bg_cell_base_snapshot;
  const uint8_t *sub_bg_y = bg_sub_bg_y_snapshot; // pre-inverted
  const uint16_t *code = bg_code;
  const uint8_t *pal = bg_pal;
  const uint8_t rx0 = tcol * 8; // raw X of the band's first line

  // Column-major: the column terms load once per 8 pixels. The band's 8
  // lines are 8 consecutive bg_col, spanning at most 2 tiles.
  for (uint8_t j = 0; j < 224; j++) {
    const uint8_t bg_col = rx0 + zaxxon_bg_xstage[j]; // 256px plane, wraps
    const uint8_t tile_col = bg_col >> 3;
    const uint8_t sub_bg_x = bg_col & 7;
    const uint8_t y = sub_bg_y[j];
    uint16_t *dst = ptr + j;

    // bg tiles are pre-rotated 90deg for the portrait framebuffer, same as
    // fg/sprite tiles (rot_galagino in gfxutil.py): undo it with
    // transposed+mirrored indices instead of the raw sub-tile offsets.
    uint16_t cell = cell_base[j] + tile_col;
    const uint8_t *src = &bgtiles[code[cell]][sub_bg_x][y];
    const uint16_t *colors = palette + pal[cell];
    for (uint8_t x = sub_bg_x; x < 8; x++, src += 8, dst += 224) {
      *dst = colors[*src];
    }

    cell = cell_base[j] + ((tile_col + 1) & 31);
    src = &bgtiles[code[cell]][0][y];
    colors = palette + pal[cell];
    for (uint8_t x = 0; x < sub_bg_x; x++, src += 8, dst += 224) {
      *dst = colors[*src];
    }
  }
}

// renders the "foreground" (basically the HUD). Only the tile's grid
// position is rotated into the band (row/trow picks the dst_x slice, col/
// tcol picks the band -- see render_row); the 8x8 bitmap itself is blitted
// straight (unrotated), same as the pre-rotation code, since the ROM tile
// bitmaps decode upright already (see romconv preview).
void zaxxon::blit_tile(short row, char col) {
  const uint8_t code = video.video_ram[row * 32 + col];
  if (char_blank[code]) {
    return;
  }

  // MAME zaxxon_get_fg_tile_info: color group selected by screen column and
  // row-quadrant, not by tile data; tileinfo color = group * 2, palette
  // granularity 4 -> palette base = group * 8.
  const uint8_t group = zaxxon_fgcolor_codes[col + 32 * (row / 4)] & 0x0f;
  const uint16_t *colors = zaxxon_palette + group * 8;

  const unsigned char (*tile)[8] = zaxxon_chartiles[code];
  const uint8_t dst_x =
      232 - row * 8; // trow 2..29 -> dst_x 216..0, 8px per tile
  uint16_t *ptr = frame_buffer + dst_x;

  for (uint8_t r = 0; r < 8; r++, ptr += (224 - 8)) {
    for (uint8_t c = 0; c < 8; c++, ptr++) {
      const uint8_t pix = tile[r][c];
      if (pix) {
        *ptr = colors[pix];
      }
    }
  }
}

// MAME zaxxon_state::draw_sprites (zaxxon_v.cpp): 32x32 3bpp sprites, drawn
// between the background and the fg tilemap. Raw X (MAME sx, spr.y here) is
// the band axis -- direct, like blit_bg_row's rx; raw Y (MAME sy, spr.x
// here) is the dst_x axis -- reversed, like blit_bg_row's ry. Sprite tiles
// are pre-rotated the same way as bg/fg tiles (rot_galagino), so the tile
// lookup mirrors zaxxon_bgtiles' [x][7-y] convention, scaled to 32x32.
void zaxxon::blit_sprite(short row, unsigned char s) {
  // Band axis (raw X) is fully visible, 0-255, with no hardware crop (see
  // render_row comment) -- unlike dst_x, which is a real VBLANK crop. A
  // sprite near the 256px seam (spr.y > 224) therefore wraps its tail back
  // to raw X 0, same as MAME's extra draw_sprites copies at sx-0x100
  // (zaxxon_v.cpp): try the sprite at its raw position and 256px earlier,
  // whichever overlaps the current band actually draws.
  static const int16_t BAND_WRAP_OFFSETS[2] = {0, -256};

  const sprite_S &spr = sprite[s];
  if (sprite_blank[spr.code]) {
    return;
  }
  const int16_t band_start = (row - 2) * 8;
  const uint16_t *colors = zaxxon_palette + spr.color * 8;
  const unsigned char (*tile)[32] = zaxxon_spritetiles[spr.code];

  int16_t dst_x[32];
  bool dst_x_ready = false;

  for (uint8_t w = 0; w < 2; w++) {
    const int16_t y0 = (int16_t)spr.y + BAND_WRAP_OFFSETS[w];

    const int16_t ox0 = (y0 < band_start) ? (band_start - y0) : 0;
    const int16_t ox1 = (y0 + 32 > band_start + 8) ? (band_start + 8 - y0) : 32;
    if (ox0 >= ox1) {
      continue;
    }

    // dst_x axis needs the same {0,-256} wrap MAME's draw_sprites draws
    // for sy -- see zaxxon_sprite_geom.h. Same for every ox: once per band.
    if (!dst_x_ready) {
      for (uint8_t oy = 0; oy < 32; oy++) {
        dst_x[oy] = zaxxon_resolve_sprite_dst_x(spr.x, oy, 224);
      }
      dst_x_ready = true;
    }

    for (int16_t ox = ox0; ox < ox1; ox++) {
      const uint8_t band_local = (y0 + ox) - band_start;
      const uint8_t sub_x = spr.flip_x ? (31 - ox) : ox;
      const unsigned char *column = tile[sub_x];
      uint16_t *const ptr = frame_buffer + band_local * 224;

      for (uint8_t oy = 0; oy < 32; oy++) {
        if (dst_x[oy] < 0) {
          continue;
        }

        const uint8_t pix = column[spr.flip_y ? oy : 31 - oy];
        if (pix) {
          ptr[dst_x[oy]] = colors[pix];
        }
      }
    }
  }
}

const unsigned short *zaxxon::logo(void) {
  return zaxxon_logo;
}

#ifdef LED_PIN
static const CRGB ZAXXON_LED_DARKBLUE(0x000033);
static const CRGB ZAXXON_LED_LIGHTBLUE(0x3399ff);

void zaxxon::menuLeds(CRGB *leds) {
  static const CRGB menu_leds[NUM_LEDS] = {
      ZAXXON_LED_DARKBLUE, LED_BLUE, ZAXXON_LED_LIGHTBLUE, LED_CYAN,
      ZAXXON_LED_LIGHTBLUE, LED_BLUE, ZAXXON_LED_DARKBLUE};
  memcpy(leds, menu_leds, NUM_LEDS * sizeof(CRGB));
}

void zaxxon::gameLeds(CRGB *leds) {
  static int explosion_cnt = 0;
  // bg_color_bank (fffb) turns red on fuel alarm and player hit.
  if (bg_color_bank && explosion_cnt == 0) {
      explosion_cnt = 90;  // show explosion fx for 1.5s
  }
  if (explosion_cnt > 0) {
    for (uint8_t c = 0; c < NUM_LEDS; c++) {
      leds[c] = (random(2) & 1) == 0 ? LED_RED : LED_YELLOW;
    }
    explosion_cnt--;
    return;
  }

  static char sub_cnt = 0;  // update only every 1/10s
  if (sub_cnt++ < 6) {
    return;
  }
  sub_cnt = 0;

  // Animate only while the playfield scrolls (fff8/fff9 bg_position).
  static uint16_t last_bg_position = bg_position;
  const bool scrolling = bg_position != last_bg_position;
  last_bg_position = bg_position;
  if (!scrolling) {
    return;
  }

  static const CRGB idle_leds[] = {ZAXXON_LED_DARKBLUE, LED_BLUE,
                                   ZAXXON_LED_LIGHTBLUE, LED_CYAN};
  static char led = 0;
  for (char c = 0; c < (NUM_LEDS + 1) / 2; c++) {
    leds[NUM_LEDS - 1 - c] = leds[c] =
        idle_leds[(c + led) % (sizeof(idle_leds) / sizeof(CRGB))];
  }
  led = (led + 1) % (sizeof(idle_leds) / sizeof(CRGB));
}
#endif
#endif
