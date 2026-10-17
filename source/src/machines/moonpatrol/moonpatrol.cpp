/*
 * Moon Patrol (Irem, 1982) - GALAGINO emulator implementation
 * Hardware: Irem M52 board - Z80 @ 3.072MHz + M6803 audio + 2x AY-3-8910
 * Video: 240x252 visible, ROT0 (landscape), 3 parallax BG layers
 * Tilemap: 32x32 tiles (8x8, 2bpp), partial foreground scroll
 * Sprites: 16x16, 3bpp, split-screen rendering
 *
 * Memory map (Main CPU):
 *   0x0000-0x3FFF: ROM (16KB)
 *   0x8000-0x83FF: Video RAM (1KB, tile codes)
 *   0x8400-0x87FF: Color RAM (1KB, tile attributes)
 *   0x8800:        Protection read
 *   0xC800-0xCBFF: Sprite RAM (256 bytes used, mirrored)
 *   0xD000:        IN0 (R) / Sound command (W)
 *   0xD001:        IN1 (R) / Flip screen (W)
 *   0xD002:        IN2 (R)
 *   0xD003:        DSW1 (R)
 *   0xD004:        DSW2 (R)
 *   0xE000-0xE7FF: Work RAM (2KB)
 *
 * I/O Ports:
 *   0x00-0x1F: FG scroll X (bottom quarter only)
 *   0x40-0x5F: BG1 X position
 *   0x60-0x7F: BG1 Y position
 *   0x80-0x9F: BG2 X position
 *   0xA0-0xBF: BG2 Y position
 *   0xC0-0xDF: BG control
 */

#include "config.h"
#include "machines/moonpatrol/moonpatrol.h"

#ifdef ENABLE_MOONPATROL

// ============================================================
// Z80 CPU interface
// ============================================================

unsigned char moonpatrol::opZ80(unsigned short Addr) {
    if (Addr < 0x4000)
        return moonpatrol_rom[Addr];
    return 0xFF;
}

unsigned char moonpatrol::rdZ80(unsigned short Addr) {
    // ROM: 0x0000-0x3FFF
    if (Addr < 0x4000)
        return moonpatrol_rom[Addr];

    // Video RAM: 0x8000-0x83FF
    if (Addr >= 0x8000 && Addr <= 0x83FF)
        return memory[MP_MEM_VRAM + (Addr - 0x8000)];

    // Color RAM: 0x8400-0x87FF
    if (Addr >= 0x8400 && Addr <= 0x87FF)
        return memory[MP_MEM_CRAM + (Addr - 0x8400)];

    // Protection circuit: popcount(bg1_xpos & 0x7F) XOR (bg1_xpos >> 7)
    if (Addr == 0x8800) {
        unsigned char val = bg1_xpos_full;
        unsigned char low7 = val & 0x7F;
        unsigned char bit7 = (val >> 7) & 1;
        // popcount of lower 7 bits
        unsigned char cnt = 0;
        while (low7) { cnt += low7 & 1; low7 >>= 1; }
        return cnt ^ bit7;
    }

    // Sprite RAM: 0xC800-0xCBFF (mirrored to 0xCFFF)
    if (Addr >= 0xC800 && Addr <= 0xCFFF)
        return memory[MP_MEM_SPRITES + ((Addr - 0xC800) & 0xFF)];

    // Input ports
    if (Addr == 0xD000) {
        // IN0: SYSTEM — active LOW
        unsigned char keymask = input->buttons_get();
        unsigned char val = 0xFF;
        if (keymask & BUTTON_START)  val &= ~0x01;  // START1
        if (keymask & BUTTON_COIN)   val &= ~0x08;  // COIN1
        return val;
    }
    if (Addr == 0xD001) {
        // IN1: P1 controls — active LOW
        // Moon Patrol: 2-way joystick (left/right) + 2 buttons (shoot, jump)
        unsigned char keymask = input->buttons_get();
        unsigned char val = 0xFF;
        if (keymask & BUTTON_RIGHT)  val &= ~0x01;
        if (keymask & BUTTON_LEFT)   val &= ~0x02;
        if (keymask & BUTTON_FIRE)   val &= ~0x80;  // Button 1 (shoot)
        if (keymask & BUTTON_UP)     val &= ~0x20;  // Button 2 (jump) mapped to UP
        if (keymask & BUTTON_EXTRA)  val &= ~0x20;  // Button 2 (jump) mapped to EXTRA
        return val;
    }
    if (Addr == 0xD002)
        return 0xFF;  // P2 (not used in single player)
    if (Addr == 0xD003)
        return MOONPATROL_DSW1;
    if (Addr == 0xD004)
        return MOONPATROL_DSW2;

    // Work RAM: 0xE000-0xE7FF
    if (Addr >= 0xE000 && Addr <= 0xE7FF)
        return memory[MP_MEM_WORKRAM + (Addr - 0xE000)];

    return 0xFF;
}

