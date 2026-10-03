// -O2 for the emulation hot path, as arkanoid.cpp (the project builds -Os)
#pragma GCC optimize("-O2")

#include "mpatrol.h"
#if defined(_MSC_VER)
#include <intrin.h>
#endif

// Moon Patrol. Every behaviour below cites the MAME function it was taken
// from - see mpatrol.h for the file list.

static_assert(MPATROL_MEM_END <= RAMSIZE, "RAMSIZE too low for mpatrol");

// MAME timing constants (attoseconds) - see "Scheduling" below.
#define MP_ATTOS       1000000000000000000ULL
#define MP_Z80_APC     325520833333ULL          // HZ_TO_ATTOSECONDS(3072000)
#define MP_Z80_SHIFT   8
#define MP_Z80_DIV     (MP_Z80_APC >> MP_Z80_SHIFT)
#define MP_SND_APC     1117459522822ULL         // HZ_TO_ATTOSECONDS((3579545 + 3) / 4)
#define MP_SND_SHIFT   10
// device_execute_interface::local_time() inside a run adds
// cycles_to_attotime(cycles) = clocks_to_attotime(cycles * 4) - through the
// 3.579545MHz clock, not MP_SND_APC. Timestamps of 6803 accesses (syncs,
// timer re-arms) use this.
#define MP_SND_LOCAL   (4 * 279365114840ULL)    // 4 * HZ_TO_ATTOSECONDS(3579545)
#define MP_SND_DIV     (MP_SND_APC >> MP_SND_SHIFT)
#define MP_QUANTUM     (MP_ATTOS / 60)          // default maximum quantum (60Hz)
#define MP_FRAME       (162760416666ULL * 384 * 282)   // HZ_TO_ATTOSECONDS(6144000) * htotal * vtotal
#define MP_SCAN        (MP_FRAME / 282)
#define MP_SND_UPDATE  (MP_ATTOS / 50)          // sound_manager STREAMS_UPDATE_ATTOTIME
#define MP_MSM_CLK     2604166666666ULL         // HZ_TO_ATTOSECONDS(384000)
#define MP_OUT_RATE    24000                    // audio.cpp output rate
static_assert(MP_Z80_DIV < (1ULL << 31) && (MP_Z80_APC >> (MP_Z80_SHIFT - 1)) >= (1ULL << 31), "Z80 divshift");
static_assert(MP_SND_DIV < (1ULL << 31) && (MP_SND_APC >> (MP_SND_SHIFT - 1)) >= (1ULL << 31), "6803 divshift");

mpatrol *mpatrol::s_instance = 0;
#if MPATROL_PROFILE && (!MPATROL_DBG_HARNESS || MPATROL_HOST_PROF)
static uint32_t pf_out = 0, pf_ev = 0, pf_key = 0, pf_ay = 0, pf_calls = 0;
#endif
#if MPATROL_HOST_PROF
unsigned long hp_steps = 0, hp_quiet = 0, hp_out = 0, hp_calls = 0, hp_ev_tone = 0, hp_ev_noise = 0, hp_ev_env = 0;
#endif

// Main CPU ROM, copied to internal RAM when the heap allows: flash reads on
// the emulation core stall behind the render core's gfx traffic (arkanoid).
static const unsigned char *mp_rom = mpatrol_rom;

// mpatrol_z80.c: the shared Z80 core built with this machine's ROM / RAM
// reads and writes inline (same opcodes, no call per memory byte)
extern "C" {
extern const unsigned char *mpz_rom;
extern unsigned char *mpz_mem;
void mpz_Step(Z80 *R);
void mpz_Int(Z80 *R, unsigned short Vector);
}
static void z80_info_init(void);   // z80_exec() opcode table, see z80_io_access

// Graphics and palettes for the render core, in RAM when the heap allows
// (else flash): with them in flash the render core keeps the flash bus busy
// and every cache miss of the emulation core waits behind it.
static const unsigned char (*mp_tx)[8][8] = mpatrol_tx;
static const unsigned char (*mp_spr)[16][16] = mpatrol_spr;
static const unsigned char (*mp_bg)[64][256] = mpatrol_bg;
static unsigned short DRAM_ATTR mp_tx_pal[512], mp_sp_pal[256], mp_bg_pal[12];
static unsigned char DRAM_ATTR mp_sp_clut[256];
// mp_tx_colmask[code][x]: bit y set when pixel (x, y) of the char is not pen 0
static unsigned char mp_tx_colmask[512][8];

// Cycle counter for MPATROL_PROFILE (one instruction, unlike micros())
static inline uint32_t mp_ccount(void)
{
#if defined(ARDUINO)
  uint32_t c;
  __asm__ __volatile__("rsr %0, ccount" : "=a"(c));
  return c;
#elif defined(_MSC_VER)
  return (uint32_t)__rdtsc();
#else
  return 0;
#endif
}

void mpatrol::reset()
{
  machineBase::reset();
  s_instance = this;

  // sound ROM to internal RAM when the heap allows (flash reads stall behind
  // the render core's gfx traffic)
  /*
  if (!snd_rom_ram)
  {
    snd_rom_ram = (unsigned char *)ownAlloc(malloc(0x1000));
    if (snd_rom_ram)
      memcpy(snd_rom_ram, mpatrol_snd, 0x1000);
  }
  snd_rom = snd_rom_ram ? snd_rom_ram : mpatrol_snd;
  if (!rom_ram)
  {
    rom_ram = (unsigned char *)ownAlloc(malloc(0x4000));
    if (rom_ram)
      memcpy(rom_ram, mpatrol_rom, 0x4000);
  }
  mp_rom = rom_ram ? rom_ram : mpatrol_rom;
  */
  
  snd_rom = mpatrol_snd;
  mp_rom = mpatrol_rom;
  mpz_rom = mp_rom;
  mpz_mem = memory;
  z80_info_init();
  /*
  for (int i = 0; i < 3; i++)
  {
    static const size_t size[3] = { sizeof(mpatrol_tx), sizeof(mpatrol_spr), sizeof(mpatrol_bg) };
    static const void *const src[3] = { mpatrol_tx, mpatrol_spr, mpatrol_bg };
    if (!gfx_ram[i])
    {
      gfx_ram[i] = (unsigned char *)ownAlloc(malloc(size[i]));
      if (gfx_ram[i])
        memcpy(gfx_ram[i], src[i], size[i]);
    }
  }
  */
  mp_tx = gfx_ram[0] ? (const unsigned char (*)[8][8])gfx_ram[0] : mpatrol_tx;
  mp_spr = gfx_ram[1] ? (const unsigned char (*)[16][16])gfx_ram[1] : mpatrol_spr;
  mp_bg = gfx_ram[2] ? (const unsigned char (*)[64][256])gfx_ram[2] : mpatrol_bg;
  memcpy(mp_tx_pal, mpatrol_tx_565, sizeof(mp_tx_pal));
  memcpy(mp_sp_pal, mpatrol_sp_565, sizeof(mp_sp_pal));
  memcpy(mp_bg_pal, mpatrol_bg_565, sizeof(mp_bg_pal));
  memcpy(mp_sp_clut, mpatrol_sp_clut, sizeof(mp_sp_clut));
  for (int code = 0; code < 512; code++)
    for (int px = 0; px < 8; px++)
    {
      unsigned char m = 0;
      for (int py = 0; py < 8; py++)
        if (mpatrol_tx[code][py][px])
          m |= 1 << py;
      mp_tx_colmask[code][px] = m;
    }
  printf("mpatrol: in RAM: rom %d snd %d tx %d spr %d bg %d\n", rom_ram != 0, snd_rom_ram != 0,
         gfx_ram[0] != 0, gfx_ram[1] != 0, gfx_ram[2] != 0);

  // m52_state::machine_reset()
  m_bgxpos[0] = m_bgxpos[1] = 0;
  m_bgypos[0] = m_bgypos[1] = 0;
  m_bgcontrol = 0;
  m_scroll = 0;
  m_scroll_written = false;
  m_flip = false;
  in_keys = 0;

  z80_irq = false;
  ei_shadow = false;
  z80_r = z80_r2 = 0;
  z80_part_done = 0;
  base = z80_t = snd_t = frame_start = exec_base = 0;
  exec_pos = 0;
  exec_cpu = 0;
  snd_update_next = MP_SND_UPDATE;
  time_seconds = 0;
  frame_timer = 0;
  sync_n = 0;
  abort_run = false;

  // ---- sound board. Power-on state first (device_start), then
  // device_reset() in device tree order: irem_audio, its CPU, the two AYs,
  // the MSM5205 (m52_soundc_audio_device::device_add_mconfig order).
  m_port1 = m_port2 = m_soundlatch = 0;          // irem_audio_device::device_reset
  memset(&snd, 0, sizeof(snd));
  snd.ctx = this;
  snd.read = snd_read;
  snd.write = snd_write;
  snd.port_r = snd_port_r;
  snd.port_w = snd_port_w;
  // 7000-7fff (and its mirror f000-ffff, global_mask 0x7fff) is the ROM
  snd.rom = snd_rom;
  snd.rom_match_mask = 0x7000;
  snd.rom_match = 0x7000;
  snd.rom_index_mask = 0x0fff;
  // m52_small_sound_map: map.global_mask(0x7fff) - covers the 6803's own
  // registers and RAM too (8000-80ff = 0000-00ff)
  snd.addr_mask = 0x7fff;
  // fcf5: lda $bd / beq $fcf5 - waits for the NMI handler (see m6801.h)
  snd.idle_pc = MPATROL_IDLE_SKIP ? 0xfcf5 : 0;
  snd.idle_addr = 0xbd;
  m6801_power_on(&snd);                          // m6800/m6801 device_start + device_reset

  // msm5205: constructor S96_4B (set_prescaler_selector), device_start ->
  // notify_clock_changed() starts VCK at /96 from time 0 ...
  msm_s1 = false;
  msm_s2 = false;
  msm_bitwidth = 4;
  msm_vck_period = 0;
  msm_capture_next = 0;
  msm_clock_changed(0);
  // ... then the AY resets: ay8910_reset_ym() writes R7 = 0 with
  // m_last_enable = 0xc0, so port B (set to input) calls
  // ay8910_45M_portb_w(0xff): MSM slave mode (VCK timer stopped), reset on.
  pace_init = false;
  snd_out_reset();
  ay_reset(0);
  ay_reset(1);
  // msm5205_device::device_reset() (after the AYs)
  msm_data = 0;
  msm_vck = false;
  msm_reset = false;
  msm_signal = 0;
  msm_f = 0;
  msm_step = 0;


  // irem_audio_device::device_reset(): m_cpu->set_input_line(0, ASSERT_LINE)
  // - synced, applied before the first timeslice
  m6801_set_input(&snd, M6801_IRQ_LINE, 1);

  memset(snap, 0, sizeof(snap));
  snap_front = 0;
  render_snap = 0;

  game_started = 1;
}

