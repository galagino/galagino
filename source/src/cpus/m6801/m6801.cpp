// MC6803 core - see m6801.h. Every function names the MAME function it is
// taken from; the opcodes are MAME's own (m6801_ops.inc = 6800ops.hxx) and
// the opcode/cycle tables are generated from m6801.cpp (m6801_tables.inc).

#pragma GCC optimize("-O2")
#pragma GCC diagnostic ignored "-Wunused-function"

#include "m6801.h"

#ifdef ARDUINO
#include <esp_attr.h>
#else
#ifndef IRAM_ATTR
#define IRAM_ATTR
#endif
#ifndef DRAM_ATTR
#define DRAM_ATTR
#endif
#endif

typedef uint8_t u8;
typedef uint16_t u16;
typedef int16_t s16;
typedef uint32_t u32;
typedef m6801_pair PAIR;

// the running CPU (one at a time)
static m6801_state *M;

// ---- m6800.cpp register / memory macros ------------------------------------
#define pPPC    M->ppc
#define pPC     M->pc
#define pS      M->s
#define pX      M->x
#define pD      M->d

#define m_d     M->d
#define m_x     M->x
#define m_s     M->s
#define m_pc    M->pc
#define m_ea    M->ea
#define m_cc    M->cc
#define m_wai_state M->wai_state

#define PC      M->pc.w.l
#define PCD     M->pc.d
#define S       M->s.w.l
#define SD      M->s.d
#define X       M->x.w.l
#define D       M->d.w.l
#define A       M->d.b.h
#define B       M->d.b.l
#define CC      M->cc

#define EAD     M->ea.d
#define EA      M->ea.w.l

enum { M6800_WAI = 8, M6800_SLP = 0x10 };

static u8 IRAM_ATTR RM(u32 addr);
static void IRAM_ATTR WM(u32 addr, u8 value);

#define M_RDOP(Addr)     RM((Addr) & 0xffff)
#define M_RDOP_ARG(Addr) RM((Addr) & 0xffff)

#define IMMBYTE(b)  b = M_RDOP_ARG(PCD); PC++
#define IMMWORD(w)  w.d = (M_RDOP_ARG(PCD)<<8) | M_RDOP_ARG((PCD+1)&0xffff); PC+=2

#define PUSHBYTE(b) WM(SD,b); --S
#define PUSHWORD(w) WM(SD,w.b.l); --S; WM(SD,w.b.h); --S
#define PULLBYTE(b) S++; b = RM(SD)
#define PULLWORD(w) S++; w.d = RM(SD)<<8; S++; w.d |= RM(SD)

#define CLR_HNZVC   CC&=0xd0
#define CLR_NZV     CC&=0xf1
#define CLR_HNZC    CC&=0xd2
#define CLR_NZVC    CC&=0xf0
#define CLR_Z       CC&=0xfb
#define CLR_ZC      CC&=0xfa
#define CLR_C       CC&=0xfe

#define SET_Z(a)        if(!(a))SEZ
#define SET_Z8(a)       SET_Z(u8(a))
#define SET_Z16(a)      SET_Z(u16(a))
#define SET_N8(a)       CC|=(((a)&0x80)>>4)
#define SET_N16(a)      CC|=(((a)&0x8000)>>12)
#define SET_H(a,b,r)    CC|=((((a)^(b)^(r))&0x10)<<1)
#define SET_C8(a)       CC|=(((a)&0x100)>>8)
#define SET_C16(a)      CC|=(((a)&0x10000)>>16)
#define SET_V8(a,b,r)   CC|=((((a)^(b)^(r)^((r)>>1))&0x80)>>6)
#define SET_V16(a,b,r)  CC|=((((a)^(b)^(r)^((r)>>1))&0x8000)>>14)

static const u8 DRAM_ATTR flags8i[256] = /* increment */
{
0x04,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x0a,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,
0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,
0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,
0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,
0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,
0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,
0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,
0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08
};

