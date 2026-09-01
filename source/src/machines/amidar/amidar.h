#ifndef AMIDAR_H
#define AMIDAR_H

#include "amidar_logo.h"
#include "amidar_main_rom.h"
#include "amidar_audio_rom.h"
#include "amidar_spritemap.h"
#include "amidar_tilemap.h"
#include "amidar_cmap.h"
#include "amidar_dipswitches.h"
#include "../turtles/turtles.h"

// ============================================================
// Amidar (Konami 1982) — runs on identical hardware to Turtles
// ROM set: amidar  (5 x 4KB main ROMs = 16KB)
// ROM set: amidar1 (4 x 4KB main ROMs = 16KB)
//
// Memory map: identical to Turtles except CPU1_ROM_SIZE = 0x4000 (amidar1)
// Input differences vs Turtles:
//   IN1 bits 1:0: Lives 11=3, 10=4, 01=5, 00=cheat (reversed)
//   IN2 bit 1: Demo Sounds (0=On); bit 2: Bonus Life
//   IN3 (0xB030): Coinage (bits 3:0=CoinA, bits 7:4=CoinB; 0xFF=1C/1C)
// ============================================================

class amidar : public turtles
{
public:
  amidar() {}
  ~amidar() {}

  signed char machineType() override { return MCH_AMIDAR; }

  unsigned char opZ80(unsigned short Addr) override;
  unsigned char rdZ80(unsigned short Addr) override;

  const unsigned short *logo(void) override;

#ifdef LED_PIN
  void menuLeds(CRGB *leds) override;
  void gameLeds(CRGB *leds) override;
#endif

protected:
  void blit_tile(short row, char col) override;
  void blit_sprite(short row, unsigned char s) override;

private:
  /*
  static constexpr unsigned short CPU1_ROM_SIZE = 0x4000;  // 4 x 4KB (amidar1 set)
  */
  static constexpr unsigned short CPU1_ROM_SIZE = 0x5000;  // 4 x 4KB (amidar set)
  static constexpr unsigned short CPU2_ROM_SIZE = 0x2000;

#ifdef LED_PIN
  const CRGB menu_leds[7] = { LED_RED, LED_YELLOW, LED_RED, LED_WHITE, LED_RED, LED_YELLOW, LED_RED };
#endif
};

#endif