void moonpatrol::wrZ80(unsigned short Addr, unsigned char Value) {
    // Video RAM: 0x8000-0x83FF
    if (Addr >= 0x8000 && Addr <= 0x83FF) {
        memory[MP_MEM_VRAM + (Addr - 0x8000)] = Value;
        return;
    }

    // Color RAM: 0x8400-0x87FF
    if (Addr >= 0x8400 && Addr <= 0x87FF) {
        memory[MP_MEM_CRAM + (Addr - 0x8400)] = Value;
        return;
    }

    // Sprite RAM: 0xC800-0xCFFF
    if (Addr >= 0xC800 && Addr <= 0xCFFF) {
        memory[MP_MEM_SPRITES + ((Addr - 0xC800) & 0xFF)] = Value;
        return;
    }

    // Sound command
    if (Addr == 0xD000) {
        sound_cmd = Value;
        if ((Value & 0x80) == 0)
            m6803_irq(&snd_cpu);  // IRQ when bit7=0

        switch (Value) {
            case 0x98: mp_player_active = 1; break;  // avvio gioco operativo
            case 0x01: if (mp_player_active && mp_trig_explo < 255) mp_trig_explo++; break;
            case 0x03: if (mp_player_active && mp_trig_explo < 255) mp_trig_explo++; break;
            case 0x18: if (mp_player_active && mp_trig_death < 255) mp_trig_death++; break;
            case 0x1F: if (mp_player_active && mp_trig_death < 255) mp_trig_death++; break;
            case 0x80:
                mp_trig_explo = 0;
                mp_trig_death = 0;
                mp_trig_rhythm = 0;
                mp_player_active = 0;
                break;
        }
        return;
    }

    // Flip screen / coin counters
    if (Addr == 0xD001) {
        flipscreen = Value & 1;
        if (!game_started && (Value & 0x02))
            game_started = 1;
        return;
    }

    // Work RAM: 0xE000-0xE7FF
    if (Addr >= 0xE000 && Addr <= 0xE7FF) {
        memory[MP_MEM_WORKRAM + (Addr - 0xE000)] = Value;
        return;
    }
}

// ── I/O port writes (scroll & background control) ──
void moonpatrol::outZ80(unsigned short Port, unsigned char Value) {
    unsigned char port = Port & 0xFF;

    if (port <= 0x1F) {
        fg_scroll = Value;        return;
    }
    if (port >= 0x40 && port <= 0x5F) {
        bg1_xpos_lo = Value;
        bg1_xpos_full = Value;
        return;
    }
    if (port >= 0x60 && port <= 0x7F) {
        bg1_ypos = Value;
        return;
    }
    if (port >= 0x80 && port <= 0x9F) {
        bg2_xpos_lo = Value;
        return;
    }
    if (port >= 0xA0 && port <= 0xBF) {
        bg2_ypos = Value;
        return;
    }
    if (port >= 0xC0 && port <= 0xDF) {
        bg_control = Value;
        return;
    }
}

unsigned char moonpatrol::inZ80(unsigned short Port) {
    return 0xFF;
}

// ============================================================
// M6803 Sound CPU — DISABILITATO
// ============================================================

moonpatrol *g_moonpatrol_instance = nullptr;

