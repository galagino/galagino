// MC6803 (M6801 family) core, ported from MAME 0.289
//   src/devices/cpu/m6800/m6800.cpp   (execute loop, interrupts, reset)
//   src/devices/cpu/m6800/m6801.cpp   (6803 cycle table, timer, ports 1/2)
//   src/devices/cpu/m6800/6800ops.hxx (the opcodes, included unchanged as
//                                      m6801_ops.inc)
// Only what an MC6803 in expanded mode needs: ports 1 and 2, the 16-bit
// timer (OCI/TOI/ICI), the RAM control register and 128 bytes of internal
// RAM at 0080-00ff. The serial interface (SCI) registers read back their
// reset values and writes are kept but do nothing.
//
// One instance at a time runs (m6801_run sets the current state).

#ifndef M6801_H
#define M6801_H

#include <stdint.h>

typedef union
{
  struct { uint8_t l, h, h2, h3; } b;   // little endian, as MAME's PAIR
  struct { uint16_t l, h; } w;
  uint32_t d;
} m6801_pair;

struct m6801_state
{
  // m6800_cpu_device
  m6801_pair ppc, pc, s, x, d, ea;
  uint8_t cc;
  uint8_t wai_state;
  uint8_t nmi_state;
  uint8_t nmi_pending;
  uint8_t irq_state[3];     // IRQ1, TIN, IS3
  int icount;
  int stolen;               // abort_timeslice(): cycles taken back

  // m6801_cpu_device
  uint8_t port_ddr[4];
  uint8_t port_data[4];
  bool port2_written;
  uint8_t tcsr, pending_tcsr;
  m6801_pair counter, output_compare, timer_over;
  uint32_t timer_next;
  uint16_t input_capture;
  uint8_t ram_ctrl, latch09;
  uint8_t trcsr, rmcr, p3csr;
  uint8_t internal_ram[128];

  // the board
  void *ctx;
  uint8_t (*read)(void *ctx, uint16_t addr);                 // external bus
  void (*write)(void *ctx, uint16_t addr, uint8_t data);
  uint8_t (*port_r)(void *ctx, int port);                    // in_pN_cb
  void (*port_w)(void *ctx, int port, uint8_t data, uint8_t ddr);  // out_pN_cb

  uint32_t total_cycles;

  // the board map's global_mask (address_map::global_mask), which MAME also
  // applies to the internal registers and RAM; 0 before m6801_power_on() = none
  uint16_t addr_mask;

  // Optional fast paths (0 = off), both exact:
  // rom: reads with (addr & rom_match_mask) == rom_match come from
  // rom[addr & rom_index_mask] without the read callback (a board's ROM
  // area, only valid when that area has no side effects).
  const uint8_t *rom;
  uint16_t rom_match_mask, rom_match, rom_index_mask;
  // idle_pc: a "LDA <idle_addr> (direct, internal RAM) / BEQ idle_pc" loop
  // at idle_pc. While the byte is 0 each iteration is charged like the two
  // instructions (increment_counter, so timer events happen at the same
  // cycle) without fetching and decoding them. Nothing but an interrupt,
  // which is only taken at the start of a run, can change the byte here.
  uint16_t idle_pc;
  uint8_t idle_addr;

#ifdef M6801_DBG_HOOKS
  // native harness only: every program-space access, as MAME lua taps see them
  void (*dbg_r)(void *ctx, uint16_t addr, uint8_t data);
  void (*dbg_w)(void *ctx, uint16_t addr, uint8_t data);
#endif
};

enum { M6801_IRQ_LINE = 0, M6801_TIN_LINE = 1, M6801_NMI_LINE = 0x20 };

// power on: device_start() values, then device_reset()
void m6801_power_on(m6801_state *s);
void m6801_reset(m6801_state *s);
// execute_set_input()
void m6801_set_input(m6801_state *s, int line, int state);
// device_execute_interface::run(): runs `cycles`, returns the cycles used
// (more than asked when the last instruction overshoots, fewer when aborted)
int m6801_run(m6801_state *s, int cycles);
// device_execute_interface::abort_timeslice() - from inside a bus callback
void m6801_abort_timeslice(m6801_state *s);
// cycles executed so far in the current run (for time stamps inside a run):
// MAME's device_execute_interface::cycles_remaining() counterpart
int m6801_cycles_done(const m6801_state *s, int cycles);

#endif
