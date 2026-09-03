#ifndef MAPPY_H
#define MAPPY_H

// ============================================================================
// Mappy (Namco 1983) — hardware "Super Pacman class":
// 2x MC6809E @ 1.536MHz (main + sound), WSG 15XX 8 voices (clock 24000 Hz =
// galagino sample rate, zero resampling), 2x custom I/O Namco 58XX,
// tilemap 36x60 con scroll + 2x2 fixed columns, 64 sprites 16x16 4bpp.
//
// MAME: namco/mappy.cpp, mappy_v.cpp, namcoio.cpp, sound/namco.cpp.
// Rotated gfx ROT90 (romconv/mappy/mappy_rom_convert.py)
// galagino X axis (224) == MAME y axis
// galagino Y axis (288) == MAME x axis (36 columns)
// ============================================================================

#include "../machineBase.h"
#include "../../cpus/m6809/m6809.h"
#include "mappy_rom_main.h"
#include "mappy_rom_sub.h"
#include "mappy_tilemap.h"
#include "mappy_spritemap.h"
#include "mappy_cmap.h"
#include "mappy_wavetable.h"
#include "mappy_dipswitches.h"
#include "mappy_logo.h"

#define MAPPY_SLICES 1600

// memory layout
//
//   0x0000-0x0fff  VRAM (tiles 0x000-0x07ff, attributes 0x0800-0x0fff)
//   0x1000-0x27ff  work RAM main with embedded RAM
//     (tab1 tiles/colors @0x1780, tab2 Y/X @0x1f80, tab3 flags @0x2780)
//   0x2800-0x2BFF  shared RAM 
//             main 0x4000-0x43ff 
//             sub  0x0000-0x03ff
//     (i primi 0x40 byte sono i registri WSG 15XX -> soundregs[])
#define MAPPY_SHARED_OFF 0x2800

class mappy : public machineBase
{
public:
    mappy() : rom_main(mappy_rom_main), 
              rom_sub(mappy_rom_sub),
              tiles(mappy_tilemap), 
              cmap_tiles(mappy_colormap_tiles),
              cmap_prio(mappy_colormap_tiles_prio),
              cmap_sprites(mappy_colormap_sprites) { }
    ~mappy() { }

    signed char machineType() override { return MCH_MAPPY; }
    signed char useVideoHalfRate() override { return 1; }

    void reset() override;

    unsigned char m6809_read(m6809_state *s, uint16_t addr) override;
    void m6809_write(m6809_state *s, uint16_t addr, uint8_t val) override;
    unsigned char m6809_read_opcode(m6809_state *s, uint16_t addr) override;

    void run_frame(void) override;
    void prepare_frame(void) override;
    void render_row(short row) override;

    // audio WSG 15XX 8 voci (Audio::namco_15xx_render_buffer legge soundregs)
    bool hasNamco15xxAudio() override { return true; }
    bool namcoSoundEnabled() override { return wsg_enable != 0; }
    const signed char *waveRom(unsigned char value) override { return mappy_wavetable[value & 7]; }

    const unsigned short *logo(void) override { return mappy_logo; }

protected:
    void blit_tile(unsigned short idx, short x, char prio);
    void blit_sprite(short row, unsigned char s);

private:
    void render_tiles(short row, char prio);
    void mainlatch_w(uint16_t addr);
    void install_rom_direct(void);

    // --- Namco 58XX custom I/O (2 chips), MAME namcoio.cpp ---
    struct n58xx_S {
        unsigned char ram[16];      // 16 nibbles CPU (0xf0 | val)
        int lastcoins, lastbuttons;
        int credits;
        int coins[2];
        int coins_per_cred[2];
        int creds_per_coin[2];
    };
    void io_chips_reset(void);
    void customio_run(unsigned char chip);
    void handle_coins(unsigned char chip, unsigned char swap);
    unsigned char io_in(unsigned char chip, unsigned char port);

    m6809_state main_cpu;
    m6809_state sub_cpu;

    const unsigned char *rom_main;               // 24KB, main 0xA000-0xFFFF
    const unsigned char *rom_sub;                // 8KB,  sub 0xE000-0xFFFF
    const unsigned short (*tiles)[8];            // 4KB,  mappy_tilemap
    const unsigned short (*cmap_tiles)[4];       // 512B
    const unsigned short (*cmap_prio)[4];        // 512B
    const unsigned short (*cmap_sprites)[16];    // 512B

    n58xx_S io[2];
    unsigned char dipmux_sel;      // LS157: 0 = DSW2 nibble low, 1 = high

    // mainlatch LS259 @0x5000 main / 0x2000 sub (A0 = data):
    // Q0 = sub irq mask, Q1 = main irq mask, Q2 = flip (ignored),
    // Q3 = sound enable, Q4 = !reset namcoio, Q5 = !reset sub CPU
    unsigned char main_irq_mask, sub_irq_mask;
    unsigned char wsg_enable;
    unsigned char sub_reset, io_reset;

    unsigned char scroll;          // 0x3800-0x3FFF: m_scroll = offset >> 3
};

#endif
