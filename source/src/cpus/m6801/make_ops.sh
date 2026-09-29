#!/bin/sh
# Regenerates m6801_ops.inc from the MAME source tree (run from source/).
{ echo "// Generated from MAME 0.289 src/devices/cpu/m6800/6800ops.hxx (BSD-3-Clause,"
  echo "// Aaron Giles). Only change: the OP_HANDLER #define line is removed - m6801.cpp"
  echo "// defines it. Regenerate with src/cpus/m6801/make_ops.sh, never hand-edit."
  sed '/^#define OP_HANDLER(_name) void m6800_cpu_device::_name ()$/d' mame/mame-master/src/devices/cpu/m6800/6800ops.hxx
} > src/cpus/m6801/m6801_ops.inc
# 6803 opcode + cycle tables from m6801.cpp (m6803_insn[], cycles_6803[])
M=mame/mame-master/src/devices/cpu/m6800/m6801.cpp
{ echo "// Generated from MAME 0.289 m6801.cpp (cycles_6803[], m6803_insn[]) by make_ops.sh"
  echo "#define XX 4 // illegal opcode unknown cycle count"
  echo "static const uint8_t DRAM_ATTR cycles_6803[256] ="
  sed -n '/^const u8 m6801_cpu_device::cycles_6803\[256\] =$/,/^};$/p' $M | sed '1d'
  echo "#undef XX"
  echo "static void (*const DRAM_ATTR m6803_insn[0x100])() = {"
  sed -n '/^const m6800_cpu_device::op_func m6801_cpu_device::m6803_insn\[0x100\] = {$/,/^};$/p' $M | sed '1d' | sed 's/&m6801_cpu_device::/\&/g'
} > src/cpus/m6801/m6801_tables.inc