// ============================================================================
// Main CPU memory map - m52_state::main_map / main_portmap (m52.cpp)
// ============================================================================

unsigned char IRAM_ATTR mpatrol::opZ80(unsigned short Addr)
{
  return Addr < 0x4000 ? mp_rom[Addr] : rdZ80(Addr);
}

unsigned char IRAM_ATTR mpatrol::rdZ80(unsigned short Addr)
{
  if (Addr < 0x4000)                         // map(0x0000, 0x3fff).rom()
    return mp_rom[Addr];
  switch (Addr >> 11)
  {
  case 0x10:                                 // 8000-87ff videoram / colorram
    return memory[MPATROL_VIDEORAM + (Addr & 0x7ff)];
  case 0x11:                                 // 8800 protection_r, mirror 0x07ff
  {
    // result = popcount(bgxpos[0] & 0x7f) ^ (bgxpos[0] >> 7)
    int popcount = 0;
    for (int temp = m_bgxpos[0] & 0x7f; temp != 0; temp >>= 1)
      popcount += temp & 1;
    return popcount ^ (m_bgxpos[0] >> 7);
  }
  case 0x1a:                                 // d000-d7ff, inputs mirror 0x07f8
  {
    unsigned char k = in_keys;
    switch (Addr & 7)
    {
    case 0:                                  // IN0
    {
      unsigned char v = 0xff;
      if (k & BUTTON_START) v &= ~0x01;      // IPT_START1
      if (k & BUTTON_COIN)  v &= ~0x08;      // IPT_COIN1
      return v;
    }
    case 1:                                  // IN1 (mpatrol: 2-way joystick)
    {
      unsigned char v = 0xff;
      if (k & BUTTON_RIGHT) v &= ~0x01;      // IPT_JOYSTICK_RIGHT
      if (k & BUTTON_LEFT)  v &= ~0x02;      // IPT_JOYSTICK_LEFT
      if (k & (BUTTON_UP | BUTTON_EXTRA)) v &= ~0x20;   // IPT_BUTTON2
      if (k & BUTTON_FIRE)  v &= ~0x80;      // IPT_BUTTON1
      return v;
    }
    case 2:  return 0xff;                    // IN2 (cocktail side, COIN2)
    case 3:  return MPATROL_DSW1;
    case 4:  return MPATROL_DSW2;
    default: return 0x00;                    // d005-d007: unmapped
    }
  }
  case 0x1c:                                 // e000-e7ff RAM
    return memory[MPATROL_WORKRAM + (Addr & 0x7ff)];
  default:                                   // unmapped (incl. c800-cfff write-only): 0
    return 0x00;
  }
}

void IRAM_ATTR mpatrol::wrZ80(unsigned short Addr, unsigned char Value)
{
  switch (Addr >> 11)
  {
  case 0x10:                                 // videoram_w / colorram_w
    memory[MPATROL_VIDEORAM + (Addr & 0x7ff)] = Value;
    return;
  case 0x19:                                 // c800-cbff spriteram, mirror 0x0400
    memory[MPATROL_SPRITERAM + (Addr & 0x3ff)] = Value;
    return;
  case 0x1a:                                 // d000-d7ff, mirror 0x07fc
    if ((Addr & 3) == 0)
    {
      // irem_audio_device::cmd_w()
      m_soundlatch = Value;
#if MPATROL_DBG_HARNESS
      if (dbg_cmd_hook)
        dbg_cmd_hook(Value, dbg_cycles(cur_time() + (uint64_t)exec_io_offset * MP_Z80_APC));
#endif
      if ((Value & 0x80) == 0)
        line_w(LINE_SND_IRQ, true, cur_time() + (uint64_t)exec_io_offset * MP_Z80_APC);
    }
    else if ((Addr & 3) == 1)
    {
      // flipscreen_w(): (data & 1) ^ (~DSW2 & 1); coin counters not emulated
      m_flip = ((Value & 0x01) ^ (~MPATROL_DSW2 & 0x01)) != 0;
    }
    return;
  case 0x1c:                                 // e000-e7ff RAM
    memory[MPATROL_WORKRAM + (Addr & 0x7ff)] = Value;
    return;
  default:
    return;
  }
}

// main_portmap (global_mask 0xff)
void IRAM_ATTR mpatrol::outZ80(unsigned short Port, unsigned char Value)
{
  switch (Port & 0xe0)
  {
  case 0x00:                                 // scroll_w, mirror 0x1f
    m_scroll = Value;
    m_scroll_written = true;
    break;
  case 0x40: m_bgxpos[0] = Value; break;     // bgxpos_w<0>
  case 0x60: m_bgypos[0] = Value; break;     // bgypos_w<0>
  case 0x80: m_bgxpos[1] = Value; break;     // bgxpos_w<1>
  case 0xa0: m_bgypos[1] = Value; break;     // bgypos_w<1>
  case 0xc0: m_bgcontrol = Value; break;     // bgcontrol_w
  default: break;
  }
}

// ============================================================================
// Sound board - irem_audio_device (irem.cpp), m52_small_sound_map
// ============================================================================

// map.global_mask(0x7fff): 0000-0fff w m52_adpcm_w, 1000-1fff w
// sound_irq_ack_w, 2000-7fff rom. The 6803's own registers (0000-0014) and
// RAM (0080-00ff) are handled in the core before this is reached.
uint8_t IRAM_ATTR mpatrol::snd_read(void *ctx, uint16_t addr)
{
  mpatrol *m = (mpatrol *)ctx;
  addr &= 0x7fff;
  if (addr >= 0x7000)
    return m->snd_rom[addr - 0x7000];
  return 0x00;                               // 2000-6fff: empty region (0); below: unmapped
}

void IRAM_ATTR mpatrol::snd_write(void *ctx, uint16_t addr, uint8_t data)
{
  mpatrol *m = (mpatrol *)ctx;
  addr &= 0x7fff;
  if (addr < 0x1000)
  {
    // m52_adpcm_w(): offset & 1 -> msm1 data_w; offset & 2 -> msm2 (none here)
    if (addr & 1)
    {
      // msm5205_device::data_w()
      m->msm_data = (m->msm_bitwidth == 4) ? (data & 0x0f) : ((data & 0x07) << 1);
#if MPATROL_DBG_HARNESS
      if (m->dbg_msm_hook)
        m->dbg_msm_hook('D', data, m->dbg_cycles(m->cur_time()));
#endif
    }
  }
  else if (addr < 0x2000)
    m->sound_irq_ack_w();
}

// irem_audio_device::sound_irq_ack_w()
void IRAM_ATTR mpatrol::sound_irq_ack_w(void)
{
  if ((m_soundlatch & 0x80) != 0)
    line_w(LINE_SND_IRQ, false, cur_time());
}

// in_p1_cb / in_p2_cb
uint8_t IRAM_ATTR mpatrol::snd_port_r(void *ctx, int port)
{
  mpatrol *m = (mpatrol *)ctx;
  if (port == 0)
    return m->port1_r();
  if (port == 1)
    return 0x00;                             // m6803_port2_r()
  return 0xff;
}

// out_p1_cb / out_p2_cb
void IRAM_ATTR mpatrol::snd_port_w(void *ctx, int port, uint8_t data, uint8_t ddr)
{
  mpatrol *m = (mpatrol *)ctx;
  if (port == 0)
    m->m_port1 = data;                       // m6803_port1_w()
  else if (port == 1)
    m->port2_w(data);
}

// irem_audio_device::m6803_port1_r()
unsigned char IRAM_ATTR mpatrol::port1_r(void)
{
  if (m_port2 & 0x08)
    return ay_data_r(0);
  if (m_port2 & 0x10)
    return ay_data_r(1);
  return 0xff;
}

// irem_audio_device::m6803_port2_w()
void IRAM_ATTR mpatrol::port2_w(unsigned char data)
{
  // write latch
  if ((m_port2 & 0x01) && !(data & 0x01))
  {
    if (m_port2 & 0x04)                      // control
    {
      if (m_port2 & 0x08) ay_address_w(0, m_port1);
      if (m_port2 & 0x10) ay_address_w(1, m_port1);
    }
    else                                     // data
    {
      if (m_port2 & 0x08) ay_data_w(0, m_port1);
      if (m_port2 & 0x10) ay_data_w(1, m_port1);
    }
  }
  m_port2 = data;
}

// ---- AY-3-8910 (ay8910.cpp) -----------------------------------------------------
// soundregs[16 * n + r] mirrors ay[n].regs for audio.cpp.

// ay8910_device::ay8910_reset_ym()
void mpatrol::ay_reset(int n)
{
  ay[n].active = false;
  ay[n].latch = 0;
  ay[n].last_enable = 0xc0;                  // force a write
  for (int i = 0; i < 16; i++)
    ay[n].regs[i] = 0;
  for (int i = 0; i < 14; i++)               // AY_PORTA = 14
    ay_write_reg(n, i, 0);
}

// ay8910_write_ym(0, data)
void IRAM_ATTR mpatrol::ay_address_w(int n, unsigned char data)
{
  ay[n].active = (data >> 4) == 0;           // mask programmed 4-bit code
  if (ay[n].active)
    ay[n].latch = data & 0x0f;
}

// ay8910_write_ym(1, data)
void IRAM_ATTR mpatrol::ay_data_w(int n, unsigned char data)
{
  if (ay[n].active)
    ay_write_reg(n, ay[n].latch, data);
}

