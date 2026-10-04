#ifndef MPATROL_H
#define MPATROL_H

// Moon Patrol - Irem 1982. MAME set `mpatrol`.
// Ground truth (MAME 0.289, source/mame/mame-master):
//   src/mame/irem/m52.cpp      main board: memory map, inputs, video, palette
//   src/mame/irem/irem.cpp     sound board m52_soundc_audio_device
//   src/devices/cpu/m6800/     MC6803 (ported as src/cpus/m6801)
//   src/devices/sound/ay8910.cpp, msm5205.cpp
//
// Hardware: Z80 @ 18.432MHz/6 = 3.072MHz; sound: MC6803 @ 3.579545MHz
// (E = /4), 2x AY-3-8910 @ 3.579545MHz/4, MSM5205 @ 384kHz. Screen
// set_raw(18.432MHz/3, 384, 136, 376, 282, 22, 274): 240x252 visible,
// 56.74Hz, ROT0 (a landscape game).

#include "mpatrol_rom.h"
#include "mpatrol_gfx.h"
#include "mpatrol_palette.h"
#include "mpatrol_dipswitches.h"
#include "mpatrol_logo.h"
#include "../machineBase.h"
#include "../../cpus/m6801/m6801.h"

// DIP switches as read by the game: mpatrol_dipswitches.h, unless overridden
// (the harness passes MAME's defaults so the MAME comparison stays exact).
#ifndef MPATROL_DSW1
#define MPATROL_DSW1 MPATROL_DSW1_DEFAULT
#endif
#ifndef MPATROL_DSW2
#define MPATROL_DSW2 MPATROL_DSW2_DEFAULT
#endif

// memory[] layout
#define MPATROL_VIDEORAM  0x0000   // 8000-83ff
#define MPATROL_COLORRAM  0x0400   // 8400-87ff
#define MPATROL_SPRITERAM 0x0800   // c800-cbff (mirrored at cc00-cfff)
#define MPATROL_WORKRAM   0x0c00   // e000-e7ff
#define MPATROL_MEM_END   0x1400

// ---- Landscape screen mapping ------------------------------------------------
// The panel is used on its side, like boblbobl/flstory: MAME x (240 visible
// columns, 136-375) runs along the panel's 288 lines, MAME y (252 visible
// lines, 22-273) along its 224 columns. 252 lines do not fit in 224, so 28
// are cropped: MPATROL_CROP_TOP of them at the top, the rest at the bottom.
// 0 = keep the HUD, lose 28 lines of plain ground at the bottom.
#define MPATROL_VIS_X0     136     // first visible MAME x

// Output level: the discrete network's output (NODE_40) to audio.cpp's
// +-512 range. 14000 puts the loudest moment of 53s of gameplay at about 430
// (measured in the harness, -wav) - no clipping.
#ifndef MPATROL_SND_GAIN
#define MPATROL_SND_GAIN 14000.0f
#endif

// Run at the game's own 56.74Hz instead of the display's ~60Hz: a frame is
// only emulated when real time (micros) has caught up with emulated time, so
// the game and its music have MAME's speed and the sound ring neither
// overflows nor runs dry. The display then shows about one frame in 18
// twice. 0 = one frame per display frame (5.7% fast).
#ifndef MPATROL_REAL_SPEED
#define MPATROL_REAL_SPEED 1
#endif

// Sound render cost on the device (boblbobl's BOBLBOBL_SND_FAST idea): the
// AY state (tone counters, noise LFSR, envelope) always advances exactly as
// MAME's; with 1, a 24kHz output sample that contains a tone edge / noise
// change is the old value up to the first one and the new value after it,
// instead of the exact average of every stream sample. 0 = exact (the
// harness uses both).
#ifndef MPATROL_SND_FAST
#define MPATROL_SND_FAST 1
#endif

// 1 = step the AY one stream sample at a time (reference); 0 = step to the
// next counter event in one go (same result, much cheaper).
#ifndef MPATROL_SND_EXACT_STEP
#define MPATROL_SND_EXACT_STEP 0
#endif

