#ifndef MOONPATROL_H
#define MOONPATROL_H

#include "../machineBase.h"

#ifdef ENABLE_MOONPATROL

// ============================================================================
// Moon Patrol (Irem 1982) — porting da FULL33 a SPINNERINO WIFI (ESP32-P4).
//
// Hardware (MAME irem/mpatrol.cpp):
//   Main CPU:  Z80 @ 3.072 MHz
//   Audio CPU: M6803 (Irem M52 sound board) + 2x AY-3-8910 + MSM5205 ADPCM
//   Sound:     path generico AY in audio.cpp (AY=2, gia' cablato) + MSM5205
//              ADPCM mixato via g_moonpatrol_instance->msm_signal.
//   Video:     game nativo ORIZZONTALE (monitor orizzontale, ROT0). 3 layer di
//              parallax (bg0 mountains / bg1 hills / bg2 city) + tilemap 32x32
//              foreground (scroll parziale ultimo quarto) + sprite 16x16 3bpp.
//
// Adattamento SPINNERINO (vs FULL33), come greenberet/mariobros (orizzontali):
//   - Core emulazione (rd/wr/op/in/out/run_frame + M6803/AY/MSM5205 + parsing
//     sprite) IDENTICO a FULL33.
//   - Render: come FULL33/Green Beret. Il game LANDSCAPE 240-wide e' reso
//     commutando il pannello in PORTRAIT (video.setLandscapeGame) e scrivendo le
//     righe native da 240 px DIRETTAMENTE in frame_buffer (render_row righe 2..33).
//     Niente mp_full, niente transpose software.
//   - usesPortraitPanel()=true (il render loop usa il path portrait).
//   - Anti-flicker per-slot (MP_SPRITE_PERSIST) per il blink 60Hz vs 30Hz FULL33.
// ============================================================================

#include "moonpatrol_logo.h"
#include "moonpatrol_rom.h"
#include "moonpatrol_snd_rom.h"
#include "moonpatrol_dipswitches.h"
#include "moonpatrol_tilemap.h"
#include "moonpatrol_spritemap.h"
#include "moonpatrol_bgmap.h"
#include "moonpatrol_cmap.h"
#include "../../cpus/m6803/m6803.h"

// Geometria nativa: ORIZZONTALE 240 wide x 224 tall. Reso come FULL33/Green
// Beret: pannello commutato in PORTRAIT (video.setLandscapeGame) e render diretto
// a righe da 240 in frame_buffer. Niente mp_full, niente transpose software.
#define MP_NW   240
#define MP_NH   224

// ANTI-FLICKER (vedi greenberet): SPINNERINO 60Hz vs FULL33 30Hz -> gli sprite
// che il gioco fa lampeggiare a frame alterni qui si vedrebbero sfarfallare.
// Persistenza di N frame per-slot: uno sprite spento viene ridisegnato dal frame
// precedente -> nemici completi, niente ghosting. 1 = anti-flicker; 0 = originale.
#define MP_SPRITE_PERSIST 1
#define MP_SPRITE_SLOTS   64   // 4 gruppi x 16 slot

// Moon Patrol memory layout in our buffer
// VRAM:       0x8000-0x83FF  -> offset 0x0000 (1KB)
// CRAM:       0x8400-0x87FF  -> offset 0x0400 (1KB)
// Sprite RAM: 0xC800-0xCBFF  -> offset 0x0800 (1KB with mirrors)
// Work RAM:   0xE000-0xE7FF  -> offset 0x0C00 (2KB)
// Total: 0x1400 = 5120 bytes < RAMSIZE (8192)
#define MP_MEM_VRAM     0x0000
#define MP_MEM_CRAM     0x0400
#define MP_MEM_SPRITES  0x0800
#define MP_MEM_WORKRAM  0x0C00

// Sound command logger
#define MP_SND_LOG_SIZE 32

class moonpatrol : public machineBase
{
public:
    moonpatrol() { }
    ~moonpatrol() { }

    signed char machineType() override { return MCH_MOONPATROL; }

    const int   renderWidth()  override { return 240; }
    const int   renderBuffer() override { return 240 * 2 * 8; }

    unsigned char rdZ80(unsigned short Addr) override;
    void wrZ80(unsigned short Addr, unsigned char Value) override;
    unsigned char opZ80(unsigned short Addr) override;
    void outZ80(unsigned short Port, unsigned char Value) override;
    unsigned char inZ80(unsigned short Port) override;