// Le callback restano definite per evitare errori di linking
// ma non vengono registrate né chiamate
static uint8_t moonpatrol_snd_read(uint16_t addr) {
    return g_moonpatrol_instance->snd_read(addr);
}
static void moonpatrol_snd_write(uint16_t addr, uint8_t val) {
    g_moonpatrol_instance->snd_write(addr, val);
}
static uint8_t moonpatrol_snd_port_read(uint8_t port) {
    return g_moonpatrol_instance->snd_port_read(port);
}
static void moonpatrol_snd_port_write(uint8_t port, uint8_t val) {
    g_moonpatrol_instance->snd_port_write(port, val);
}

void moonpatrol::init(Input *inp, unsigned short *fb, sprite_S *sb, unsigned char *mb) {
    machineBase::init(inp, fb, sb, mb);
    g_moonpatrol_instance = this;
}

void moonpatrol::reset() {
    machineBase::reset();
    // Render diretto per-banda (come FULL33): nessun buffer full-frame nativo.
    // Anti-flicker: nessuno slot ha cronologia valida all'avvio.
    for (int i = 0; i < MP_SPRITE_SLOTS; i++) mp_slot_age[i] = MP_SPRITE_PERSIST;
    fg_scroll = 0;
    bg1_xpos_lo = 0;
    bg1_ypos = 0;
    bg2_xpos_lo = 0;
    bg2_ypos = 0;
    bg_control = 0;
    bg1_xpos_full = 0;
    flipscreen = 0;
    sound_cmd = 0;
    snd_port1 = 0;
    snd_port2 = 0;
    memset(ay_addr, 0, sizeof(ay_addr));
    memset(ay_regs, 0, sizeof(ay_regs));
    msm_signal = 0;
    msm_step_idx = 0;
    msm_step = 16;
    msm_data = 0;
    msm_data_pending = false;
    msm_reset = true;               // fermo finche' il driver non scrive R15
    msm_mode = 4;                   // S96 a 4 bit: il default del driver
    msm_coda_w = msm_coda_r = 0;

    g_moonpatrol_instance = this;

    // Reset flag suoni discreti
    mp_trig_explo = 0;
    mp_trig_death = 0;
    mp_trig_rhythm = 0;
    mp_player_active = 0;

    // M6803 callbacks e reset
    m6803_ext_read_fn  = moonpatrol_snd_read;
    m6803_ext_write_fn = moonpatrol_snd_write;
    m6803_port_read_fn = moonpatrol_snd_port_read;
    m6803_port_write_fn = moonpatrol_snd_port_write;
    m6803_reset(&snd_cpu);
    snd_cpu.irq_pending = 1;
}

uint8_t moonpatrol::snd_read(uint16_t addr) {
    if (addr == 0x0800)
        return sound_cmd;
    if (addr >= 0xC000)
        return moonpatrol_snd_rom[addr - 0xC000];
    return 0xFF;
}

// MSM5205 ADPCM step size table (49 entries)
static const int msm5205_step_table[49] = {
    16, 17, 19, 21, 23, 25, 28, 31, 34, 37, 41, 45, 50, 55, 60, 66,
    73, 80, 88, 97, 107, 118, 130, 143, 157, 173, 190, 209, 230, 253,
    279, 307, 337, 371, 408, 449, 494, 544, 598, 658, 724, 796, 876,
    963, 1060, 1166, 1282, 1411, 1552
};
static const int msm5205_index_shift[8] = { -1, -1, -1, -1, 2, 4, 6, 8 };