// Skip exactly, in whole iterations: the sound CPU's idle loop (fcf5 lda $bd
// / beq, charged cycle by cycle - m6801.h idle_pc) and the Z80's table scan
// at 12b0 (mpatrol.cpp z80_scan_skip). 0 runs every instruction.
#ifndef MPATROL_IDLE_SKIP
#define MPATROL_IDLE_SKIP 1
#endif

// Print the emulation time per frame over serial once a second (device only).
#ifndef MPATROL_PROFILE
#define MPATROL_PROFILE 0
#endif

class mpatrol : public machineBase
{
public:
  mpatrol() { }
  ~mpatrol() { if (s_instance == this) s_instance = 0; }

  signed char machineType() override { return MCH_MPATROL; }

  const int   renderWidth()  override { return 240; }
  const int   renderBuffer() override { return 240 * 2 * 8; }
  signed char videoFlipX()  override { return 1; }
  signed char isLandscape() override { return 1; }

  void reset() override;

  unsigned char rdZ80(unsigned short Addr) override;
  void wrZ80(unsigned short Addr, unsigned char Value) override;
  unsigned char opZ80(unsigned short Addr) override;
  void outZ80(unsigned short Port, unsigned char Value) override;
  unsigned char inZ80(unsigned short Port) override { return 0x00; }   // no I/O reads mapped: unmap value 0

  void run_frame(void) override;
  void prepare_frame(void) override { }
  void render_row(short row) override;
  unsigned char ayEnvelopeRestarts(unsigned char ay) override { return ay < 2 ? ay_r13_writes[ay] : 0; }
  int renderFmSample() override;

  const unsigned short *logo(void) override { return mpatrol_logo; }

#ifdef LED_PIN
  void menuLeds(CRGB *leds) override;
  void gameLeds(CRGB *leds) override;
#endif

  // 6803 bus callbacks (static: the core calls them with the machine as ctx)
  static uint8_t snd_read(void *ctx, uint16_t addr);
  static void snd_write(void *ctx, uint16_t addr, uint8_t data);
  static uint8_t snd_port_r(void *ctx, int port);
  static void snd_port_w(void *ctx, int port, uint8_t data, uint8_t ddr);

private:
  static mpatrol *s_instance;
  uint32_t prof_z80 = 0, prof_snd = 0, prof_render = 0, prof_slices = 0;   // MPATROL_PROFILE (CPU cycles)
  uint32_t prof_z80_insn = 0;    // MPATROL_PROFILE: Z80 instructions run (not skipped)
  bool pace_init = false;        // MPATROL_REAL_SPEED
  uint32_t pace_last = 0;
  int32_t pace_credit = 0;

  // ---- main CPU ------------------------------------------------------------
  bool z80_irq = false;          // vblank IRQ, irq0_line_hold: dropped when taken
  bool ei_shadow = false;        // last instruction was an enabling EI
  unsigned char z80_r = 0, z80_r2 = 0; // refresh register as MAME counts it

  // ---- m52_state -----------------------------------------------------------
  unsigned char m_bgxpos[2] = { 0, 0 }, m_bgypos[2] = { 0, 0 };
  unsigned char m_bgcontrol = 0;
  unsigned char m_scroll = 0;    // last scroll_w value (tilemap row 3)
  bool m_scroll_written = false; // before the first scroll_w every row uses scrollx 0
  bool m_flip = false;
  unsigned char in_keys = 0;     // buttons sampled at the last frame update
  unsigned char service_impulse = 0;