static const u8 DRAM_ATTR flags8d[256] = /* decrement */
{
0x04,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x02,
0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,
0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,
0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,
0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,
0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,
0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,
0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,
0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08
};

#define SET_FLAGS8I(a)      {CC|=flags8i[(a)&0xff];}
#define SET_FLAGS8D(a)      {CC|=flags8d[(a)&0xff];}

#define SET_NZ8(a)          {SET_N8(a);SET_Z8(a);}
#define SET_NZ16(a)         {SET_N16(a);SET_Z16(a);}
#define SET_FLAGS8(a,b,r)   {SET_N8(r);SET_Z8(r);SET_V8(a,b,r);SET_C8(r);}
#define SET_FLAGS16(a,b,r)  {SET_N16(r);SET_Z16(r);SET_V16(a,b,r);SET_C16(r);}

#define SIGNED(b) (s16(b&0x80?b|0xff00:b))

#define DIRECT IMMBYTE(EAD)
#define IMM8 EA=PC++
#define IMM16 {EA=PC;PC+=2;}
#define EXTENDED IMMWORD(m_ea)
#define INDEXED {EA=X+(u8)M_RDOP_ARG(PCD);PC++;}

#define SEC CC|=0x01
#define CLC CC&=0xfe
#define SEZ CC|=0x04
#define CLZ CC&=0xfb
#define SEN CC|=0x08
#define CLN CC&=0xf7
#define SEV CC|=0x02
#define CLV CC&=0xfd
#define SEH CC|=0x20
#define CLH CC&=0xdf
#define SEI CC|=0x10
#define CLI CC&=~0x10

#define DIRBYTE(b) {DIRECT;b=RM(EAD);}
#define DIRWORD(w) {DIRECT;w.d=RM16(EAD);}
#define EXTBYTE(b) {EXTENDED;b=RM(EAD);}
#define EXTWORD(w) {EXTENDED;w.d=RM16(EAD);}

#define IDXBYTE(b) {INDEXED;b=RM(EAD);}
#define IDXWORD(w) {INDEXED;w.d=RM16(EAD);}

#define BRANCH(f) {IMMBYTE(t);if(f){PC+=SIGNED(t);}}
#define NXORV  ((CC&0x08)^((CC&0x02)<<2))
#define NXORC  ((CC&0x08)^((CC&0x01)<<3))

#define logerror(...) do { } while (0)

// ---- m6801.cpp timer ---------------------------------------------------------
#define CT      M->counter.w.l
#define CTH     M->counter.w.h
#define CTD     M->counter.d
#define OC      M->output_compare.w.l
#define OCH     M->output_compare.w.h
#define OCD     M->output_compare.d
#define TOH     M->timer_over.w.h
#define TOD     M->timer_over.d

#define TCSR_OLVL   0x01
#define TCSR_IEDG   0x02
#define TCSR_ETOI   0x04
#define TCSR_EOCI   0x08
#define TCSR_EICI   0x10
#define TCSR_TOF    0x20
#define TCSR_OCF    0x40
#define TCSR_ICF    0x80

#define M6801_TRCSR_RDRF 0x80
#define M6801_TRCSR_ORFE 0x40
#define M6801_TRCSR_TDRE 0x20
#define M6801_TRCSR_RIE  0x10
#define M6801_TRCSR_TIE  0x04
#define M6801_TRCSR_TE   0x02

static void IRAM_ATTR check_irq_lines();
// execute_one / increment_counter run once per instruction: inlined into
// the run loop (same code, no call)
#if defined(_MSC_VER)
#define M6801_INLINE __forceinline
#else
#define M6801_INLINE inline __attribute__((always_inline))
#endif
static M6801_INLINE void IRAM_ATTR execute_one();
static M6801_INLINE void IRAM_ATTR increment_counter(int amount);
static void IRAM_ATTR eat_cycles();
static void IRAM_ATTR take_trap() { }
static void IRAM_ATTR write_port2();