// ay8910_device::ay8910_write_reg()
void IRAM_ATTR mpatrol::ay_write_reg(int n, int r, unsigned char v)
{
  ay_S &a = ay[n];
  // ay8910_write_ym(): update the output buffer before changing a register
  if (r == 13 || a.regs[r] != v)
    snd_render_to(exec_cpu ? cur_time() : base);
  ay_flush(n);                                  // banked quiet steps use the old registers
  a.regs[r] = v;
  soundregs[16 * n + r] = v;
  ay_synth_reg(n, r);
#if MPATROL_DBG_HARNESS
  if (dbg_ay_hook && exec_cpu)
    dbg_ay_hook(n, r, v, dbg_cycles(cur_time()));
#endif
  switch (r)
  {
  case 7:                                    // AY_ENABLE
  {
    unsigned char enable = a.regs[7] & 0x40;
    if (enable != (a.last_enable & 0x40))
    {
      // port A write callback: 45L ay8910_45L_porta_w drives the netlist
      // drum inputs, which m52_soundc has none of - nothing to do
    }
    enable = a.regs[7] & 0x80;
    if (enable != (a.last_enable & 0x80))
    {
      // output is high-impedance if port is set to input
      if (n == 0)
        ay45m_portb_w(enable ? a.regs[15] : 0xff);
    }
    a.last_enable = a.regs[7];
    break;
  }
  case 13:                                   // AY_EASHAPE: set_shape() restarts the envelope
    ay_r13_writes[n]++;
    break;
  case 15:                                   // AY_PORTB
    if ((a.regs[7] & 0x80) && n == 0)
      ay45m_portb_w(a.regs[15]);
    break;
  default:
    break;
  }
}

// ay8910_device::ay8910_read_ym() for an AY-3-8910
unsigned char IRAM_ATTR mpatrol::ay_data_r(int n)
{
  static const unsigned char mask[16] = {
    0xff, 0x0f, 0xff, 0x0f, 0xff, 0x0f, 0x1f, 0xff, 0x1f, 0x1f, 0x1f, 0xff, 0xff, 0x0f, 0xff, 0xff
  };
  ay_S &a = ay[n];
  if (!a.active)
    return 0xff;                             // high impedance
  int r = a.latch;
  if (r == 14 && n == 0)
    a.regs[14] = m_soundlatch;               // port_a_read_callback = soundlatch_r
  return a.regs[r] & mask[r];
}

// irem_audio_device::ay8910_45M_portb_w()
void IRAM_ATTR mpatrol::ay45m_portb_w(unsigned char data)
{
  // bits 2-4 select MSM5205 clock & 3b/4b playback mode
  msm_playmode_w((data >> 2) & 7);
  // bits 0 and 1 reset the two chips (only msm1 on this board)
  msm_reset = (data & 1) != 0;
#if MPATROL_DBG_HARNESS
  if (dbg_msm_hook && exec_cpu)
    dbg_msm_hook('B', data, dbg_cycles(cur_time()));
#endif
}

// ---- MSM5205 (msm5205.cpp) --------------------------------------------------------

// msm5205_device::playmode_w()
void IRAM_ATTR mpatrol::msm_playmode_w(unsigned char data)
{
  bool s1 = (data & 1) != 0;
  bool s2 = (data & 2) != 0;
  unsigned char bitwidth = (data & 4) ? 4 : 3;
  if (msm_s1 != s1 || msm_s2 != s2)
  {
    msm_s1 = s1;
    msm_s2 = s2;
    // notify_clock_changed() -> device_clock_changed(), at the current time
    msm_clock_changed(exec_cpu ? cur_time() : base);
  }
  msm_bitwidth = bitwidth;
}

// msm5205_device::device_clock_changed(): VCK toggles every prescaler/2
// clocks from now, or stops in slave mode (prescaler 0)
void IRAM_ATTR mpatrol::msm_clock_changed(uint64_t now)
{
  int prescaler = msm_s1 ? (msm_s2 ? 0 : 48) : (msm_s2 ? 64 : 96);   // get_prescaler()
  if (prescaler != 0)
  {
    uint64_t first = first_timer();
    msm_vck_period = (uint64_t)(prescaler / 2) * MP_MSM_CLK;   // clocks_to_attotime(prescaler / 2)
    msm_vck_next = now + msm_vck_period;
    // emu_timer::adjust(): a timer that becomes the first one aborts the
    // running CPU's timeslice
    if (exec_cpu == 2 && msm_vck_next < first)
      m6801_abort_timeslice(&snd);
  }
  else
    msm_vck_period = 0;
}

// msm5205_device::toggle_vck()
void IRAM_ATTR mpatrol::msm_toggle_vck(uint64_t now)
{
  msm_vck = !msm_vck;
  // vck_callback -> set_inputline(m_cpu, INPUT_LINE_NMI): synced, applied
  // before any CPU runs again
  m6801_set_input(&snd, M6801_NMI_LINE, msm_vck ? 1 : 0);
  if (!msm_vck)
    msm_capture_next = now + 6 * MP_MSM_CLK;   // m_capture_timer->adjust(from_ticks(adpcm_capture_divisor(), clock()))
}

// msm5205_device::update_adpcm() - at the capture time after the VCK falling edge
void IRAM_ATTR mpatrol::msm_update_adpcm(void)
{
  static const int index_shift[8] = { -1, -1, -1, -1, 2, 4, 6, 8 };
  // compute_tables(): stepval = floor(16 * 1.1^step), nibble bits
  static int diff_lookup[49 * 16];
  static bool tables = false;
  if (!tables)
  {
    static const int nbl2bit[16][4] = {
      { 1, 0, 0, 0}, { 1, 0, 0, 1}, { 1, 0, 1, 0}, { 1, 0, 1, 1},
      { 1, 1, 0, 0}, { 1, 1, 0, 1}, { 1, 1, 1, 0}, { 1, 1, 1, 1},
      {-1, 0, 0, 0}, {-1, 0, 0, 1}, {-1, 0, 1, 0}, {-1, 0, 1, 1},
      {-1, 1, 0, 0}, {-1, 1, 0, 1}, {-1, 1, 1, 0}, {-1, 1, 1, 1}
    };
    for (int step = 0; step <= 48; step++)
    {
      int stepval = (int)floor(16.0 * pow(11.0 / 10.0, (double)step));
      for (int nib = 0; nib < 16; nib++)
        diff_lookup[step * 16 + nib] = nbl2bit[nib][0] *
          (stepval * nbl2bit[nib][1] + stepval / 2 * nbl2bit[nib][2] +
           stepval / 4 * nbl2bit[nib][3] + stepval / 8);
    }
    tables = true;
  }

  int new_signal;
  if (msm_reset)
  {
    new_signal = 0;
    msm_step = 0;
  }
  else
  {
    int val = msm_data;
    new_signal = msm_signal + diff_lookup[msm_step * 16 + (val & 15)];
    if (new_signal > 2047)
      new_signal = 2047;
    else if (new_signal < -2048)
      new_signal = -2048;
    msm_step += index_shift[val & 7];
    if (msm_step > 48)
      msm_step = 48;
    else if (msm_step < 0)
      msm_step = 0;
  }
  if (msm_signal != new_signal)
  {
    snd_render_to(msm_capture_next);           // m_stream->update() before the change
    msm_signal = new_signal;
    msm_f = (float)(msm_signal & ~3) * (1.0f / 4096.0f);   // 10-bit DAC
  }
#if MPATROL_DBG_HARNESS
  if (dbg_msm_hook)
    dbg_msm_hook('S', msm_signal, dbg_cycles(msm_capture_next));
#endif
}

// ---- Sound output --------------------------------------------------------------
// The whole mix is rendered here, in emulated time, like MAME's streams: the
// output is brought up to "now" before an AY register or the MSM5205 output
// changes (ay8910_write_ym: m_channel->update(); update_adpcm:
// m_stream->update()) and at the end of each frame. audio.cpp pops one
// 24kHz sample per renderFmSample() call.
//
// AY-3-8910 (ay8910.cpp sound_stream_update): stream at clock / 8 = 111860Hz
// (894886 / 8), tone count +1 per stream sample, output flips when it
// reaches the period; noise: prescaler flips every noise period, the 17-bit
// LFSR ticks every second flip; envelope: one step per period * 2 (m_step),
// 16 steps. AY8910_SINGLE_OUTPUT: the three channels drive one resistor
// network (build_3D_table: ay8910_param, 470 ohm load, zero_is_off) -
// computed here directly instead of MAME's 256K-entry table. Each 24kHz
// output sample is the average of the stream samples inside it.
//
// m52_sound_c_discrete (irem.cpp) at 24kHz:
//   NODE_10 = (AY 45M + AY 45L) / 2
//   NODE_20 = MIXER3(MSM, 5V, 0; R19 10k with C37 0.1uF, R22 47k, R23 2.2k)
//             (the 5V term is a constant removed again by the output
//             coupling capacitor, so it is left out)
//   NODE_25 = RC low pass R12 10k, C30 22nF
//   NODE_40 = MIXER2(NODE_10 R25 10k, NODE_25 R15 2.2k; rF VR1 50k,
//             cAmp 1uF = high pass with 100k), the output (DISCRETE_OUTPUT)
// MSM5205 stream value: signal & ~3 (10-bit DAC) / 4096.

static const double ay_r_up = 800000.0, ay_r_down = 8000000.0, ay_rl = 470.0;
static const double ay_res[16] = { 15950, 15350, 15090, 14760, 14275, 13620, 12890, 11370,
                                   10600,  8590,  7190,  5985,  4820,  3945,  3017,  2345 };

void mpatrol::snd_out_reset(void)
{
  for (int n = 0; n < 2; n++)
  {
    ay_synth_S &s = ays[n];
    memset(&s, 0, sizeof(s));
    s.rng = 1;                                  // ay8910_reset_ym(): m_rng = 1
    s.mix_key = 0xffffffff;
    s.dirty = 1;
    for (int c = 0; c < 3; c++)
      s.tone_period_eff[c] = 1;
  }
  for (int i = 0; i < MIX_CACHE; i++)
    mix_cache[i].key = 0xffffffff;
  snd_stream_t = 0;
  snd_stream_frac = 0;
  snd_acc = 0;
  snd_sum0 = snd_sum1 = snd_msum = 0;
  snd_steps = 0;
  hp37 = lp30 = hpamp = 0.0f;
  snd_wr = snd_rd = 0;
  snd_last = 0;
  // RC_CHARGE_EXP(rc) = 1 - exp(-dt / rc), dt = 1 / 24000
  const double dt = 1.0 / MP_OUT_RATE;
  e37 = (float)(1.0 - exp(-dt / (10000.0 * 0.1e-6)));
  e30 = (float)(1.0 - exp(-dt / (10000.0 * 0.022e-6)));
  eamp = (float)(1.0 - exp(-dt / (100000.0 * 1e-6)));
  for (int j = 0; j < 16; j++)
    inv_res[j] = (float)(1.0 / ay_res[j]);
}