// ⭐⭐ 26/09 ESPLOSIONI DEI MASSI. Sono ADPCM, non AY: MISURATO col banco del
// CABINATONE (`_verify_gfx/mpatrol_harness/mk_mpcomandi.py inietta`): i comandi
// 0x01/0x02/0x03 (0x01 = masso colpito, Z80 a 0x1768) portano R15 dell'AY 45M
// da 0x13 a 0x12 = il MSM5205 esce dal RESET e suona ~0,5 s. Qui l'ADPCM non
// suonava MAI, per tre motivi:
//   1) il driver scrive il campione a 0x0801 e si decodificava solo sotto 0x0800;
//   2) il RESET (R15 bit 0) era ignorato: il driver scrive un campione a OGNI
//      NMI anche a MSM fermo (72 per quadro) — per questo la vecchia prova con
//      0x0801 «gracchiava», e poi era stata spenta con mp_player_active;
//   3) si decodificava all'istante della scrittura, e il task audio leggeva un
//      solo valore per quadro: ora la decodifica e' sul VCK (= l'NMI) e i
//      campioni passano per msm_coda.
// La decodifica e' quella di msm5205.cpp (compute_tables/update_adpcm), come
// msm_vck() in mpatrol.cpp del CABINATONE.
void moonpatrol::msm5205_decode(unsigned char nibble) {
    const int stepval = msm5205_step_table[msm_step_idx];
    int diff = stepval / 8;
    if (nibble & 4) diff += stepval;
    if (nibble & 2) diff += stepval / 2;
    if (nibble & 1) diff += stepval / 4;
    int s = msm_signal + ((nibble & 8) ? -diff : diff);
    if (s > 2047) s = 2047; else if (s < -2048) s = -2048;
    msm_signal = (int16_t)s;
    msm_step_idx += msm5205_index_shift[nibble & 7];
    if (msm_step_idx < 0) msm_step_idx = 0;
    if (msm_step_idx > 48) msm_step_idx = 48;
    msm_step = msm5205_step_table[msm_step_idx];
}

void moonpatrol::msm_vck(void) {
    if (msm_reset) {
        msm_step_idx = 0;
        msm_step = msm5205_step_table[0];
        msm_signal = 0;
    } else {
        msm5205_decode((msm_mode & 4) ? (msm_data & 15) : ((msm_data & 7) << 1));
    }
    const uint8_t w = msm_coda_w;
    if ((uint8_t)(w + 1) != msm_coda_r) {       // piena: si perde il campione
        msm_coda[w] = msm_signal;
        msm_coda_w = (uint8_t)(w + 1);
    }
}

// Task audio, a 32 kHz: un campione nuovo ogni 8 (= 4 kHz, il VCK in S96).
// Coda vuota -> resta sull'ultimo valore; troppo piena (il gioco e' andato
// avanti di piu' quadri) -> si salta avanti, per non accumulare ritardo.
int16_t moonpatrol::msm_campione(void) {
    static uint8_t sotto = 0;
    static int16_t ultimo = 0;
    if (++sotto < 8) return ultimo;
    sotto = 0;
    uint8_t r = msm_coda_r;
    const uint8_t w = msm_coda_w;
    if ((uint8_t)(w - r) > 160) r = (uint8_t)(w - 80);
    if (r != w) {
        ultimo = msm_coda[r];
        msm_coda_r = (uint8_t)(r + 1);
    }
    return ultimo;
}

void moonpatrol::snd_write(uint16_t addr, uint8_t val) {
    // IRQ acknowledge
    if (addr == 0x0800) {
        if ((sound_cmd & 0x80) != 0)
            snd_cpu.irq_pending = 0;
        return;
    }
    // m52_adpcm_w: gli indirizzi DISPARI sotto 0x1000 vanno al MSM (il driver
    // usa 0x0801). Solo il LATCH: la decodifica la fa il VCK (msm_vck).
    if (addr < 0x1000 && (addr & 1)) {
        msm_data = val;
        return;
    }
}

uint8_t moonpatrol::snd_port_read(uint8_t port) {
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
    if (port == 2)
        return 0x00;
    return 0xFF;
}

