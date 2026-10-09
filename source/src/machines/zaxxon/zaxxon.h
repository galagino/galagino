#ifndef ZAXXON_H
#define ZAXXON_H

#ifdef ENABLE_ZAXXON

#include "zaxxon_rom_main.h"
#include "zaxxon_chartiles.h"
#include "zaxxon_bgtiles.h"
#include "zaxxon_spritetiles.h"
#include "zaxxon_tilemap.h"
#include "zaxxon_palette.h"
#include "zaxxon_logo.h"
#include "zaxxon_sample_missile_homing.h"
#include "zaxxon_sample_missile_base.h"
#include "zaxxon_sample_laser.h"
#include "zaxxon_sample_battleship.h"
#include "zaxxon_sample_explosion_enemy.h"
#include "zaxxon_sample_explosion_ship.h"
#include "zaxxon_sample_cannon.h"
#include "zaxxon_sample_shot.h"
#include "zaxxon_sample_alarm_lock.h"
#include "zaxxon_sample_alarm_fuel.h"
#include "zaxxon_sample_noise_intro.h"
#include "zaxxon_sample_noise_asteroid.h"
#include "../machineBase.h"
#include "../../emulation/seqlock.h"

// PPI-triggered discrete sample board (zaxxon_a.cpp zaxxon_sample_names):
// 12 channels, indexed exactly as MAME's sample table so Audio can trigger
// them by index without knowing individual symbol names. Each points at a
// 4-bit IMA-ADPCM stream (2-byte predictor header + packed nibbles, see
// romconv/pyconv/adpcm.py), decoded per-sample by Audio::zaxxon_render_buffer.
static const uint8_t *const zaxxon_sample_ptr[12] = {
  zaxxon_sample_missile_homing,  // 0
  zaxxon_sample_missile_base,    // 1
  zaxxon_sample_laser,           // 2
  zaxxon_sample_battleship,      // 3
  zaxxon_sample_explosion_enemy, // 4 (S-Exp)
  zaxxon_sample_explosion_ship,  // 5 (M-Exp)
  zaxxon_sample_cannon,          // 6
  zaxxon_sample_shot,            // 7
  zaxxon_sample_alarm_lock,      // 8 (Alarm2)
  zaxxon_sample_alarm_fuel,      // 9 (Alarm3)
  zaxxon_sample_noise_intro,     // 10
  zaxxon_sample_noise_asteroid,  // 11
};
static const uint32_t zaxxon_sample_len[12] = {
  sizeof(zaxxon_sample_missile_homing),
  sizeof(zaxxon_sample_missile_base),
  sizeof(zaxxon_sample_laser),
  sizeof(zaxxon_sample_battleship),
  sizeof(zaxxon_sample_explosion_enemy),
  sizeof(zaxxon_sample_explosion_ship),
  sizeof(zaxxon_sample_cannon),
  sizeof(zaxxon_sample_shot),
  sizeof(zaxxon_sample_alarm_lock),
  sizeof(zaxxon_sample_alarm_fuel),
  sizeof(zaxxon_sample_noise_intro),
  sizeof(zaxxon_sample_noise_asteroid),
};

// The ROM (0x0165) pulses the one-shot bits of ports B/C: writes ~request,
// then 0xff (or |0xf0) right after, so render-time sampling of the port
// latch never sees them low. zaxxon::wrZ80 counts each falling edge in
// soundregs[ZAXXON_TRIG_BASE + ch] (ch 4..9); Audio starts a channel when
// its counter changed. Single writer, single reader, no lock needed.
static const uint8_t ZAXXON_TRIG_BASE = 4;

// ============================================================
// Zaxxon (Sega 1982, set "zaxxon" US rev D). Single Z80 @ 3.04125MHz
// (48.66MHz/16). Memory map (from schematics, see zaxxon_rom_convert.py):
//   0000-4fff ROM (20KB, not encrypted on this set)
//   6000-6fff work RAM
//   8000-83ff fg videoram (mirrored)
//   a000-a0ff sprite RAM, 32 sprites x 4 bytes (mirrored)
//   c000-c003 IN0/IN1/DSW0/DSW1 (r) / output latch (w, coin/flip/etc)
//   c100      IN2 (r)
//   e03c-e03f i8255 PPI (discrete sound triggers)
//   fff0-fffb irq enable / fg+bg color bank / bg scroll (11-bit) / bg enable
// ============================================================

class zaxxon : public machineBase
{
protected:
  /*
  // ROM assets, unpacked into the Arena on first data()
  Asset<unsigned char[8][8], COMPRESSED> zaxxon_bgtiles;
  Asset<unsigned char[8][8], COMPRESSED> zaxxon_chartiles;
  Asset<unsigned short, COMPRESSED> zaxxon_palette;
  Asset<unsigned char, COMPRESSED> zaxxon_fgcolor_codes;
  Asset<unsigned char, COMPRESSED> zaxxon_rom_main;
  Asset<unsigned char[32][32], COMPRESSED> zaxxon_spritetiles;
  Asset<unsigned short, PLAIN> zaxxon_tilemap_code;
  Asset<unsigned char, PLAIN> zaxxon_tilemap_color;
  */
public:
  zaxxon();

  signed char machineType() override { return MCH_ZAXXON; }