// ay8910_device::ay8910_write_reg() effects on the synthesis state
void IRAM_ATTR mpatrol::ay_synth_reg(int n, int r)
{
  ay_synth_S &s = ays[n];
  const unsigned char *regs = ay[n].regs;
  s.dirty = 1;                                  // volume / enable / envelope may have changed
  switch (r)
  {
  case 0: case 1: s.tone_period[0] = regs[0] | ((regs[1] & 0x0f) << 8); s.tone_period_eff[0] = s.tone_period[0] > 0 ? s.tone_period[0] : 1; break;
  case 2: case 3: s.tone_period[1] = regs[2] | ((regs[3] & 0x0f) << 8); s.tone_period_eff[1] = s.tone_period[1] > 0 ? s.tone_period[1] : 1; break;
  case 4: case 5: s.tone_period[2] = regs[4] | ((regs[5] & 0x0f) << 8); s.tone_period_eff[2] = s.tone_period[2] > 0 ? s.tone_period[2] : 1; break;
  case 8: case 9: case 10: s.tone_volume[r - 8] = regs[r]; break;
  case 11: case 12: s.env_period = regs[11] | (regs[12] << 8); break;
  case 13:
  {
    // envelope_t::set_shape(shape, 0x0f)
    unsigned char shape = regs[13];
    s.env_attack = (shape & 0x04) ? 0x0f : 0x00;
    if ((shape & 0x08) == 0)
    {
      s.env_hold = 1;
      s.env_alternate = s.env_attack;
    }
    else
    {
      s.env_hold = shape & 0x01;
      s.env_alternate = shape & 0x02;
    }
    s.env_step = 0x0f;
    s.env_holding = 0;
    s.env_volume = s.env_step ^ s.env_attack;
    break;
  }
  default:
    break;
  }
}

// One stream sample of one AY (sound_stream_update body), returns the
// SINGLE_OUTPUT value (mix_3D). The mix is only rebuilt when a tone output,
// the noise bit or the envelope level changed, or a register was written
// (dirty) - the value is the same function of the same state either way.
float IRAM_ATTR mpatrol::ay_stream_sample(int n)
{
  ay_synth_S &s = ays[n];
  unsigned changed = s.dirty;
  for (int chan = 0; chan < 3; chan++)
  {
    if (++s.tone_count[chan] >= s.tone_period_eff[chan])
    {
      int period = s.tone_period_eff[chan];
      do
      {
        s.tone_duty[chan] = (s.tone_duty[chan] - 1) & 0x1f;
        s.tone_count[chan] -= period;
      } while (s.tone_count[chan] >= period);
      unsigned char out = s.tone_duty[chan] & 1;
      changed |= out ^ s.tone_output[chan];
      s.tone_output[chan] = out;
    }
  }
  if (++s.count_noise >= (ay[n].regs[6] & 0x1f))
  {
    s.count_noise = 0;
    s.prescale_noise ^= 1;
    if (!s.prescale_noise)                         // noise_rng_tick()
    {
      uint32_t old = s.rng;
      s.rng = (s.rng >> 1) | (((s.rng & 1) ^ ((s.rng >> 3) & 1)) << 16);
      changed |= (old ^ s.rng) & 1;
    }
  }
  // envelope (m_step = 2, mask 0x0f)
  if (!s.env_holding)
  {
    if (++s.env_count >= s.env_period * 2)
    {
      s.env_count = 0;
      s.env_step--;
      if (s.env_step < 0)
      {
        if (s.env_hold)
        {
          if (s.env_alternate)
            s.env_attack ^= 0x0f;
          s.env_holding = 1;
          s.env_step = 0;
        }
        else
        {
          if (s.env_alternate && (s.env_step & 0x10))
            s.env_attack ^= 0x0f;
          s.env_step &= 0x0f;
        }
      }
      s.env_volume = s.env_step ^ s.env_attack;
      changed = 1;
    }
  }
  if (!changed)
    return s.mix_val;
  s.dirty = 0;

  return ay_mix(n);
}

// mix_3D() of the current state of one AY (cached in mix_val / mix_key)
float IRAM_ATTR mpatrol::ay_mix(int n)
{
  ay_synth_S &s = ays[n];
  // mix_3D(): per channel the volume index (0 if the output is off) and
  // whether it is an envelope channel
  const unsigned char enable = ay[n].regs[7];
  uint32_t key = 0;
  const unsigned noise_out = s.rng & 1;
  for (int chan = 0; chan < 3; chan++)
  {
    bool vol_enabled = ((s.tone_output[chan] | ((enable >> chan) & 1)) & (noise_out | ((enable >> (3 + chan)) & 1))) != 0;
    bool env = (s.tone_volume[chan] >> 4) & 1;
    unsigned vol = vol_enabled ? (env ? s.env_volume : (s.tone_volume[chan] & 0x0f)) : 0;
    key |= (vol | (env ? 0x10 : 0)) << (chan * 5);
  }
  if (key != s.mix_key)
  {
    // mix_cache: key -> value, so the float division (a soft-float routine
    // in flash on the ESP32) only runs the first time a key is seen
    mix_cache_S &e = mix_cache[(key ^ (key >> 7)) & (MIX_CACHE - 1)];
    if (e.key != key)
    {
#if MPATROL_PROFILE && (!MPATROL_DBG_HARNESS || MPATROL_HOST_PROF)
      pf_key++;
#endif
      // build_3D_table() entry for this key: n channels "on" (zero_is_off: a
      // non-envelope channel at level 0 is off), rw / rt of the network
      float nn = 0;
      float sum = 0;
      for (int chan = 0; chan < 3; chan++)
      {
        unsigned k = (key >> (chan * 5)) & 0x1f;
        if ((k & 0x0f) != 0 || (k & 0x10))
          nn += 1;
        sum += inv_res[k & 0x0f];
      }
      float rw = nn * (float)(1.0 / ay_r_up) + sum;
      float rt = rw + (float)(3.0 / ay_r_down + 1.0 / ay_rl);
      e.key = key;
      e.val = rw / rt;
    }
    s.mix_val = e.val;
    s.mix_key = key;
  }
  return s.mix_val;
}

// k stream samples of one AY, returns the sum of their values. Exactly the
// same as k calls of ay_stream_sample(): between two steps where a counter
// reaches its threshold (tone, noise prescaler, envelope) nothing changes,
// so those steps only advance the counters and repeat the current mix.
// MPATROL_SND_EXACT_STEP 1 uses the plain loop (reference for the harness).
// Steps until the first step (included) where a counter that can change
// the output fires - see ay_stream_samples.
int IRAM_ATTR mpatrol::ay_event_distance(int n)
{
  ay_synth_S &s = ays[n];
  const int np = ay[n].regs[6] & 0x1f;
  // Which counters can change the output (the mix_3D index):
  //  - a tone: its tone is enabled in R7 and the channel is audible
  //    (volume != 0 or envelope mode) - otherwise its output is masked
  //  - the noise: enabled in R7 on an audible channel; and then only an
  //    LFSR tick that changes bit 0
  //  - the envelope: a channel is in envelope mode
  // The others (and every counter during the quiet steps) are advanced in
  // bulk, to the same state the per-step code would leave.
  const unsigned char r7 = ay[n].regs[7];
  bool tone_rel[3];
  bool noise_used = false, env_used = false;
  for (int chan = 0; chan < 3; chan++)
  {
    bool env = (s.tone_volume[chan] >> 4) & 1;
    bool audible = env || (s.tone_volume[chan] & 0x0f) != 0;
    tone_rel[chan] = audible && !((r7 >> chan) & 1);
    noise_used |= audible && !((r7 >> (3 + chan)) & 1);
    env_used |= env;
  }
  // steps until the first relevant event (that step included)
  int d = 1 << 30;
  for (int chan = 0; chan < 3; chan++)
    if (tone_rel[chan])
    {
      int t = s.tone_period_eff[chan] - s.tone_count[chan];
      if (t < d) d = t;
    }
  if (noise_used)
  {
    // steps until the noise OUTPUT (rng bit 0) changes. Flips of the
    // prescaler come after max(1, np - count) steps, then every max(1, np);
    // the LFSR ticks on the flip that clears the prescaler, and the output
    // after m ticks is bit m of rng (m <= 16), so find the first m where it
    // differs from bit 0.
    int per = np > 0 ? np : 1;
    int t = np - s.count_noise;
    if (t < 1) t = 1;
    if (!s.prescale_noise)
      t += per;                                   // this flip sets it, the next one ticks
    int m = 1;
    while (m < 16 && (((s.rng >> m) ^ s.rng) & 1) == 0)
      m++;
    t += (m - 1) * 2 * per;
    if (t < d) d = t;
  }
  if (env_used && !s.env_holding)
  {
    int64_t t = (int64_t)s.env_period * 2 - (int64_t)s.env_count;
    if (t < d) d = (int)t;
  }
  if (d < 1)
    d = 1;
  return d;
}

// Advance every counter of one AY by q steps in which no relevant event
// happens - the state the per-step code would leave.
void IRAM_ATTR mpatrol::ay_bulk_advance(int n, int q)
{
  ay_synth_S &s = ays[n];
  const int np = ay[n].regs[6] & 0x1f;
  // tones: count += q, the output flips once per period reached
  for (int chan = 0; chan < 3; chan++)
  {
    int total = s.tone_count[chan] + q;
    int period = s.tone_period_eff[chan];
    if (total >= period)
    {
      int flips = total / period;
      s.tone_count[chan] = total % period;
      s.tone_duty[chan] = (s.tone_duty[chan] - flips) & 0x1f;
      s.tone_output[chan] = s.tone_duty[chan] & 1;
    }
    else
      s.tone_count[chan] = total;
  }
  {
    // the same steps of the noise prescaler / LFSR as ay_stream_sample(),
    // counted: the first flip after max(1, np - count) steps, then one
    // every max(1, np); the LFSR ticks on each flip that clears the
    // prescaler
    int t = np - s.count_noise;
    if (t < 1) t = 1;
    if (t > q)
      s.count_noise += q;
    else
    {
      int per = np > 0 ? np : 1;
      int r = q - t;
      int flips = 1 + r / per;
      s.count_noise = r % per;
      int ticks = s.prescale_noise ? (flips + 1) >> 1 : flips >> 1;
      s.prescale_noise ^= flips & 1;
      while (ticks-- > 0)
        s.rng = (s.rng >> 1) | (((s.rng & 1) ^ ((s.rng >> 3) & 1)) << 16);
    }
  }
  // envelope: its steps one by one (it holds after 16 for most shapes)
  {
    int r = q;
    while (r > 0 && !s.env_holding)
    {
      int64_t t = (int64_t)s.env_period * 2 - (int64_t)s.env_count;
      if (t < 1) t = 1;
      if (t > r)
      {
        s.env_count += r;
        break;
      }
      r -= (int)t;
      s.env_count = 0;
      s.env_step--;
      if (s.env_step < 0)
      {
        if (s.env_hold)
        {
          if (s.env_alternate)
            s.env_attack ^= 0x0f;
          s.env_holding = 1;
          s.env_step = 0;
        }
        else
        {
          if (s.env_alternate && (s.env_step & 0x10))
            s.env_attack ^= 0x0f;
          s.env_step &= 0x0f;
        }
      }
      s.env_volume = s.env_step ^ s.env_attack;
    }
  }
}