// ---- m6800.cpp: RM16 / WM16 --------------------------------------------------
static u32 IRAM_ATTR RM16(u32 Addr)
{
  u32 result = RM(Addr) << 8;
  return result | RM((Addr + 1) & 0xffff);
}

static void IRAM_ATTR WM16(u32 Addr, PAIR *p)
{
  WM(Addr, p->b.h);
  WM((Addr + 1) & 0xffff, p->b.l);
}

// ---- the opcodes (6800ops.hxx) -------------------------------------------------
// every opcode runs from IRAM: the render core's flash traffic stalls
// anything the emulation core fetches from flash
#define OP_HANDLER(_name) static void IRAM_ATTR _name ()
#include "m6801_ops.inc"

// ---- the 6803 tables (m6801.cpp) -----------------------------------------------
#include "m6801_tables.inc"

// ---- m6801.cpp timer ------------------------------------------------------------
// m6801_cpu_device::set_timer_event()
static void IRAM_ATTR set_timer_event()
{
  M->timer_next = (OCD < TOD) ? OCD : TOD;
}

// m6801_cpu_device::modified_counters()
static void IRAM_ATTR modified_counters()
{
  OCH = (OC >= CT) ? CTH : CTH + 1;
  set_timer_event();
}

static bool IRAM_ATTR check_irq2_ici() { return (M->tcsr & (TCSR_EICI|TCSR_ICF)) == (TCSR_EICI|TCSR_ICF); }
static bool IRAM_ATTR check_irq2_oci() { return (M->tcsr & (TCSR_EOCI|TCSR_OCF)) == (TCSR_EOCI|TCSR_OCF); }
static bool IRAM_ATTR check_irq2_toi() { return (M->tcsr & (TCSR_ETOI|TCSR_TOF)) == (TCSR_ETOI|TCSR_TOF); }
static bool IRAM_ATTR check_irq2_sci()
{
  return (((M->trcsr & (M6801_TRCSR_RIE|M6801_TRCSR_RDRF)) == (M6801_TRCSR_RIE|M6801_TRCSR_RDRF)) ||
          ((M->trcsr & (M6801_TRCSR_RIE|M6801_TRCSR_ORFE)) == (M6801_TRCSR_RIE|M6801_TRCSR_ORFE)) ||
          ((M->trcsr & (M6801_TRCSR_TIE|M6801_TRCSR_TDRE)) == (M6801_TRCSR_TIE|M6801_TRCSR_TDRE)));
}

// m6800_cpu_device::enter_interrupt()
static void IRAM_ATTR enter_interrupt(u16 irq_vector)
{
  int cycles_to_eat = 0;
  if (m_wai_state & M6800_WAI)
  {
    cycles_to_eat = 4;
    m_wai_state &= ~M6800_WAI;
  }
  else
  {
    PUSHWORD(pPC);
    PUSHWORD(pX);
    PUSHBYTE(A);
    PUSHBYTE(B);
    PUSHBYTE(CC);
    cycles_to_eat = 12;
  }
  SEI;
  PCD = RM16(irq_vector);
  increment_counter(cycles_to_eat);
}

// m6801_cpu_device::take_irq2()
static void IRAM_ATTR take_irq2(u16 irq_vector)
{
  m_wai_state &= ~M6800_SLP;
  if (!(m_cc & 0x10))
    enter_interrupt(irq_vector);
}

// m6801_cpu_device::check_irq2()
static void IRAM_ATTR check_irq2()
{
  if (check_irq2_ici())
    take_irq2(0xfff6);
  else if (check_irq2_oci())
    take_irq2(0xfff4);
  else if (check_irq2_toi())
    take_irq2(0xfff2);
  else if (check_irq2_sci())
    take_irq2(0xfff0);
}