  // Hardware is native landscape 256x224 (MAME ROT90), panel is portrait.
  // render_row()/blit_tile() rotate the fg tilemap 90 deg into the portrait
  // frame_buffer: after rotation, screen width == raw visible height (224).
  const int renderWidth() override { return 224; }
  const int renderBuffer() override { return 224 * 2 * 8; }

  //static Asset<unsigned short, COMPRESSED> &logo() { return zaxxon_logo; }
  const unsigned short *logo(void) override;

  unsigned char rdZ80(unsigned short Addr) override;
  void wrZ80(unsigned short Addr, unsigned char Value) override;
  unsigned char opZ80(unsigned short Addr) override;

  void start(void) override;
  void run_frame(void) override;
  void prepare_frame(void) override;
  void render_row(short row) override;

#ifdef LED_PIN
  static void menuLeds(CRGB *leds);
  void gameLeds(CRGB *leds) override;
#endif

protected:
  void blit_tile(short row, char col) override;
  void blit_sprite(short row, unsigned char s) override;

private:
  // per-pixel skewed background, drawn before the fg tilemap (see
  // draw_background() in zaxxon_v.cpp for the reference discrete-logic formula)
  void blit_bg_row(uint8_t row);

  // MAME zaxxon_state::find_minimum_x/y (zaxxon_v.cpp): discrete-logic line
  // buffer comparators that position a sprite on the raw X/Y axis. Ported
  // verbatim with flip_screen hardcoded false (no flip-screen support on
  // this port, see blit_bg_row).
  static uint8_t find_minimum_x(uint8_t value);
  static uint8_t find_minimum_y(uint8_t value);

  // sprite RAM holds 32 sprites x 4 bytes, but MAME only reads the lower
  // half (offs 0x7c..0x00) when drawing -- see prepare_frame.
  static constexpr uint8_t SPRITE_COUNT = 32;

  // Codes whose bitmap is all pen 0, built once in the ctor: most of the
  // fg layer is blank, and drawing a blank tile/sprite changes nothing.
  static constexpr uint16_t CHAR_CODES = 256;
  static constexpr uint8_t SPRITE_CODES = 64;
  bool char_blank[CHAR_CODES];
  bool sprite_blank[SPRITE_CODES];

  // --- work RAM layout inside machineBase::memory (RAMSIZE budget) ---
  static constexpr uint16_t WORK_RAM_OFFSET = 0x0000;
  static constexpr uint16_t WORK_RAM_SIZE    = 0x1000; // 6000-6fff
  static constexpr uint16_t VIDEORAM_OFFSET  = WORK_RAM_OFFSET + WORK_RAM_SIZE;
  static constexpr uint16_t VIDEORAM_SIZE    = 0x0400; // 8000-83ff
  static constexpr uint16_t SPRITERAM_OFFSET = VIDEORAM_OFFSET + VIDEORAM_SIZE;
  static constexpr uint16_t SPRITERAM_SIZE   = 0x0100; // a000-a0ff
  static constexpr uint16_t MEM_FREE_ZAXXON  = SPRITERAM_OFFSET + SPRITERAM_SIZE;
  static_assert(MEM_FREE_ZAXXON <= RAMSIZE, "RAMSIZE too low for zaxxon");

  uint8_t *work_ram;
  uint8_t *video_ram;
  uint8_t *sprite_ram;

  uint8_t last_coin = 0;

  uint16_t bg_position = 0;   // fff8/fff9, 11 bits
  uint8_t bg_color_bank = 0;  // fffa, 0x00 or 0x80
  uint8_t bg_enable = 0;      // fffb

  // Sprite/bg/fg state as of the end of run_frame() (= MAME's render point,
  // before the vblank IRQ handler rewrites sprite RAM and scroll), handed
  // to the video core via Seqlock (see seqlock.h). fg tile RAM (HUD) is in
  // here too so tiles, sprites and scroll always come from the same frame.
  struct VideoState {
    uint8_t video_ram[VIDEORAM_SIZE];
    uint8_t sprite_ram[SPRITE_COUNT * 4];
    uint16_t bg_position;
    uint8_t bg_color_bank;
    uint8_t bg_enable;
  };
  Seqlock<VideoState> vblank;
  VideoState video = {};

  // blit_bg_row's dst_x-axis terms (draw_background's tile_row/sub_bg_y)
  // depend only on the column j and video.bg_position -- never on the
  // band or the 8 sub_y sub-rows blit_bg_row loops over -- so precompute
  // them once per frame here (in prepare_frame()) instead of redoing the
  // same 224 values 8x/band = 256x/frame inside the per-pixel hot loop.
  uint16_t bg_cell_base_snapshot[224]; // bg window cell of column j, col 0
  uint8_t bg_sub_bg_y_snapshot[224];   // pre-inverted: 7 - sub_bg_y

  // Visible slice of the flash tilemap, copied once per frame: the per
  // pixel loop then reads RAM, not flash (57344 lookups per frame). 224
  // lines span at most 29 tile rows. bg_pal = color group * 8 + color bank.
  static const uint16_t BG_WINDOW_ROWS = 29;
  uint16_t bg_code[BG_WINDOW_ROWS * 32];
  uint8_t bg_pal[BG_WINDOW_ROWS * 32];
};

#endif
#endif