// Apply the quiet steps banked by ay_stream_samples (before the state is
// used or a register changes)
void IRAM_ATTR mpatrol::ay_flush(int n)
{
  ay_synth_S &s = ays[n];
  if (s.pending)
  {
    ay_bulk_advance(n, s.pending);
    s.pending = 0;
  }
}

#if MPATROL_SND_CHECK
// harness only: every chunk both ways from the same state, first mismatch printed
float mpatrol::ay_stream_samples(int n, int k)
{
  static bool reported = false;
  ay_flush(n);
  ays[n].quiet_left = 0;
  ay_synth_S save = ays[n];
  float fast = ay_stream_samples_fast(n, k);
  ay_synth_S after = ays[n];
  ays[n] = save;
  float exact = 0;
  for (int i = 0; i < k; i++)
    exact += ay_stream_sample(n);
  ay_synth_S ref = ays[n];
  bool same = (fast - exact < 1e-5f && exact - fast < 1e-5f) && ref.rng == after.rng && ref.count_noise == after.count_noise &&
              ref.prescale_noise == after.prescale_noise && ref.env_count == after.env_count &&
              ref.env_step == after.env_step && ref.env_volume == after.env_volume && ref.env_holding == after.env_holding;
  for (int c = 0; c < 3; c++)
    same = same && ref.tone_count[c] == after.tone_count[c] && ref.tone_duty[c] == after.tone_duty[c] && ref.tone_output[c] == after.tone_output[c];
  if (!same && !reported)
  {
    reported = true;
    printf("SND_CHECK mismatch chip %d k %d: fast %f exact %f | R6 %02x R7 %02x vol %02x %02x %02x per %d %d %d envper %u shape %02x\n",
           n, k, fast, exact, ay[n].regs[6], ay[n].regs[7], save.tone_volume[0], save.tone_volume[1], save.tone_volume[2],
           save.tone_period[0], save.tone_period[1], save.tone_period[2], save.env_period, ay[n].regs[13]);
    printf("  before: cnt %d %d %d duty %d %d %d out %d %d %d | noise cnt %d pre %d rng %05x | env cnt %u step %d hold %d holding %d dirty %d mixkey %x\n",
           save.tone_count[0], save.tone_count[1], save.tone_count[2], save.tone_duty[0], save.tone_duty[1], save.tone_duty[2],
           save.tone_output[0], save.tone_output[1], save.tone_output[2], save.count_noise, save.prescale_noise, save.rng,
           save.env_count, save.env_step, save.env_hold, save.env_holding, save.dirty, save.mix_key);
    printf("  fast:   cnt %d %d %d duty %d %d %d | noise cnt %d pre %d rng %05x | env cnt %u step %d\n",
           after.tone_count[0], after.tone_count[1], after.tone_count[2], after.tone_duty[0], after.tone_duty[1], after.tone_duty[2],
           after.count_noise, after.prescale_noise, after.rng, after.env_count, after.env_step);
    printf("  exact:  cnt %d %d %d duty %d %d %d | noise cnt %d pre %d rng %05x | env cnt %u step %d\n",
           ref.tone_count[0], ref.tone_count[1], ref.tone_count[2], ref.tone_duty[0], ref.tone_duty[1], ref.tone_duty[2],
           ref.count_noise, ref.prescale_noise, ref.rng, ref.env_count, ref.env_step);
  }
  return exact;
}
float IRAM_ATTR mpatrol::ay_stream_samples_fast(int n, int k)
#else
float IRAM_ATTR mpatrol::ay_stream_samples(int n, int k)
#endif
{
  ay_synth_S &s = ays[n];
  float sum = 0;
#if MPATROL_SND_EXACT_STEP
  while (k-- > 0)
    sum += ay_stream_sample(n);
  return sum;
#else
  // quiet: bank the steps, the counters are advanced when needed (ay_flush)
  if (!s.dirty && s.quiet_left >= k)
  {
    s.pending += k;
    s.quiet_left -= k;
    return s.mix_val * k;
  }
  ay_flush(n);
#if MPATROL_SND_FAST
  // MPATROL_SND_FAST: the chip state still advances exactly (bulk), but a
  // group with an event inside is output as the old value up to the event
  // and the new value after it, instead of stepping every stream sample
  // through the event (see mpatrol.h).
  if (s.dirty)
  {
    s.dirty = 0;
    ay_mix(n);
  }
  {
    int d = ay_event_distance(n);
    if (d > k)
    {
      ay_bulk_advance(n, k);
      s.quiet_left = d - 1 - k;
      return s.mix_val * k;
    }
#if MPATROL_PROFILE && (!MPATROL_DBG_HARNESS || MPATROL_HOST_PROF)
    pf_ev++;
#endif
    sum = s.mix_val * (d - 1);
    ay_bulk_advance(n, k);
    sum += ay_mix(n) * (k - d + 1);
    s.quiet_left = ay_event_distance(n) - 1;
    return sum;
  }
#endif
  if (s.dirty)
  {
    // the step that would rebuild the mix: let the normal path do it
    sum += ay_stream_sample(n);
    k--;
  }
  while (k > 0)
  {
    int d = ay_event_distance(n);
    // q quiet steps, then (if the event lies within k) the event step
    bool event = d <= k;
    int q = event ? d - 1 : k;
    if (q > 0)
    {
      ay_bulk_advance(n, q);
      sum += s.mix_val * q;
    }
    if (event)
    {
#if MPATROL_PROFILE && (!MPATROL_DBG_HARNESS || MPATROL_HOST_PROF)
      pf_ev++;
#endif
      sum += ay_stream_sample(n);
      k -= d;
    }
    else
      k = 0;
  }
  s.quiet_left = s.dirty ? 0 : ay_event_distance(n) - 1;
  return sum;
#endif
}

// Render the AY stream up to time t: every stream sample k (time
// k * 1e18 / 111860) before t, like sound_stream::update(). Every 111860/24000
// stream samples the average of the ones since the last output sample goes
// through the discrete network into the ring. The stream samples of one
// output sample are stepped as a group (ay_stream_samples) when they all
// lie before t - the same result as one by one.
void IRAM_ATTR mpatrol::snd_render_to(uint64_t t)
{
#if MPATROL_PROFILE && (!MPATROL_DBG_HARNESS || MPATROL_HOST_PROF)
  pf_calls++;
#endif
#if MPATROL_HOST_PROF
  hp_calls++;
#endif
#if MPATROL_PROFILE && (!MPATROL_DBG_HARNESS || MPATROL_HOST_PROF)
  uint32_t pc0 = mp_ccount();
#endif
  while (snd_stream_t < t)
  {
    // stream samples left in the current output sample, and how many of
    // them lie before t
    int left = (int)((111860 - snd_acc + MP_OUT_RATE - 1) / MP_OUT_RATE);
    int n = 1;
    // time of the group's last stream sample; if before t, take it whole
    uint64_t last = snd_stream_t + (uint64_t)(left - 1) * 8939746111210ULL +
                    (snd_stream_frac + (uint32_t)(left - 1) * 49400) / 111860;
    if (last < t)
      n = left;
    else
    {
      uint64_t tt = snd_stream_t;
      uint32_t fr = snd_stream_frac;
      while (n < left)
      {
        tt += 8939746111210ULL;
        fr += 49400;
        if (fr >= 111860) { fr -= 111860; tt++; }
        if (tt >= t)
          break;
        n++;
      }
    }
#if MPATROL_PROFILE && (!MPATROL_DBG_HARNESS || MPATROL_HOST_PROF)
    uint32_t pa0 = mp_ccount();
#endif
    snd_sum0 += ay_stream_samples(0, n);
    snd_sum1 += ay_stream_samples(1, n);
#if MPATROL_PROFILE && (!MPATROL_DBG_HARNESS || MPATROL_HOST_PROF)
    pf_ay += mp_ccount() - pa0;
#endif
    snd_msum += msm_f * n;
    snd_steps += n;
    for (int i = 0; i < n; i++)
    {
      // next stream sample time: + 1e18 / 111860 = 8939746111210 rem 49400
      snd_stream_t += 8939746111210ULL;
      snd_stream_frac += 49400;
      if (snd_stream_frac >= 111860)
      {
        snd_stream_frac -= 111860;
        snd_stream_t++;
      }
    }
    snd_acc += MP_OUT_RATE * n;
    if (snd_acc < 111860)
      continue;
    snd_acc -= 111860;
#if MPATROL_PROFILE && (!MPATROL_DBG_HARNESS || MPATROL_HOST_PROF)
    pf_out++;
#endif
#if MPATROL_HOST_PROF
    hp_out++;
#endif
    static const float DRAM_ATTR recip[6] = { 0.0f, 1.0f, 1.0f / 2, 1.0f / 3, 1.0f / 4, 1.0f / 5 };
    float inv = recip[snd_steps];   // 4 or 5 stream samples per output sample
    float n10 = (snd_sum0 + snd_sum1) * 0.5f * inv;
    float msm = snd_msum * inv;
    snd_sum0 = snd_sum1 = snd_msum = 0;
    snd_steps = 0;
    // NODE_20: C37 high pass on the MSM input, then the resistor mixer
    hp37 += (msm - hp37) * e37;
    // (constant divisors as multipliers: the ESP32 has no fast float divide)
    static const float k20 = (float)((1.0 / 10000.0) / (1.0 / 10000.0 + 1.0 / 47000.0 + 1.0 / 2200.0));
    float n20 = (msm - hp37) * k20;
    // NODE_25: RC low pass
    lp30 += (n20 - lp30) * e30;
    // NODE_40: resistor mixer with rF, then the cAmp high pass
    static const float k40 = (float)(1.0 / (1.0 / 10000.0 + 1.0 / 2200.0 + 1.0 / 50000.0));
    float n40 = (n10 * (1.0f / 10000.0f) + lp30 * (1.0f / 2200.0f)) * k40;
    hpamp += (n40 - hpamp) * eamp;
    float v = (n40 - hpamp) * MPATROL_SND_GAIN;
    if (v > 511.0f) v = 511.0f;
    if (v < -511.0f) v = -511.0f;

    unsigned short w = snd_wr;
    unsigned short nw = (unsigned short)((w + 1) & (SND_RING - 1));
    if (nw != snd_rd)                          // full: this sample is dropped
    {
      snd_ring[w] = (short)v;
      snd_wr = nw;
    }
  }
#if MPATROL_PROFILE && (!MPATROL_DBG_HARNESS || MPATROL_HOST_PROF)
  prof_render += mp_ccount() - pc0;
#endif
}