  // ---- scheduler (see "Scheduling" in mpatrol.cpp), MAME attoseconds --------
  enum { LINE_SND_IRQ, LINE_NONE };
  struct sync_S { uint64_t time; unsigned char line; bool state; };
  sync_S sync_q[16];
  int sync_n = 0;
  uint64_t base = 0, z80_t = 0, snd_t = 0, frame_start = 0;
  uint64_t exec_base = 0;        // running CPU: local time at the start of its run
  int exec_pos = 0;              // Z80: cycles from run start to the current instruction
  int snd_want = 0;              // 6803: cycles asked for in the current run
  uint64_t cur_time(void) const;
  uint64_t snd_update_next = 0;  // sound_manager update timer (50Hz)
  uint32_t time_seconds = 0;     // whole seconds taken off the clock above
  unsigned char exec_cpu = 0;    // 0 none, 1 Z80, 2 6803
  unsigned char exec_io_offset = 0;
public:
  struct z80_io_S
  {
  	unsigned char offset;     // cycle offset of the I/O access
  	unsigned char nsteps;
  	unsigned char steps[7];   // machine cycle lengths of the whole instruction
  };
private:
  z80_io_S z80_part_io;
  unsigned char z80_part_done = 0;
  unsigned short exec_pc = 0;
  int frame_timer = 0;
  bool abort_run = false;
  int z80_exec(int want);
  bool z80_scan_skip(int &budget);
  int snd_exec(int want);
  uint64_t first_timer(void);
  void line_w(unsigned char line, bool state, uint64_t t);
  void line_apply(unsigned char line, bool state);
  void vblank_begin(void);

  // ---- irem_audio_device ---------------------------------------------------
  m6801_state snd;
  unsigned char m_port1 = 0, m_port2 = 0, m_soundlatch = 0;
  unsigned char *snd_rom_ram = 0;
  unsigned char *rom_ram = 0;    // main CPU ROM in RAM
  unsigned char *gfx_ram[3] = { 0, 0, 0 };   // tx, sprites, backgrounds in RAM
  const unsigned char *snd_rom = mpatrol_snd;
  void sound_irq_ack_w(void);
  void port2_w(unsigned char data);
  unsigned char port1_r(void);

  // ---- AY-3-8910 x2: 0 = 45M (port A in = sound latch, port B out = MSM
  // control), 1 = 45L (port A out = analog drums, not on this board) -------
  struct ay_S {
  	unsigned char regs[16];
  	unsigned char latch;
  	bool active;
  	unsigned char last_enable;
  };
  ay_S ay[2];
  volatile unsigned char ay_r13_writes[2] = { 0, 0 };
  void ay_reset(int n);
  void ay_address_w(int n, unsigned char data);
  void ay_data_w(int n, unsigned char data);
  void ay_write_reg(int n, int r, unsigned char v);
  unsigned char ay_data_r(int n);
  void ay45m_portb_w(unsigned char data);

  // ---- MSM5205 --------------------------------------------------------------
  bool msm_s1 = false, msm_s2 = false; // set_prescaler_selector(S96_4B)
  unsigned char msm_bitwidth = 4;
  unsigned char msm_data = 0;
  bool msm_vck = false, msm_reset = false;
  int msm_signal = 0, msm_step = 0;
  float msm_f = 0;               // msm_signal as the stream value
  uint64_t msm_vck_next = 0, msm_vck_period = 0;  // 0 = timer off (slave mode)
  uint64_t msm_capture_next = 0;                  // 0 = not armed
  void msm_playmode_w(unsigned char data);
  void msm_clock_changed(uint64_t now);
  void msm_toggle_vck(uint64_t now);
  void msm_update_adpcm(void);