// m6800_cpu_device::check_irq_lines()
static void IRAM_ATTR check_irq_lines()
{
  if (M->nmi_pending)
  {
    m_wai_state &= ~M6800_SLP;
    M->nmi_pending = false;
    enter_interrupt(0xfffc);
  }
  else if (M->irq_state[M6801_IRQ_LINE] != 0)   // check_irq1_enabled()
  {
    m_wai_state &= ~M6800_SLP;
    if (!(CC & 0x10))
      enter_interrupt(0xfff8);
  }
  else
    check_irq2();
}

// m6801_cpu_device::check_timer_event()
static void IRAM_ATTR check_timer_event()
{
  // OCI
  if (CTD >= OCD)
  {
    OCH++; // next IRQ point
    M->tcsr |= TCSR_OCF;
    M->pending_tcsr |= TCSR_OCF;

    // if output on P21 is enabled, let's do it
    if (M->port_ddr[1] & 2)
    {
      M->port_data[1] &= ~2;
      M->port_data[1] |= (M->tcsr & TCSR_OLVL) << 1;
      M->port2_written = true;
      write_port2();
    }
  }

  // TOI
  if (CTD >= TOD)
  {
    TOH++; // next IRQ point
    M->tcsr |= TCSR_TOF;
    M->pending_tcsr |= TCSR_TOF;
  }

  check_irq2();

  // set next event
  set_timer_event();
}

// m6801_cpu_device::increment_counter()
static M6801_INLINE void IRAM_ATTR increment_counter(int amount)
{
  M->icount -= amount;          // m6800_cpu_device::increment_counter()
  M->total_cycles += amount;
  CTD += amount;
  if (CTD >= M->timer_next)
    check_timer_event();
}

// m6801_cpu_device::eat_cycles()
static void IRAM_ATTR eat_cycles()
{
  int cycles_to_eat = (int)(M->timer_next - CTD);
  if (M->icount < cycles_to_eat)
    cycles_to_eat = M->icount;
  if (cycles_to_eat > 0)
    increment_counter(cycles_to_eat);
}

// m6801_cpu_device::cleanup_counters()
static void IRAM_ATTR cleanup_counters()
{
  OCH -= CTH;
  TOH -= CTH;
  CTH = 0;
  set_timer_event();
  if (CTD >= M->timer_next)
    check_timer_event();
}

// ---- m6801.cpp ports ---------------------------------------------------------------
// m6801_cpu_device::write_port2()
static void IRAM_ATTR write_port2()
{
  if (!M->port2_written) return;

  u8 data = M->port_data[1];
  u8 ddr = M->port_ddr[1] & 0x1f;

  if ((ddr != 0x1f) && ddr)
    data = (M->port_data[1] & ddr) | (ddr ^ 0xff);

  if (M->trcsr & M6801_TRCSR_TE)
  {
    data = (data & 0xef) | (1 << 4);   // m_tx idles high (no transmission emulated)
    ddr |= 0x10;
  }

  data &= 0x1f;

  if (M->port_w)
    M->port_w(M->ctx, 1, data, ddr);
}

static u8 IRAM_ATTR port_in(int port)
{
  return M->port_r ? M->port_r(M->ctx, port) : 0xff;   // m_in_port_func default 0xff
}

