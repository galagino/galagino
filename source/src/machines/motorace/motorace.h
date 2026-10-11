#ifndef MOTORACE_H
#define MOTORACE_H

#include "../machineBase.h"

#ifdef ENABLE_MOTORACE

#include "motorace_logo.h"
#include "motorace_dipswitches.h"
#include "../../cpus/m6803/m6803.h"

// MotoRace USA memory layout in `memory[]` buffer (RAMSIZE=9344):
//   VRAM: 0x8000-0x8FFF (4096) -> offset 0x0000
//   Sprite RAM: 0xC800-0xC9FF (512) -> offset 0x1000
//   Work RAM: 0xE000-0xEFFF (4096) -> offset 0x1200
#define MR_MEM_VRAM     0x0000
#define MR_MEM_SPRITES  0x1000
#define MR_MEM_WORKRAM  0x1200

class motorace : public machineBase
{
public:
  motorace() { }
  ~motorace() { }

  signed char machineType()    override { return MCH_MOTORACE; }
  // flipY toggles both MY and MX (video.cpp); flipX undoes MX.
  signed char videoFlipY()  override { return 1; }
  signed char videoFlipX()  override { return 1; }
  signed char isLandscape() override { return 1; }

  signed char useVideoHalfRate() override { return 0; }
  //bool hasOpaqueBG()           override { return true;  } // BG scroll opaco (no memset richiesto)

  // Native raster is portrait 240x256.
  const int   renderWidth()  override { return 256; }
  const int   renderBuffer() override { return 256 * 2 * 8; }

  unsigned char rdZ80(unsigned short Addr) override;
  void          wrZ80(unsigned short Addr, unsigned char Value) override;
  unsigned char opZ80(unsigned short Addr) override;

  void init(Input *input, unsigned short *framebuffer,
            sprite_S *spritebuffer, unsigned char *memorybuffer) override;
  void reset() override;

  void run_frame(void) override;
  void prepare_frame(void) override;
  void render_row(short row) override;
  const unsigned short *logo(void) override;

  // M6803 sound CPU memory access (chiamati da callback C)
  uint8_t snd_read(uint16_t addr);
  void    snd_write(uint16_t addr, uint8_t val);
  void    snd_port_write(uint8_t port, uint8_t val);
  uint8_t snd_port_read(uint8_t port);

protected:
  // Unused: render_row() draws strips directly.
  void blit_tile(short row, char col)            override { }
  void blit_sprite(short row, unsigned char s)   override { }

private:
  void blit_scroll_strip_t(short strip_r);
  void blit_sprite_t(short strip_r, unsigned char s);

  unsigned char scroll_x_low;
  unsigned char scroll_x_high;
  unsigned char flipscreen;
  unsigned char sound_cmd;

  // M6803 sound CPU state
  m6803_state   snd_cpu;
  unsigned char snd_port1;
  unsigned char snd_port2;
  unsigned char ay_addr[2];
  unsigned char ay_regs[2][16];
};

// Global pointer per callback M6803
extern motorace *g_motorace_instance;

#endif // ENABLE_MOTORACE
#endif // MOTORACE_H
