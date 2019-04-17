/*****************************************************************************/
/*     Generator - Sega Genesis emulation - (c) James Ponder 1997-2001       */
/*****************************************************************************/
/*                                                                           */
/* cpu68k-7.c                                                                */
/*                                                                           */
/*****************************************************************************/

#include "cpu68k-inline.h"

#include "longtab68k.h"

void cpu_op_7000a(void) /* MOVE */ {
  /* mask f100, bits 7000, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0
     set -2, size 3, stype 18, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  signed int srcdata = (sint8)(OPCODE & 0xFF);
  const int dstreg = (0x7000 >> 9) & 7;
  uint32 outdata = srcdata;

  DATAREG(dstreg) = outdata;
}

void cpu_op_7000b(void) /* MOVE */ {
  /* mask f100, bits 7000, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0
     set -2, size 3, stype 18, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  signed int srcdata = (sint8)(OPCODE & 0xFF);
  const int dstreg = (0x7000 >> 9) & 7;
  uint32 outdata = srcdata;

  DATAREG(dstreg) = outdata;

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
}

void cpu_op_7200a(void) /* MOVE */ {
  /* mask f100, bits 7000, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0
     set -2, size 3, stype 18, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  signed int srcdata = (sint8)(OPCODE & 0xFF);
  const int dstreg = (0x7200 >> 9) & 7;
  uint32 outdata = srcdata;

  DATAREG(dstreg) = outdata;
}

void cpu_op_7200b(void) /* MOVE */ {
  /* mask f100, bits 7000, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0
     set -2, size 3, stype 18, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  signed int srcdata = (sint8)(OPCODE & 0xFF);
  const int dstreg = (0x7200 >> 9) & 7;
  uint32 outdata = srcdata;

  DATAREG(dstreg) = outdata;

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
}

void cpu_op_7400a(void) /* MOVE */ {
  /* mask f100, bits 7000, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0
     set -2, size 3, stype 18, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  signed int srcdata = (sint8)(OPCODE & 0xFF);
  const int dstreg = (0x7400 >> 9) & 7;
  uint32 outdata = srcdata;

  DATAREG(dstreg) = outdata;
}

void cpu_op_7400b(void) /* MOVE */ {
  /* mask f100, bits 7000, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0
     set -2, size 3, stype 18, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  signed int srcdata = (sint8)(OPCODE & 0xFF);
  const int dstreg = (0x7400 >> 9) & 7;
  uint32 outdata = srcdata;

  DATAREG(dstreg) = outdata;

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
}

void cpu_op_7600a(void) /* MOVE */ {
  /* mask f100, bits 7000, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0
     set -2, size 3, stype 18, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  signed int srcdata = (sint8)(OPCODE & 0xFF);
  const int dstreg = (0x7600 >> 9) & 7;
  uint32 outdata = srcdata;

  DATAREG(dstreg) = outdata;
}

void cpu_op_7600b(void) /* MOVE */ {
  /* mask f100, bits 7000, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0
     set -2, size 3, stype 18, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  signed int srcdata = (sint8)(OPCODE & 0xFF);
  const int dstreg = (0x7600 >> 9) & 7;
  uint32 outdata = srcdata;

  DATAREG(dstreg) = outdata;

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
}

void cpu_op_7800a(void) /* MOVE */ {
  /* mask f100, bits 7000, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0
     set -2, size 3, stype 18, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  signed int srcdata = (sint8)(OPCODE & 0xFF);
  const int dstreg = (0x7800 >> 9) & 7;
  uint32 outdata = srcdata;

  DATAREG(dstreg) = outdata;
}

void cpu_op_7800b(void) /* MOVE */ {
  /* mask f100, bits 7000, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0
     set -2, size 3, stype 18, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  signed int srcdata = (sint8)(OPCODE & 0xFF);
  const int dstreg = (0x7800 >> 9) & 7;
  uint32 outdata = srcdata;

  DATAREG(dstreg) = outdata;

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
}

void cpu_op_7a00a(void) /* MOVE */ {
  /* mask f100, bits 7000, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0
     set -2, size 3, stype 18, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  signed int srcdata = (sint8)(OPCODE & 0xFF);
  const int dstreg = (0x7a00 >> 9) & 7;
  uint32 outdata = srcdata;

  DATAREG(dstreg) = outdata;
}

void cpu_op_7a00b(void) /* MOVE */ {
  /* mask f100, bits 7000, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0
     set -2, size 3, stype 18, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  signed int srcdata = (sint8)(OPCODE & 0xFF);
  const int dstreg = (0x7a00 >> 9) & 7;
  uint32 outdata = srcdata;

  DATAREG(dstreg) = outdata;

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
}

void cpu_op_7c00a(void) /* MOVE */ {
  /* mask f100, bits 7000, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0
     set -2, size 3, stype 18, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  signed int srcdata = (sint8)(OPCODE & 0xFF);
  const int dstreg = (0x7c00 >> 9) & 7;
  uint32 outdata = srcdata;

  DATAREG(dstreg) = outdata;
}

void cpu_op_7c00b(void) /* MOVE */ {
  /* mask f100, bits 7000, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0
     set -2, size 3, stype 18, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  signed int srcdata = (sint8)(OPCODE & 0xFF);
  const int dstreg = (0x7c00 >> 9) & 7;
  uint32 outdata = srcdata;

  DATAREG(dstreg) = outdata;

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
}

void cpu_op_7e00a(void) /* MOVE */ {
  /* mask f100, bits 7000, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0
     set -2, size 3, stype 18, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  signed int srcdata = (sint8)(OPCODE & 0xFF);
  const int dstreg = (0x7e00 >> 9) & 7;
  uint32 outdata = srcdata;

  DATAREG(dstreg) = outdata;
}

void cpu_op_7e00b(void) /* MOVE */ {
  /* mask f100, bits 7000, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0
     set -2, size 3, stype 18, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  signed int srcdata = (sint8)(OPCODE & 0xFF);
  const int dstreg = (0x7e00 >> 9) & 7;
  uint32 outdata = srcdata;

  DATAREG(dstreg) = outdata;

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
}