void moonpatrol::snd_port_write(uint8_t port, uint8_t val) {
    if (port == 1) {
        snd_port1 = val;
        return;
    }
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
                    // ⭐ 25/09 BATTERIA: ogni scrittura del registro 13 (forma
                    // dell'inviluppo) FA RIPARTIRE l'inviluppo, anche se il valore
                    // e' lo stesso di prima — e Moon Patrol riscrive sempre 0x09 a
                    // ogni colpo. audio.cpp legge solo la copia dei registri e non
                    // vedrebbe la scrittura: si CONTA qui, in soundregs[14] (cella
                    // che questo gioco non usa: si scrivono solo i registri 0..13).
                    if (reg == 13) soundregs[14]++;
                    // AY 45M, porta B (R15): ay8910_45M_portb_w -> MSM5205
                    // (bit 2-4 playmode, bit 0 reset). 0x13 = fermo, 0x12 = suona.
                    if (reg == 15) {
                        msm_mode  = (snd_port1 >> 2) & 7;
                        msm_reset = (snd_port1 & 1) != 0;
                    }
                }
                if (snd_port2 & 0x10) {
                    unsigned char reg = ay_addr[1] & 0x0F;
                    ay_regs[1][reg] = snd_port1;
                    if (reg < 14) soundregs[16 + reg] = snd_port1;
                    if (reg == 13) soundregs[16 + 14]++;   // idem per il secondo AY
                }
            }
            m6803_internal_ram[0x3D] = 1;
        }
        snd_port2 = val;
        return;
    }
}

// ============================================================
// Frame execution: solo Z80, M6803 disabilitato
// ============================================================

void moonpatrol::run_frame(void) {
    // Main Z80 @ 3.072MHz
    current_cpu = 0;
    for (int i = 0; i < INST_PER_FRAME; i++) {
        StepZ80(&cpu[0]); StepZ80(&cpu[0]);
        StepZ80(&cpu[0]); StepZ80(&cpu[0]);

        // IRQ at vblank (end of frame)
        if (i == INST_PER_FRAME - 1)
            IntZ80(&cpu[0], INT_IRQ);
    }

    // M6803 sound CPU: batch-process ~14900 cycles per frame
    {
        static int nmi_cycle_cnt = 0;
        static int nmi_count = 0;
        int snd_budget = 14900;
        while (snd_budget > 0) {
            int cyc = m6803_step(&snd_cpu);
            snd_budget -= cyc;
            nmi_cycle_cnt += cyc;
            if (nmi_cycle_cnt >= 224) {
                nmi_cycle_cnt -= 224;
                // l'NMI E' il VCK del MSM5205 (invertitore NPN): prima il
                // campione, poi il driver scrive il prossimo a 0x0801
                msm_vck();
                m6803_nmi(&snd_cpu);
                // Ritmo: click ogni 250 NMI (~16Hz) solo quando gioco operativo
                if (mp_player_active) {
                    nmi_count++;
                    if (nmi_count >= 500) {
                        nmi_count = 0;
                        if (mp_trig_rhythm < 255) mp_trig_rhythm++;
                    }
                } else {
                    nmi_count = 0;
                }
            }
            uint16_t old_tc = snd_cpu.timer_counter;
            snd_cpu.timer_counter += cyc;
            if (snd_cpu.timer_counter < old_tc) snd_cpu.tcsr |= 0x20;
            if (old_tc <= snd_cpu.timer_output_compare &&
                snd_cpu.timer_counter >= snd_cpu.timer_output_compare)
                snd_cpu.tcsr |= 0x40;
        }
    }
}

// ============================================================
// Sprite preparation
// ============================================================

void moonpatrol::prepare_frame(void) {
    // Driver long-press reset (regola SPINNERINO: chiamare buttons_get ogni frame).
    if (input) input->buttons_get();

    active_sprites = 0;

    unsigned char *sram = memory + MP_MEM_SPRITES;

    // Sprite parse VERBATIM FULL33 + ANTI-FLICKER per-slot (vedi MP_SPRITE_PERSIST).
    // 4 gruppi x 16 slot. Slot valido -> disegna (posizione corrente), eta'=0;
    // slot spento ma valido entro PERSIST frame -> ridisegna l'ultimo sprite di
    // quello slot (i nemici che lampeggiano a frame alterni restano completi).
    for (int group = 0; group < 4; group++) {
        int base = group * 0x40;
        for (int offs = base + 0x3C; offs >= base; offs -= 4) {
            int slot = group * 16 + (offs - base) / 4;   // 0..63
            bool valid_now = false;
            struct sprite_S spr;

            unsigned char sy_raw = sram[offs];
            unsigned char attr   = sram[offs + 1];
            unsigned char code   = sram[offs + 2];
            unsigned char sx_raw = sram[offs + 3];

            if (!(sy_raw == 0 && sx_raw == 0)) {
                int sy = 257 - (int)sy_raw;
                int sx = (int)sx_raw;

                if (!(sy <= -16 || sy >= 260)) {
                    spr.code = code;
                    spr.color = attr & 0x0F;

                    bool flipx = (attr & 0x40) != 0;
                    bool flipy = (attr & 0x80) != 0;
                    spr.flags = (flipx ? 1 : 0) | (flipy ? 2 : 0);

                    spr.x = sx - 8;
                    spr.y = sy;

                    if ((spr.x > -16) && (spr.x < 256) &&
                        (spr.y > -16) && (spr.y < 288))
                        valid_now = true;
                }
            }

            if (valid_now) {
                mp_slot_spr[slot] = spr;
                mp_slot_age[slot] = 0;
                if (active_sprites < 64) sprite[active_sprites++] = spr;
            } else if (mp_slot_age[slot] < MP_SPRITE_PERSIST) {
                mp_slot_age[slot]++;
                if (active_sprites < 64) sprite[active_sprites++] = mp_slot_spr[slot];
            }
        }
    }
    // Niente mp_full: render_row disegna ogni banda direttamente (FULL33).
}