// m6801_io map (0000-001f) + m6803_mem RAM (0080-00ff); the rest is external
static u8 IRAM_ATTR io_r(u16 addr)
{
  switch (addr)
  {
  case 0x00: case 0x01: case 0x04: case 0x05:
    return 0xff;                                        // ff_r (DDRs)
  case 0x02:                                            // p1_data_r
  {
    u8 ddr = M->port_ddr[0];
    if (ddr == 0xff) return M->port_data[0];
    return (port_in(0) & ~ddr) | (M->port_data[0] & ddr);
  }
  case 0x03:                                            // p2_data_r
  {
    u8 ddr = M->port_ddr[1];
    if (ddr == 0xff) return M->port_data[1];
    return (port_in(1) & ~ddr) | (M->port_data[1] & ddr);
  }
  case 0x06: case 0x07:                                 // p3/p4_data_r
  {
    int n = addr == 0x06 ? 2 : 3;
    u8 ddr = M->port_ddr[n];
    if (ddr == 0xff) return M->port_data[n];
    return (port_in(n) & ~ddr) | (M->port_data[n] & ddr);
  }
  case 0x08:                                            // tcsr_r
    M->pending_tcsr = 0;
    return M->tcsr;
  case 0x09:                                            // ch_r
    if (!(M->pending_tcsr & TCSR_TOF))
      M->tcsr &= ~TCSR_TOF;
    return M->counter.b.h;
  case 0x0a: return M->counter.b.l;                     // cl_r
  case 0x0b: return M->output_compare.b.h;              // ocrh_r
  case 0x0c: return M->output_compare.b.l;              // ocrl_r
  case 0x0d:                                            // icrh_r
    if (!(M->pending_tcsr & TCSR_ICF))
      M->tcsr &= ~TCSR_ICF;
    return (M->input_capture >> 8) & 0xff;
  case 0x0e: return M->input_capture & 0xff;            // icrl_r
  case 0x0f: return M->p3csr;                           // p3_csr_r (no IS3 latch here)
  case 0x10: return M->rmcr;                            // sci_rmcr_r
  case 0x11: return M->trcsr;                           // sci_trcsr_r (flag clear sequence not emulated)
  case 0x12: return 0;                                  // sci_rdr_r (nothing received)
  case 0x14: return M->ram_ctrl | 0x3f;                 // rcr_r
  default:   return 0;                                  // 13, 15-1f: not mapped here
  }
}

static void IRAM_ATTR io_w(u16 addr, u8 data)
{
  switch (addr)
  {
  case 0x00:                                            // p1_ddr_w
    if (M->port_ddr[0] != data)
    {
      M->port_ddr[0] = data;
      if (M->port_w) M->port_w(M->ctx, 0, (M->port_data[0] & M->port_ddr[0]) | (M->port_ddr[0] ^ 0xff), M->port_ddr[0]);
    }
    break;
  case 0x01:                                            // p2_ddr_w
    if (M->port_ddr[1] != data)
    {
      M->port_ddr[1] = data;
      write_port2();
    }
    break;
  case 0x02:                                            // p1_data_w
    M->port_data[0] = data;
    if (M->port_w) M->port_w(M->ctx, 0, (M->port_data[0] & M->port_ddr[0]) | (M->port_ddr[0] ^ 0xff), M->port_ddr[0]);
    break;
  case 0x03:                                            // p2_data_w
    M->port_data[1] = data;
    M->port2_written = true;
    write_port2();
    break;
  case 0x04: M->port_ddr[2] = data; break;              // p3_ddr_w (port 3/4 not wired here)
  case 0x05: M->port_ddr[3] = data; break;              // p4_ddr_w
  case 0x06: M->port_data[2] = data; break;
  case 0x07: M->port_data[3] = data; break;
  case 0x08:                                            // tcsr_w
    data &= 0x1f;
    M->tcsr = data | (M->tcsr & 0xe0);
    M->pending_tcsr &= M->tcsr;
    check_irq2();
    break;
  case 0x09:                                            // ch_w
    M->latch09 = data;
    CT = 0xfff8;
    TOH = CTH;
    modified_counters();
    break;
  case 0x0a:                                            // cl_w
    CT = (M->latch09 << 8) | data;
    TOH = CTH;
    modified_counters();
    break;
  case 0x0b:                                            // ocrh_w
    if (!(M->pending_tcsr & TCSR_OCF))
      M->tcsr &= ~TCSR_OCF;
    if (M->output_compare.b.h != data)
    {
      M->output_compare.b.h = data;
      modified_counters();
    }
    break;
  case 0x0c:                                            // ocrl_w
    if (!(M->pending_tcsr & TCSR_OCF))
      M->tcsr &= ~TCSR_OCF;
    if (M->output_compare.b.l != data)
    {
      M->output_compare.b.l = data;
      modified_counters();
    }
    break;
  case 0x0f: M->p3csr = data; break;
  case 0x10: M->rmcr = data; break;
  case 0x11: M->trcsr = (M->trcsr & 0xe0) | (data & 0x1f); break;
  case 0x14: M->ram_ctrl = data; break;                 // rcr_w
  default: break;
  }
}

