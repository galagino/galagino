// mpatrol's own build of the shared Z80 core (src/cpus/z80/Z80.c, included
// unchanged below). The opcodes, flags and cycle tables are the shared
// core's; only the memory access differs: the shared core reaches the
// machine through OpZ80_INL / RdZ80 / WrZ80 (emulation.cpp), two calls and a
// virtual call per byte, while here ROM (0000-3fff), video/colour RAM
// (8000-87ff) and work RAM (e000-e7ff) are read and written inline - the
// same bytes mpatrol::opZ80 / rdZ80 / wrZ80 use (mpatrol.cpp main_map).
// Everything else (8800 protection, c800 sprites, d000 I/O, unmapped) still
// goes through RdZ80 / WrZ80, so the harness sees every d000 access as before.
//
// MPATROL_Z80_IRAM (below): the core's hot functions in IRAM. That fits
// when mpatrol is the only machine built (the shared core is then unused and
// left out by the linker); set it to 0 when building mpatrol together with
// other machines if the link reports an IRAM overflow.

#pragma GCC optimize("-O2")

#include <stdint.h>
#include "../../cpus/z80/Z80.h"

#ifndef MPATROL_Z80_IRAM
#define MPATROL_Z80_IRAM 1
#endif
#if !MPATROL_Z80_IRAM
#undef IRAM_ATTR
#define IRAM_ATTR
#endif

// set by mpatrol::reset() (mpatrol.cpp)
const unsigned char *mpz_rom;       // main CPU ROM 0000-3fff (RAM copy or flash)
unsigned char *mpz_mem;             // machineBase memory[] (mpatrol.h layout)

static __inline byte mpz_Rd(word A)
{
  if (A < 0x4000)                                   // map(0x0000, 0x3fff).rom()
    return mpz_rom[A];
  if ((A & 0xf800) == 0xe000)                       // e000-e7ff ram (MPATROL_WORKRAM)
    return mpz_mem[0x0c00 + (A & 0x07ff)];
  if ((A & 0xf800) == 0x8000)                       // 8000-87ff video/colour ram (MPATROL_VIDEORAM)
    return mpz_mem[A & 0x07ff];
  return RdZ80(A);                                  // mpatrol::rdZ80
}

static __inline void mpz_Wr(word A, byte V)
{
  if ((A & 0xf800) == 0xe000)
  {
    mpz_mem[0x0c00 + (A & 0x07ff)] = V;
    return;
  }
  if ((A & 0xf800) == 0x8000)
  {
    mpz_mem[A & 0x07ff] = V;
    return;
  }
  WrZ80(A, V);                                      // mpatrol::wrZ80
}

static word mpz_Loop(Z80 *R) { (void)R; return INT_NONE; }   // only ExecZ80/RunZ80 (unused)

IRAM_ATTR void mpz_Step(Z80 *R);
IRAM_ATTR void mpz_Int(Z80 *R, word Vector);

// the shared core's names -> this build's
#define OpZ80_INL mpz_Rd
#define RdZ80     mpz_Rd
#define WrZ80     mpz_Wr
#define StepZ80   mpz_Step
#define IntZ80    mpz_Int
#define ResetZ80  mpz_Reset
#define ExecZ80   mpz_Exec
#define RunZ80    mpz_Run
#define LoopZ80   mpz_Loop

#include "../../cpus/z80/Z80.c"