// Called by the audio renderer once per output sample (audio.cpp).
int mpatrol::renderFmSample()
{
  unsigned short r = snd_rd;
  if (r != snd_wr)
  {
    snd_last = snd_ring[r];
    snd_rd = (unsigned short)((r + 1) & (SND_RING - 1));
  }
  return snd_last;
}

// ============================================================================
// Scheduling - a model of MAME's device_scheduler (see arkanoid.cpp, which
// was verified against MAME cycle for cycle, for the details)
// ============================================================================
//
// Times are MAME attoseconds with MAME's truncated per-cycle periods:
//   Z80:  HZ_TO_ATTOSECONDS(3072000)              = 325520833333
//   6803: HZ_TO_ATTOSECONDS((3579545 + 3) / 4)    = 1117459522822
//         (m6801 execute_clocks_to_cycles rounds the E clock up: 894887Hz)
// cycles for a time delta: divu_64x32(delta >> divshift, divisor).
//
// device_scheduler::timeslice(): target = min(base + quantum, first timer);
// the Z80, then the 6803, run up to it (overshooting by the rest of the last
// instruction); a CPU that stops early (timeslice aborted) pulls the target
// back. Then the due timers fire. Quantum: no set_maximum_quantum in m52, so
// the 60Hz default.
//
// Timers: screen (m52: set_raw(..., 282, 22, 274)) - power on is at the start
// of VBLANK, then scanline 0 (8 lines in), VBLANK end (line 22, 30 lines in)
// and VBLANK begin one frame later (screen update, IRQ); sound_manager's
// 50Hz update; the MSM5205 VCK timer and its capture timer; and the syncs
// set_input_line() queues (the sound IRQ from cmd_w and sound_irq_ack_w).
// The 6803 only looks at its IRQ/NMI inputs at the start of a run and after
// CLI/TAP/RTI/WAI (m6800_cpu_device::execute_run / 6800ops.hxx), so where the
// runs end decides when it takes a command - which is why this is modelled.

static const uint64_t mp_frame_timers[3] = { 8 * MP_SCAN, 30 * MP_SCAN, MP_FRAME };

// Where MAME's Z80 touches d000-d7ff inside an instruction - arkanoid.cpp
// z80_io_access(), same z80.lst machine cycles (an access happens at the
// start of its machine cycle: rop 2+2, arg/rm/wm 3). Only the sound command
// write (cmd_w) is seen by the other CPU; the game writes it with
// LD (d000),A (0x5d4, 0x5d9, 0x3874...). Its reads (LD A,(d000)) pause the
// same way in MAME, so they are included.
//
// mp_z80_info[op] (built by z80_info_init from the same opcode tests):
// MPZ_IO = z80_io_access() looks at the instruction's address, MPZ_PREFIX =
// CB/ED/DD/FD (R counting, block instructions, LD A,R / LD R,A). z80_exec()
// runs the plain path for the rest.
enum { MPZ_IO = 1, MPZ_PREFIX = 2 };
static unsigned char DRAM_ATTR mp_z80_info[256];

static bool z80_io_op(unsigned char op)
{
  return op == 0x32 || op == 0x3a || op == 0x02 || op == 0x12 || op == 0x36 ||
         (op >= 0x40 && op <= 0x7f && op != 0x76 && ((op & 7) == 6 || (op & 0xf8) == 0x70)) ||
         (op >= 0x80 && op <= 0xbf && (op & 7) == 6);
}

static void z80_info_init(void)
{
  for (int op = 0; op < 256; op++)
    mp_z80_info[op] = (z80_io_op((unsigned char)op) ? MPZ_IO : 0) |
                      ((op == 0xdd || op == 0xfd || op == 0xcb || op == 0xed) ? MPZ_PREFIX : 0);
}

static IRAM_ATTR bool z80_io_access(const Z80 &r, uint16_t pc, mpatrol::z80_io_S &io)
{
  if (pc >= 0x4000 - 2)
    return false;
  unsigned char op = mp_rom[pc];
  static const unsigned char rw16[] = { 2, 2, 3, 3, 3 };        // LD A,(nn) / LD (nn),A
  static const unsigned char rw8[] = { 2, 2, 3 };               // LD (BC)/(DE),A, LD (HL),r
  static const unsigned char ldhln[] = { 2, 2, 3, 3 };          // LD (HL),n
  uint16_t addr;
  const unsigned char *st;
  int n;
  if (op == 0x32 || op == 0x3a)
  {
    addr = mp_rom[pc + 1] | (mp_rom[pc + 2] << 8); st = rw16; n = 5; io.offset = 10;
  }
  else if (op == 0x02)
  {
    addr = r.BC.W; st = rw8; n = 3; io.offset = 4;
  }
  else if (op == 0x12)
  {
    addr = r.DE.W; st = rw8; n = 3; io.offset = 4;
  }
  else if ((op >= 0x40 && op <= 0x7f && op != 0x76 && ((op & 7) == 6 || (op & 0xf8) == 0x70)) ||
           (op >= 0x80 && op <= 0xbf && (op & 7) == 6))
  {
    addr = r.HL.W; st = rw8; n = 3; io.offset = 4;
  }
  else if (op == 0x36)
  {
    addr = r.HL.W; st = ldhln; n = 4; io.offset = 7;
  }
  else
    return false;
  if ((addr & 0xf800) != 0xd000)             // d000-d7ff (only cmd_w, d000 mirror 0x07fc, is seen by the 6803)
    return false;
  io.nsteps = (unsigned char)n;
  memcpy(io.steps, st, n);
  return true;
}

uint64_t IRAM_ATTR mpatrol::cur_time(void) const
{
  if (exec_cpu == 1)
    return exec_base + (uint64_t)(int64_t)exec_pos * MP_Z80_APC;
  if (exec_cpu == 2)
    return exec_base + (uint64_t)(int64_t)m6801_cycles_done(&snd, snd_want) * MP_SND_LOCAL;
  return base;
}

// set_input_line(): device_input::set_state_synced() queues the change and
// calls synchronize() - a timer at time t, which aborts the running CPU's
// timeslice
void IRAM_ATTR mpatrol::line_w(unsigned char line, bool state, uint64_t t)
{
  if (sync_n < (int)(sizeof(sync_q) / sizeof(sync_q[0])))
  {
    sync_q[sync_n].time = t;
    sync_q[sync_n].line = line;
    sync_q[sync_n].state = state;
    sync_n++;
  }
  if (exec_cpu == 1)
    abort_run = true;
  else if (exec_cpu == 2)
    m6801_abort_timeslice(&snd);
}

void IRAM_ATTR mpatrol::line_apply(unsigned char line, bool state)
{
  if (line == LINE_SND_IRQ)
    m6801_set_input(&snd, M6801_IRQ_LINE, state ? 1 : 0);
}

// Refresh register as MAME counts it - arkanoid.cpp z80_opcode_fetches()
static IRAM_ATTR unsigned char z80_opcode_fetches(uint16_t pc)
{
  if (pc >= 0x4000 - 4)
    return 1;
  const unsigned char *p = &mp_rom[pc];
  unsigned char n = 0;
  while (n < 4 && (p[n] == 0xdd || p[n] == 0xfd))
    n++;
  if (n > 3)
    return n;
  if (n && p[n] == 0xcb)
    return n + 1;                  // DD/FD CB d op
  if (p[n] == 0xcb || p[n] == 0xed)
    return n + 2;
  return n + 1;
}

// arkanoid.cpp z80_exec(): the Z80 run with MAME's instruction timing,
// refresh register and pauses before a synced I/O access.
int IRAM_ATTR mpatrol::z80_exec(int want)
{
  static const int START = 1 << 20;
  int budget = want;
  abort_run = false;
  exec_cpu = 1;
  exec_base = z80_t;
  current_cpu = 0;
  Z80 *const R = &cpu[0];
  const unsigned char *const rom = mp_rom;
  while (budget > 0 && !abort_run)
  {
    z80_io_S io;
    bool has_io = false;
    const int done = z80_part_done;
    if (done)
    {
      io = z80_part_io;
      has_io = true;
    }
    else if (z80_irq && (R->IFF & IFF_1) && !ei_shadow)
    {
      // irq0_line_hold: RST 38h (IM 1), dropped on acknowledge.
      // z80.lst take_interrupt IM1: 13 cycles
      exec_pos = want - budget;
      exec_pc = R->PC.W;
      exec_io_offset = 0;
      mpz_Int(R, INT_IRQ);
      z80_r++;
      z80_irq = false;
      budget -= 13;
      continue;
    }
    else
    {
      // table scan at 12b0, see z80_scan_skip (after the IRQ check: no IRQ
      // can be taken for the rest of this run)
      const uint16_t pc = R->PC.W;
      if (MPATROL_IDLE_SKIP && pc == 0x12b0 && z80_scan_skip(budget))
        continue;
      // z80_io_access() only for the opcodes it looks at (mp_z80_info)
      if (pc >= 0x4000 - 4 || (mp_z80_info[rom[pc]] & MPZ_IO))
        has_io = z80_io_access(*R, pc, io);
    }

    if (has_io && io.offset - done >= budget)
    {
      // MAME stops at the first machine-cycle boundary that uses up the run,
      // before the I/O access; the rest of the instruction runs later
      int k = 0, sum = 0;
      while (sum <= done)
        sum += io.steps[k++];
      int used = sum - done;
      while (used < budget)
      {
        sum += io.steps[k++];
        used = sum - done;
      }
      z80_part_io = io;
      z80_part_done = (unsigned char)sum;
      budget -= used;
      break;
    }

    const uint16_t pc = R->PC.W;
    exec_pos = want - budget - done;
    exec_pc = pc;
    exec_io_offset = has_io ? io.offset : 0;
    const unsigned char op = pc < 0x4000 - 4 ? rom[pc] : 0x00;
    int c;
    if (!(mp_z80_info[op] & MPZ_PREFIX))
    {
      // one opcode fetch (R + 1), no block instruction, no LD A,R / LD R,A
      z80_r++;
      R->ICount = START;
      mpz_Step(R);
      int left = R->ICount;
      ei_shadow = (left == 1);
      c = (left <= 1) ? 4 : START - left;
    }
    else
    {
      z80_r += z80_opcode_fetches(pc);
      const unsigned char op2 = rom[pc + 1];
      bool ld_a_r = op == 0xed && op2 == 0x5f;
      bool ld_r_a = op == 0xed && op2 == 0x4f;
      if (op == 0xed && (op2 & 0xf4) == 0xb0)
      {
        // block instructions: one iteration per step, as z80.lst
        R->ICount = 1;
        mpz_Step(R);
        c = 1 - R->ICount;
        ei_shadow = false;
      }
      else
      {
        R->ICount = START;
        mpz_Step(R);
        int left = R->ICount;
        ei_shadow = (left == 1);
        c = (left <= 1) ? 4 : START - left;
      }
      if (ld_a_r)
      {
        unsigned char a = (z80_r & 0x7f) | z80_r2;
        R->AF.B.h = a;
        R->AF.B.l = (R->AF.B.l & C_FLAG) | (a & (S_FLAG | 0x28)) | (a ? 0 : Z_FLAG) |
                    ((R->IFF & IFF_2) ? P_FLAG : 0);
      }
      else if (ld_r_a)
      {
        z80_r = R->AF.B.h;
        z80_r2 = R->AF.B.h & 0x80;
      }
    }
#if MPATROL_DBG_HARNESS
    if (dbg_z80_pc_hook)
      dbg_z80_pc_hook(exec_pc, dbg_cycles(cur_time()));
#endif
    z80_part_done = 0;
    budget -= c - done;
#if MPATROL_PROFILE && (!MPATROL_DBG_HARNESS || MPATROL_HOST_PROF)
    prof_z80_insn++;
#endif
  }
  exec_cpu = 0;
  return want - budget;
}