// MAME builds the CPU's internal map (m6803_mem) into the same address_map
// as the board's map, so:
//  - the board map's global_mask (addr_mask) applies to the internal
//    registers and RAM as well (e.g. 8080 = 0080 with a 7fff mask)
//  - an internal entry only replaces the direction it defines: 0d/0e (ICR)
//    and 12 (RDR) are read-only, 13 (TDR) write-only - the other direction
//    of those addresses is the board's
static u8 IRAM_ATTR RM_bus(u32 addr)
{
  if (M->rom && (addr & M->rom_match_mask) == M->rom_match)
    return M->rom[addr & M->rom_index_mask];
  const u32 a = addr & M->addr_mask;
  if (a < 0x100)
  {
    if (a >= 0x80)
      return M->internal_ram[a - 0x80];
    if (a <= 0x14 && a != 0x13)
      return io_r((u16)a);
  }
  return M->read(M->ctx, (u16)addr);
}

static u8 IRAM_ATTR RM(u32 addr)
{
#ifdef M6801_DBG_HOOKS
  u8 v = RM_bus(addr);
  if (M->dbg_r)
    M->dbg_r(M->ctx, (u16)addr, v);
  return v;
#else
  return RM_bus(addr);
#endif
}

static void IRAM_ATTR WM(u32 addr, u8 value)
{
#ifdef M6801_DBG_HOOKS
  if (M->dbg_w)
    M->dbg_w(M->ctx, (u16)addr, value);
#endif
  const u32 a = addr & M->addr_mask;       // see RM_bus
  if (a < 0x100)
  {
    if (a >= 0x80)
    {
      M->internal_ram[a - 0x80] = value;
      return;
    }
    if (a <= 0x14 && a != 0x0d && a != 0x0e && a != 0x12)
    {
      io_w((u16)a, value);
      return;
    }
  }
  M->write(M->ctx, (u16)addr, value);
}

// ---- m6800.cpp execute ------------------------------------------------------------
// m6800_cpu_device::execute_one()
static M6801_INLINE void IRAM_ATTR execute_one()
{
  pPPC = pPC;
#ifdef M6801_DBG_HOOKS
  u8 ireg = M_RDOP(PCD);
#else
  // M_RDOP(PCD) = RM(PCD & 0xffff), with RM_bus()'s ROM test done here
  const u32 addr = PCD & 0xffff;
  u8 ireg = (M->rom && (addr & M->rom_match_mask) == M->rom_match) ? M->rom[addr & M->rom_index_mask] : RM_bus(addr);
#endif
  PC++;
  m6803_insn[ireg]();
  increment_counter(cycles_6803[ireg]);
}