// ============================================================
// Background rendering (3 parallax layers)
// ============================================================

// Render DIRETTO in frame_buffer (stride 240), come FULL33: pannello in PORTRAIT,
// righe native da 240 px. render_row valido per row 2..33 (game_y 0..255).
void moonpatrol::render_backgrounds(short row) {
    int game_row_base = (row - 2);

    for (int line = 0; line < 8; line++) {
        int game_y = game_row_base * 8 + line;
        unsigned short *ptr = frame_buffer + line * 240;

        memset(ptr, 0, 240 * 2);

        if (bg_control & 0x20) continue;

        if (!(bg_control & 0x10)) {
            int scr_y = (int)bg2_ypos + 16;
            int dst_y = (int)game_y + 22 - scr_y;
            if (dst_y >= 0 && dst_y < MP_BG_H) {
                int row_off = dst_y * 64;
                int scr_x = (int)bg2_xpos_lo + 124;
                for (int px = 0; px < 240; px++) {
                    int src_x = ((int)px + 136 - scr_x) & 0xFF;
                    unsigned char raw = moonpatrol_bg0[row_off + (src_x >> 2)];
                    int sub = 3 - (src_x & 3);
                    unsigned char pxval = ((raw >> sub) & 1) | (((raw >> (sub + 4)) & 1) << 1);
                    if (pxval)
                        ptr[px] = moonpatrol_bg_cmap[0][pxval];
                }
            } else if (dst_y >= MP_BG_H && dst_y < 128) {
                unsigned short fill = moonpatrol_bg_cmap[0][3];
                for (int px = 0; px < 240; px++)
                    ptr[px] = fill;
            }
        }

        {
            const unsigned char *bg_rom = NULL;
            int cmap_idx = -1;
            if (!(bg_control & 0x02)) {
                bg_rom = moonpatrol_bg1; cmap_idx = 1;
            } else if (!(bg_control & 0x04)) {
                bg_rom = moonpatrol_bg2; cmap_idx = 2;
            }
            if (bg_rom) {
                int scr_y = (int)bg1_ypos + 16;
                int dst_y = (int)game_y + 22 - scr_y;
                if (dst_y >= 0 && dst_y < MP_BG_H) {
                    int row_off = dst_y * 64;
                    int scr_x = (int)bg1_xpos_lo + 124;
                    for (int px = 0; px < 240; px++) {
                        int src_x = ((int)px + 136 - scr_x) & 0xFF;
                        unsigned char raw = bg_rom[row_off + (src_x >> 2)];
                        int sub = 3 - (src_x & 3);
                        unsigned char pxval = ((raw >> sub) & 1) | (((raw >> (sub + 4)) & 1) << 1);
                        if (pxval)
                            ptr[px] = moonpatrol_bg_cmap[cmap_idx][pxval];
                    }
                } else if (dst_y >= MP_BG_H && dst_y < 128) {
                    unsigned short fill = moonpatrol_bg_cmap[cmap_idx][3];
                    for (int px = 0; px < 240; px++)
                        ptr[px] = fill;
                }
            }
        }
    }
}