// 42% of the Z80's instructions are this table scan (called from 12a7 with
// HL = e370, DE = 0010, B = 25):
//   12b0 ld a,(hl) / cp $14 / jr z,$12df / cp $1e / jr c,$12d4
//   12d4 add hl,de / djnz $12b0
// For an entry below $1e and not $14 an iteration is those 7 instructions,
// 7+7+7+7+12+11+13 = 64 cycles (djnz taken), 7 opcode fetches (R). Such
// iterations are charged here in bulk. It reads only RAM this CPU writes,
// and no interrupt can be taken within the run. A and F are left stale:
// the iteration after the skip always runs normally inside this run (the
// budget check keeps its ld a,(hl) and cp $14 in it), and those two
// instructions set A and all of F from the entry alone - nothing reads the
// old values. The last iteration (B = 1) always runs normally.
bool IRAM_ATTR mpatrol::z80_scan_skip(int &budget)
{
  Z80 &r = cpu[0];
  if (r.DE.W != 0x0010 || ei_shadow || z80_part_done)
    return false;
  uint16_t hl = r.HL.W;
  unsigned char b = r.BC.B.h;
  int n = 0;
  while (b > 1 && budget > 64 + 7 && (hl & 0xf800) == 0xe000)
  {
    unsigned char v = memory[MPATROL_WORKRAM + (hl & 0x7ff)];
    if (v == 0x14 || v >= 0x1e)
      break;
    hl += 0x10;
    b--;
    budget -= 64;
    n++;
  }
  if (n == 0)
    return false;
  r.HL.W = hl;
  r.BC.B.h = b;
  z80_r += 7 * n;
  return true;
}

int IRAM_ATTR mpatrol::snd_exec(int want)
{
  exec_cpu = 2;
  exec_base = snd_t;
  snd_want = want;
  int ran = m6801_run(&snd, want);
  exec_cpu = 0;
  return ran;
}

uint64_t IRAM_ATTR mpatrol::first_timer(void)
{
  uint64_t t = frame_start + mp_frame_timers[frame_timer];
  if (snd_update_next < t)
    t = snd_update_next;
  if (msm_vck_period && msm_vck_next < t)
    t = msm_vck_next;
  if (msm_capture_next && msm_capture_next < t)
    t = msm_capture_next;
  for (int i = 0; i < sync_n; i++)
    if (sync_q[i].time < t)
      t = sync_q[i].time;
  return t;
}

void IRAM_ATTR mpatrol::run_frame(void)
{
#if MPATROL_REAL_SPEED && !MPATROL_DBG_HARNESS
  // MPATROL_REAL_SPEED: emulate a frame only when real time has caught up
  // (the frame period is MP_FRAME = 17624.99999993 us). The credit is capped
  // so a stall (menu, flash, serial) does not make the game race afterwards.
  {
    uint32_t now = micros();
    if (!pace_init)
    {
      pace_init = true;
      pace_last = now;
      pace_credit = 17625;
    }
    pace_credit += (int32_t)(now - pace_last);
    pace_last = now;
    if (pace_credit > 3 * 17625)
      pace_credit = 3 * 17625;
    if (pace_credit < 17625)
      return;                                  // the display shows the last frame again
    pace_credit -= 17625;
  }
#endif
#if MPATROL_PROFILE && (!MPATROL_DBG_HARNESS || MPATROL_HOST_PROF)
  uint32_t prof_t0 = micros();
#endif
  // Keep the 64-bit attosecond clock small (MAME's attotime seconds field)
  if (base >= MP_ATTOS)
  {
    base -= MP_ATTOS;
    z80_t -= MP_ATTOS;
    snd_t -= MP_ATTOS;
    frame_start -= MP_ATTOS;
    snd_update_next -= MP_ATTOS;
    if (msm_vck_period)
      msm_vck_next -= MP_ATTOS;
    if (msm_capture_next)
      msm_capture_next -= MP_ATTOS;
    snd_stream_t -= MP_ATTOS;                  // 1s = 111860 stream samples exactly
    time_seconds++;
    for (int i = 0; i < sync_n; i++)
      sync_q[i].time -= MP_ATTOS;
  }

  frame_timer = 0;
  for (;;)
  {
    // ---- device_scheduler::timeslice()
    while (base < first_timer())
    {
      uint64_t target = base + MP_QUANTUM;
      uint64_t ft = first_timer();
      if (ft < target)
        target = ft;

      if (target > z80_t && target - z80_t >= MP_Z80_APC)
      {
        int want = (int)(((target - z80_t) >> MP_Z80_SHIFT) / MP_Z80_DIV);
#if MPATROL_PROFILE && (!MPATROL_DBG_HARNESS || MPATROL_HOST_PROF)
        uint32_t pz = mp_ccount();
        z80_t += (uint64_t)z80_exec(want) * MP_Z80_APC;
        prof_z80 += mp_ccount() - pz;
#else
        z80_t += (uint64_t)z80_exec(want) * MP_Z80_APC;
#endif
        if (z80_t < target)
          target = z80_t > base ? z80_t : base;
      }
      if (target > snd_t && target - snd_t >= MP_SND_APC)
      {
        int want = (int)(((target - snd_t) >> MP_SND_SHIFT) / MP_SND_DIV);
#if MPATROL_PROFILE && (!MPATROL_DBG_HARNESS || MPATROL_HOST_PROF)
        uint32_t ps = mp_ccount();
        snd_t += (uint64_t)snd_exec(want) * MP_SND_APC;
        prof_snd += mp_ccount() - ps;
        prof_slices++;
#else
        snd_t += (uint64_t)snd_exec(want) * MP_SND_APC;
#endif
        if (snd_t < target)
          target = snd_t > base ? snd_t : base;
      }
      base = target;
#if MPATROL_DBG_HARNESS
      if (dbg_slice_hook)
        dbg_slice_hook(base, z80_t, snd_t);
#endif
    }

    // ---- execute_timers(): everything due, in time order
    bool frame_done = false;
    for (;;)
    {
      uint64_t ft = frame_start + mp_frame_timers[frame_timer];
      // the earliest due event; ties go to the frame timers first, then
      // the others in this order (timer_list_insert keeps insertion order)
      uint64_t t = ft;
      int kind = 0;                                   // 0 frame
      if (snd_update_next < t) { t = snd_update_next; kind = 1; }
      if (msm_vck_period && msm_vck_next < t) { t = msm_vck_next; kind = 2; }
      if (msm_capture_next && msm_capture_next < t) { t = msm_capture_next; kind = 3; }
      int si = -1;
      for (int i = 0; i < sync_n; i++)
        if (sync_q[i].time < t && (si < 0 || sync_q[i].time < sync_q[si].time))
          si = i;
      if (si >= 0) { t = sync_q[si].time; kind = 4; }
      if (t > base)
        break;
#if MPATROL_DBG_HARNESS
      if (dbg_timer_hook)
        dbg_timer_hook(kind, t);
#endif
      if (kind == 1)
        snd_update_next += MP_SND_UPDATE;             // sound_manager::update(): nothing visible
      else if (kind == 2)
      {
        msm_vck_next += msm_vck_period;               // periodic: schedule_next_period()
        msm_toggle_vck(t);
      }
      else if (kind == 3)
      {
        msm_update_adpcm();
        msm_capture_next = 0;
      }
      else if (kind == 4)
      {
        sync_S e = sync_q[si];
        for (int i = si; i < sync_n - 1; i++)
          sync_q[i] = sync_q[i + 1];
        sync_n--;
        line_apply(e.line, e.state);
      }
      else if (frame_timer == 2)
      {
        vblank_begin();
        frame_start += MP_FRAME;
        frame_done = true;
        break;
      }
      else
        frame_timer++;
    }
    if (frame_done)
      break;
  }

  snd_render_to(base);

#if MPATROL_PROFILE && (!MPATROL_DBG_HARNESS || MPATROL_HOST_PROF)
  static uint32_t prof_sum = 0, prof_max = 0, prof_n = 0;
  uint32_t dt = micros() - prof_t0;
  prof_sum += dt;
  if (dt > prof_max)
    prof_max = dt;
  if (++prof_n == 60)
  {
    // CPU cycle counts at 240MHz -> us. snd includes the sound rendering
    // done at AY writes; render is all of the rendering.
    printf("mpatrol: render per frame: calls %lu, out samples %lu, event steps %lu, mix rebuilds %lu, ay stepping %lu us\n", (unsigned long)(pf_calls / 60), (unsigned long)(pf_out / 60), (unsigned long)(pf_ev / 60), (unsigned long)(pf_key / 60), (unsigned long)(pf_ay / 240 / 60));
    pf_calls = pf_out = pf_ev = pf_key = pf_ay = 0;
    printf("mpatrol: run_frame avg %lu us, max %lu us (z80 %lu us, snd %lu us, render %lu us, %lu slices), budget 17625 us\n",
           (unsigned long)(prof_sum / 60), (unsigned long)prof_max,
           (unsigned long)(prof_z80 / 240 / 60), (unsigned long)(prof_snd / 240 / 60),
           (unsigned long)(prof_render / 240 / 60), (unsigned long)(prof_slices / 60));
    printf("mpatrol: z80 %lu instructions/frame, %lu cpu cycles each\n", (unsigned long)(prof_z80_insn / 60),
           (unsigned long)(prof_z80_insn ? prof_z80 / prof_z80_insn : 0));
    prof_z80_insn = 0;
#if MPATROL_HOST_PROF
    printf("  per frame: render calls %lu, output samples %lu, event steps %lu, quiet steps %lu, ev tone %lu noise %lu env %lu%c", hp_calls / 60, hp_out / 60, hp_steps / 60, hp_quiet / 60, hp_ev_tone / 60, hp_ev_noise / 60, hp_ev_env / 60, 10);
    hp_calls = hp_out = hp_steps = hp_quiet = hp_ev_tone = hp_ev_noise = hp_ev_env = 0;
#endif
    prof_sum = prof_max = prof_n = 0;
    prof_z80 = prof_snd = prof_render = prof_slices = 0;
  }
#endif
}