// m6800_cpu_device::execute_run()
int IRAM_ATTR m6801_run(m6801_state *s, int cycles)
{
  M = s;
  M->icount = cycles;
  M->stolen = 0;

  check_irq_lines();
  cleanup_counters();

  do
  {
    if (m_wai_state & (M6800_WAI | M6800_SLP))
      eat_cycles();
    else if (PC == M->idle_pc && M->idle_pc && M->internal_ram[M->idle_addr - 0x80] == 0)
    {
      // idle loop (see m6801.h): LDA dir (3 cycles), BEQ (3 cycles, taken)
      // As execute_one(): PC is past the instruction when its cycles are
      // charged, so a timer interrupt taken in increment_counter() pushes
      // the same return address.
      const u16 loop_pc = PC;
      do
      {
        pPPC = pPC;
        PC = loop_pc + 2;
        A = 0;
        CLR_NZV; SEZ;                      // lda: SET_NZ8(0)
        increment_counter(3);
        if (PC != (u16)(loop_pc + 2) || M->icount <= 0)
          break;                           // interrupt taken / the run ends before the BEQ
        pPPC = pPC;
        PC = loop_pc;                      // beq taken
        increment_counter(3);
      } while (M->icount > 0 && PC == loop_pc && M->internal_ram[M->idle_addr - 0x80] == 0);
    }
    else
      execute_one();
  } while (M->icount > 0);

  // device_scheduler::timeslice(): ran = cycles - icount - cycles_stolen
  return cycles - M->icount - M->stolen;
}

// device_execute_interface::abort_timeslice()
void IRAM_ATTR m6801_abort_timeslice(m6801_state *s)
{
  int delta = s->icount;
  s->stolen += delta;
  s->icount -= delta;
}

int IRAM_ATTR m6801_cycles_done(const m6801_state *s, int cycles)
{
  return cycles - s->icount - s->stolen;
}

// m6800_cpu_device::execute_set_input() + m6801_cpu_device::execute_set_input()
void IRAM_ATTR m6801_set_input(m6801_state *s, int line, int state)
{
  M = s;
  switch (line)
  {
  case M6801_NMI_LINE:
    if (!M->nmi_state && state)
      M->nmi_pending = true;
    M->nmi_state = state;
    break;
  case M6801_TIN_LINE:
    if (state != M->irq_state[M6801_TIN_LINE])
    {
      M->irq_state[M6801_TIN_LINE] = state;
      if (((M->tcsr & TCSR_IEDG) ^ (state == 0 ? TCSR_IEDG : 0)) == 0)
        return;
      M->tcsr |= TCSR_ICF;
      M->pending_tcsr |= TCSR_ICF;
      M->input_capture = CT;
    }
    break;
  default:
    M->irq_state[line] = state;
    break;
  }
}

// m6800_cpu_device::device_start() + m6801 (register values before reset)
void m6801_power_on(m6801_state *s)
{
  if (!s->addr_mask)
    s->addr_mask = 0xffff;                               // no board global_mask
  s->ppc.d = s->pc.d = s->s.d = s->x.d = s->d.d = s->ea.d = 0;
  s->cc = 0;
  s->wai_state = 0;
  s->nmi_state = 0;
  s->nmi_pending = 0;
  s->irq_state[0] = s->irq_state[1] = s->irq_state[2] = 0;
  s->ram_ctrl = 0;
  s->rmcr = 0;
  s->input_capture = 0;
  s->total_cycles = 0;
  for (int i = 0; i < 4; i++)
    s->port_data[i] = 0;
  for (int i = 0; i < 128; i++)
    s->internal_ram[i] = 0;
  m6801_reset(s);
}

// m6800_cpu_device::device_reset() + m6801_cpu_device::device_reset()
void m6801_reset(m6801_state *s)
{
  M = s;
  m_cc = 0xc0;
  SEI;
  PCD = RM16(0xfffe);
  m_wai_state = 0;
  M->nmi_state = 0;
  M->nmi_pending = 0;

  for (int i = 0; i < 4; i++)
    M->port_ddr[i] = 0;
  M->p3csr = 0;
  M->port2_written = false;
  M->tcsr = 0;
  M->pending_tcsr = 0;
  CTD = 0x0000;
  OCD = 0xffff;
  TOD = 0xffff;
  M->timer_next = 0xffff;
  M->ram_ctrl |= 0x40;
  M->latch09 = 0;
  M->trcsr = M6801_TRCSR_TDRE;
  M->rmcr = 0;                   // set_rmcr(0)
}