    void init(Input *input, unsigned short *framebuffer, sprite_S *spritebuffer, unsigned char *memorybuffer) override;
    void reset() override;

    void run_frame(void) override;
    void prepare_frame(void) override;
    void render_row(short row) override;
    const unsigned short *logo(void) override;

#ifdef LED_PIN
    void menuLeds(CRGB *leds) override;
    void gameLeds(CRGB *leds) override;
#endif

    // M6803 sound CPU memory access (called from C callbacks)
    uint8_t snd_read(uint16_t addr);
    void snd_write(uint16_t addr, uint8_t val);
    void snd_port_write(uint8_t port, uint8_t val);
    uint8_t snd_port_read(uint8_t port);

    // Sound command logger — accessibile da src.ino per il debug
    unsigned char snd_cmd_log[MP_SND_LOG_SIZE];
    unsigned char snd_cmd_hit[MP_SND_LOG_SIZE];  // contatore occorrenze per debug
    unsigned char snd_cmd_count;

protected:
    void blit_tile(short row, char col) override { }
    void blit_sprite(short row, unsigned char s) override { }

private:
    // Anti-flicker: ultimo sprite valido per slot HW + eta'. Vedi MP_SPRITE_PERSIST.
    sprite_S      mp_slot_spr[MP_SPRITE_SLOTS];
    unsigned char mp_slot_age[MP_SPRITE_SLOTS];

    void render_backgrounds(short row);
    void render_fg_tiles(short row, bool opaque);
    void render_sprites(short row);

    // Scroll / video registers (I/O ports)
    unsigned char fg_scroll;        // foreground scroll (bottom quarter only)
    unsigned char bg1_xpos_lo;      // BG1 X position
    unsigned char bg1_ypos;         // BG1 Y position
    unsigned char bg2_xpos_lo;      // BG2 X position
    unsigned char bg2_ypos;         // BG2 Y position
    unsigned char bg_control;       // background control register
    unsigned char flipscreen;
    unsigned char sound_cmd;

    // Protection circuit state
    unsigned char bg1_xpos_full;    // full 8-bit value for protection calc

    // M6803 sound CPU state
    m6803_state snd_cpu;
    unsigned char snd_port1;        // last port1 value (data bus for AY)
    unsigned char snd_port2;        // last port2 value (control signals)
    unsigned char ay_addr[2];       // AY address latches (AY#1=45M, AY#2=45L)
    unsigned char ay_regs[2][16];   // AY register mirrors

    // MSM5205 ADPCM decoder state (drum/percussion sounds)
    void msm5205_decode(unsigned char nibble);

public:
    volatile int16_t msm_signal;       // current ADPCM output (-2048..2047)
    volatile uint8_t mp_trig_explo;    // trigger esplosione (colpo masso/nemico)
    volatile uint8_t mp_trig_death;    // trigger esplosione macchina
    volatile uint8_t mp_trig_rhythm;   // trigger click ritmico
    volatile uint8_t mp_player_active; // diventa 1 quando il gioco è operativo
    volatile uint8_t mp_first_cmd;     // primo comando (debug)
    volatile uint16_t mp_start_mute;   // contatore mute avvio
    int16_t msm_campione(void);        // task audio: il prossimo campione ADPCM (4 kHz)

private:
    int msm_step_idx;
    int msm_step;
    unsigned char msm_data;         // latched ADPCM nibble
    bool msm_data_pending;
    // ⭐ 26/09 ESPLOSIONI: il MSM5205 come sul CABINATONE (mpatrol.cpp).
    bool    msm_reset;              // AY 45M porta B (R15) bit 0: 1 = MSM FERMO
    uint8_t msm_mode;               // AY 45M porta B (R15) bit 2-4: playmode
    void    msm_vck(void);          // un fronte del VCK: decodifica + campione in coda
    // Il M6803 gira a blocchi (un quadro alla volta) e il task audio legge a
    // 32 kHz: i campioni del VCK passano per questa coda (1 produttore, 1
    // consumatore). 256 = 64 ms a 4 kHz; l'indice a 8 bit si avvolge da solo.
    int16_t msm_coda[256];
    volatile uint8_t msm_coda_w, msm_coda_r;

#ifdef LED_PIN
    const CRGB menu_leds[7] = { LED_BLUE, LED_CYAN, LED_BLUE, LED_YELLOW, LED_BLUE, LED_CYAN, LED_BLUE };
#endif
};

// Global pointer for M6803 callbacks
extern moonpatrol *g_moonpatrol_instance;

#endif // ENABLE_MOONPATROL
#endif // MOONPATROL_H