  // ---- sound output (mpatrol.cpp "Sound output"): the mix rendered in
  // emulated time into a ring, read by the audio core -----------------------
  struct ay_synth_S {
  	int tone_period[3], tone_period_eff[3], tone_count[3];
  	unsigned char tone_duty[3], tone_output[3], tone_volume[3];
  	int count_noise;
  	unsigned char prescale_noise;
  	uint32_t rng;
  	uint32_t env_period, env_count;
  	int8_t env_step;
  	unsigned char env_attack, env_hold, env_alternate, env_holding, env_volume;
  	uint32_t mix_key;              // mix_3D() index of the cached mix_val
  	unsigned char dirty;           // a register changed: rebuild the mix
  	int quiet_left;                // steps before the next relevant event (ay_event_distance - 1)
  	int pending;                   // quiet steps not yet applied to the counters (ay_flush)
  	float mix_val;
  };
  ay_synth_S ays[2];
  enum { MIX_CACHE = 256 };
  struct mix_cache_S { uint32_t key; float val; };
  mix_cache_S mix_cache[MIX_CACHE];   // mix_3D value by key (both AYs share the formula)
  float inv_res[16];
  float hp37 = 0, lp30 = 0, hpamp = 0, e37 = 0, e30 = 0, eamp = 0;
  enum { SND_RING = 2048 };
  short snd_ring[SND_RING];
  volatile unsigned short snd_wr = 0, snd_rd = 0;
  uint64_t snd_stream_t = 0;     // time of the next AY stream sample (111860Hz)
  uint32_t snd_stream_frac = 0;  // its remainder, in 1/111860 attoseconds
  uint32_t snd_acc = 0;          // output samples (24000) per stream sample
  float snd_sum0 = 0, snd_sum1 = 0, snd_msum = 0;
  int snd_steps = 0;
  short snd_last = 0;
  void snd_out_reset(void);
  void ay_synth_reg(int n, int r);
  float ay_stream_sample(int n);
  float ay_mix(int n);
  float ay_stream_samples(int n, int k);
  float ay_stream_samples_fast(int n, int k);
  int ay_event_distance(int n);
  void ay_bulk_advance(int n, int q);
  void ay_flush(int n);
  void snd_render_to(uint64_t t);

  // ---- video ----------------------------------------------------------------
  struct snap_S {
  	unsigned char vram[0x400], cram[0x400], spr[0x100];
  	unsigned char bgxpos[2], bgypos[2], bgcontrol, scroll;
  	bool scroll_written, flip;
  };
  snap_S snap[2];
  volatile unsigned char snap_front = 0;
  unsigned char render_snap = 0;
  void publish_snapshot(void);

#ifdef LED_PIN
  const CRGB menu_leds[7] = { LED_BLUE, LED_CYAN, LED_GREEN, LED_YELLOW, LED_GREEN, LED_CYAN, LED_BLUE };
#endif

#if MPATROL_DBG_HARNESS
public:
  uint16_t dbg_pc() const { return cpu[0].PC.W; }
  uint16_t dbg_snd_pc() const { return snd.pc.w.l; }
  uint16_t dbg_snd_ppc() const { return snd.ppc.w.l; }
  unsigned char dbg_mem(unsigned short a) const { return memory[a]; }
  uint64_t dbg_now() const { return cur_time(); }
  // time in Z80 cycles since power on (MAME machine time)
  double dbg_cycles(uint64_t t) const { return time_seconds * 3072000.0 + t / 325520833333.3333; }
  // time of the current Z80 instruction's d000-d7ff access (z80_io_access)
  double dbg_io_now() const { return dbg_cycles(cur_time() + (uint64_t)exec_io_offset * 325520833333ULL); }
  uint16_t dbg_exec_pc() const { return exec_pc; }
  void (*dbg_ay_hook)(int chip, unsigned char reg, unsigned char val, double t) = 0;
  void (*dbg_cmd_hook)(unsigned char val, double t) = 0;
  void (*dbg_msm_hook)(unsigned char kind, int val, double t) = 0;
  void (*dbg_snd_pc_hook)(uint16_t pc, double t) = 0;
  void (*dbg_z80_pc_hook)(uint16_t pc, double t) = 0;
  void (*dbg_slice_hook)(uint64_t base, uint64_t z80_t, uint64_t snd_t) = 0;
  void (*dbg_timer_hook)(int kind, uint64_t t) = 0;
  const unsigned char *dbg_bg(int i) const { return &mpatrol_bg[i][0][0]; }
  unsigned char dbg_spr(int i) const { return memory[MPATROL_SPRITERAM + i]; }
  int dbg_pop_sample() { if (snd_rd == snd_wr) return 0x7fff; return renderFmSample(); }
#ifdef M6801_DBG_HOOKS
  void dbg_snd_hooks(void (*r)(void *, uint16_t, uint8_t), void (*w)(void *, uint16_t, uint8_t)) { snd.dbg_r = r; snd.dbg_w = w; }
  double dbg_snd_now() const { return dbg_cycles(cur_time()); }
#endif
#endif
};

#endif