// screen_device::vblank_begin(): screen update, then screen_vblank(1)
void mpatrol::vblank_begin(void)
{
  publish_snapshot();
  // ioport_manager::frame_update(): inputs are sampled once a frame
  in_keys = input->buttons_get();
  // m52: set_vblank_int("screen", irq0_line_hold)
  z80_irq = true;
}

// ============================================================================
// Video - m52_state::screen_update() (m52.cpp)
// ============================================================================

void mpatrol::publish_snapshot(void)
{
  unsigned char back = snap_front ^ 1;
  snap_S &s = snap[back];
  memcpy(s.vram, &memory[MPATROL_VIDEORAM], sizeof(s.vram));
  memcpy(s.cram, &memory[MPATROL_COLORRAM], sizeof(s.cram));
  memcpy(s.spr, &memory[MPATROL_SPRITERAM], sizeof(s.spr));
  s.bgxpos[0] = m_bgxpos[0]; s.bgxpos[1] = m_bgxpos[1];
  s.bgypos[0] = m_bgypos[0]; s.bgypos[1] = m_bgypos[1];
  s.bgcontrol = m_bgcontrol;
  s.scroll = m_scroll;
  s.scroll_written = m_scroll_written;
  s.flip = m_flip;
  snap_front = back;
}

// Landscape mapping (mpatrol.h): panel line L shows MAME column
// x = 375 - (L - MPATROL_ROW_OFFSET), panel column c shows MAME line
// y = MPATROL_Y0 + c. Everything below is in MAME bitmap coordinates
// (visible x 136-375, y 22-273).
//
// Flip screen is NOT drawn yet: m_flip is kept (flipscreen_w) but the
// picture is always the unflipped one. MAME's flipped layers are not a plain
// mirror of each other (tilemap set_flip + scrolldx flipped vs sprite
// 238 - sx / 282 - sy vs background 264 - xpos), so this needs a MAME run with
// the flip DIP on to get right. Upright cabinets never flip.
void IRAM_ATTR mpatrol::render_row(short row)
{
  if (row == 0)
    render_snap = snap_front;
  const snap_S &s = snap[render_snap];

  int line0 = row * 8 - MPATROL_ROW_OFFSET;
  if (line0 + 7 < 0 || line0 >= MPATROL_VIS_W)
    return;                                   // frame_buffer was cleared by the caller

  // sprites that touch this strip's 8 MAME columns, in drawing order
  // (groups of 16 from 0x3c to 0xfc, each from its highest entry down)
  int xhi = MPATROL_VIS_X0 + MPATROL_VIS_W - 1 - line0;   // x of r = 0
  int xlo = xhi - 7;
  unsigned char spr_list[64];
  int nspr = 0;
  for (int g = 0x3c; g <= 0xfc; g += 0x40)
    for (int offs = g; offs >= (g & 0xc0); offs -= 4)
    {
      int sx = s.spr[offs + 3] + 129;
      if (sx + 15 >= xlo && sx <= xhi)
        spr_list[nspr++] = (unsigned char)offs;
    }

  // background layers of screen_update (draw_background), later on top
  int layers[2], nl = 0;
  if (!(s.bgcontrol & 0x20))
  {
    if (!(s.bgcontrol & 0x10))
      layers[nl++] = 0;                       // distant mountains (bgxpos[1] / bgypos[1])
    if (!(s.bgcontrol & 0x02))
      layers[nl++] = 1;                       // hills
    else if (!(s.bgcontrol & 0x04))
      layers[nl++] = 2;                       // cityscape
  }

  const int value_static = s.scroll_written ? 128 : 127;                  // 127 - 255, 127 - 0
  const int value_row3 = s.scroll_written ? ((128 + s.scroll) & 255) : 127;

  for (int r = 0; r < 8; r++)
  {
    int line = line0 + r;
    if (line < 0 || line >= MPATROL_VIS_W)
      continue;
    int x = MPATROL_VIS_X0 + MPATROL_VIS_W - 1 - line;
    unsigned short *fb = frame_buffer + r * 240;

    // screen_update: bitmap.fill(sp palette pen 0), then the backgrounds -
    // the fill and backgrounds are skipped where an opaque tile row covers
    // the column (it overwrites every pixel).
    // (ty = (Y0 + c - 16) & 255; opaque when ty >> 3 <= 6)
    const unsigned short back = mp_sp_pal[0];
    {
      for (int cc = 0; cc < 256; cc++)
      {
        int ty = (MPATROL_Y0 + cc) & 255;
        if ((ty >> 3) > 6)
          fb[cc] = back;
      }
    }
    for (int li = 0; li < nl; li++)
    {
      int image = layers[li];
      int xpos = (image == 0 ? s.bgxpos[1] : s.bgxpos[0]) + 124;
      int ypos = (image == 0 ? s.bgypos[1] : s.bgypos[0]) + 16;
      // the image is drawn at xpos and xpos - 256, covering every visible x;
      // rows 0-63 from the ROM (pen 0 transparent), rows 64-127 pen 3 and
      // the fill (ypos + 128 .. + 255) pen 3 as well
      int ix = (x - xpos) & 255;
      const unsigned short *pal = &mp_bg_pal[image * 4];
      const unsigned char *img = &mp_bg[image][0][ix];
      int c0 = ypos - MPATROL_Y0;             // column of image row 0
      int a = c0 < 0 ? 0 : c0;
      int b = c0 + 64 < 240 ? c0 + 64 : 240;
      for (int cc = a; cc < b; cc++)
      {
        unsigned char pen = img[(cc - c0) * 256];
        if (pen)
          fb[cc] = pal[pen];
      }
      a = c0 + 64 < 0 ? 0 : c0 + 64;
      b = c0 + 256 < 240 ? c0 + 256 : 240;
      const unsigned short p3 = pal[3];
      for (int cc = a; cc < b; cc++)
        fb[cc] = p3;
    }

    // tilemap: TILEMAP_SCAN_ROWS 32x32, scrolldx 127, scrolldy 16, 4 scroll
    // rows (scroll_w: rows 0-2 scrollx 255, row 3 -(data + 1)); tile rows 0-6
    // TILE_FORCE_LAYER0 (opaque), the others pen 0 transparent. One tile run
    // (up to 8 columns of one tile) at a time; a run whose pixels are all
    // pen 0 in this column is skipped (mp_tx_colmask).
    int c = 0;
    while (c < 240)
    {
      int ty = (MPATROL_Y0 + c - 16) & 255;
      int run = 8 - (ty & 7);
      //if (run > 240 - c)
      //  run = 240 - c;
      int tx = (x - ((ty >> 6) == 3 ? value_row3 : value_static)) & 255;
      int idx = (ty >> 3) * 32 + (tx >> 3);
      unsigned char color = s.cram[idx];
      int code = s.vram[idx] | ((color & 0x80) << 1);
      bool op = (ty >> 3) <= 6;
      if (op || mp_tx_colmask[code][tx & 7])
      {
        const unsigned char *g = &mp_tx[code][ty & 7][tx & 7];
        const unsigned short *pal = &mp_tx_pal[(color & 0x7f) * 4];
        for (int j = 0; j < run; j++)
        {
          unsigned char pen = g[j * 8];
          if (pen || op)
            fb[c + j] = pal[pen];
        }
      }
      c += run;
    }

    // sprites: transparent where the colour's clut entry is 0 (transpen_mask)
    for (int k = 0; k < nspr; k++)
    {
      int offs = spr_list[k];
      int sx = s.spr[offs + 3] + 129;
      if (x < sx || x >= sx + 16)
        continue;
      int sy = 257 - s.spr[offs];
      int color = (s.spr[offs + 1] & 0x3f) & 15;   // color % colors() (16)
      bool flipx = (s.spr[offs + 1] & 0x40) != 0;
      bool flipy = (s.spr[offs + 1] & 0x80) != 0;
      int code = s.spr[offs + 2] & 127;             // code % elements() (128)
      int px = flipx ? 15 - (x - sx) : (x - sx);
      const unsigned char *clut = &mp_sp_clut[color * 8];
      const unsigned short *pal = &mp_sp_pal[color * 8];
      int j0 = MPATROL_Y0 - sy;                     // first j on screen
      if (j0 < 0) j0 = 0;
      int j1 = 240 + MPATROL_Y0 - sy;               // first j below the screen
      if (j1 > 16) j1 = 16;
      for (int j = j0; j < j1; j++)
      {
        int py = flipy ? 15 - j : j;
        unsigned char pen = mp_spr[code][py][px];
        if (clut[pen])
          fb[sy + j - MPATROL_Y0] = pal[pen];
      }
    }
  }
}

// ============================================================================
// Menu, LEDs
// ============================================================================

#ifdef LED_PIN
// The moon buggy's six wheels rolling along.
void mpatrol::gameLeds(CRGB *leds)
{
  static char sub_cnt = 0;
  static char pos = 0;
  if (sub_cnt++ < 6)
    return;
  sub_cnt = 0;
  for (char c = 0; c < NUM_LEDS; c++)
    leds[c] = ((c + pos) % 3 == 0) ? LED_MAGENTA : LED_BLUE;
  pos = (pos + 1) % 3;
}

void mpatrol::menuLeds(CRGB *leds)
{
  memcpy(leds, menu_leds, NUM_LEDS * sizeof(CRGB));
}
#endif