// ============================================================
// Foreground tile rendering
// ============================================================

void moonpatrol::render_fg_tiles(short row, bool opaque) {
    int game_row_base = (row - 2);

    for (int line = 0; line < 8; line++) {
        int game_y = game_row_base * 8 + line;
        unsigned short *ptr = frame_buffer + line * 240;

        int scroll = (game_y >= 192) ? fg_scroll : 0;

        int tile_row = (game_y >> 3) & 31;
        int tile_r = game_y & 7;

        for (int px = 0; px < 240; px++) {
            int game_x = (px + 8 - scroll) & 0xFF;

            int tile_col = (game_x >> 3) & 31;
            int tile_c = game_x & 7;

            int tile_idx = tile_row * 32 + tile_col;
            unsigned char vram_val = memory[MP_MEM_VRAM + tile_idx];
            unsigned char cram_val = memory[MP_MEM_CRAM + tile_idx];

            unsigned short tile_code = vram_val | ((cram_val & 0x80) << 1);
            if (tile_code >= MP_NTILES) tile_code = 0;

            unsigned char color = cram_val & 0x7F;

            unsigned short tile_row_data = moonpatrol_tilemap[tile_code][tile_r];
            unsigned char px_val = (tile_row_data >> (tile_c * 2)) & 3;

            if (opaque || px_val != 0) {
                unsigned short col = moonpatrol_tile_cmap[color][px_val];
                if (opaque) {
                    ptr[px] = col;
                } else if (col) {
                    ptr[px] = col;
                }
            }
        }
    }
}

// ============================================================
// Sprite rendering
// ============================================================

void moonpatrol::render_sprites(short row) {
    for (int s = 0; s < active_sprites; s++) {
        short spr_x = sprite[s].x;
        short spr_y = sprite[s].y;

        if ((spr_y >= 8 * (row + 1)) || ((spr_y + 16) <= 8 * row))
            continue;

        int code = sprite[s].code;
        if (code >= MP_NSPRITES) continue;

        int variant = sprite[s].flags & 3;
        const unsigned long *spr_gfx = moonpatrol_spritemap[variant][code];
        const unsigned short *colors = moonpatrol_spr_cmap[sprite[s].color & 0x0F];

        int band_top = 8 * row;

        for (int sub = 0; sub < 8; sub++) {
            int screen_y = band_top + sub;
            if (screen_y < spr_y || screen_y >= spr_y + 16) continue;

            int dy = screen_y - spr_y;

            for (int dx = 0; dx < 16; dx++) {
                int screen_x = spr_x + dx;
                if (screen_x < 0 || screen_x >= 240) continue;

                unsigned char px;
                if (dx < 8)
                    px = (spr_gfx[dy * 2] >> (dx * 4)) & 0x07;
                else
                    px = (spr_gfx[dy * 2 + 1] >> ((dx - 8) * 4)) & 0x07;

                if (px) {
                    unsigned short col = colors[px];
                    if (col)
                        frame_buffer[sub * 240 + screen_x] = col;
                }
            }
        }
    }
}

// ============================================================
// Main render function (FULL33 verbatim): row 2..33, diretto in frame_buffer.
// ============================================================

void moonpatrol::render_row(short row) {
    if (row < 2 || row >= 34) return;

    render_backgrounds(row);

    int game_row_base = (row - 2);
    int game_y_start = game_row_base * 8;
    if (game_y_start < 56) {
        render_fg_tiles(row, true);
    } else {
        render_fg_tiles(row, false);
    }

    render_sprites(row);
}

const unsigned short *moonpatrol::logo(void) {
    return moonpatrol_logo;
}

#ifdef LED_PIN
void moonpatrol::menuLeds(CRGB *leds) {
    memcpy(leds, menu_leds, NUM_LEDS * sizeof(CRGB));
}
void moonpatrol::gameLeds(CRGB *leds) {
    memcpy(leds, menu_leds, NUM_LEDS * sizeof(CRGB));
}
#endif

#endif // ENABLE_MOONPATROL
