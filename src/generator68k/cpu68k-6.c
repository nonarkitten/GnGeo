/*****************************************************************************/
/*     Generator - Sega Genesis emulation - (c) James Ponder 1997-2001       */
/*****************************************************************************/
/*                                                                           */
/* cpu68k-6.c                                                                */
/*                                                                           */
/*****************************************************************************/

#include "cpu68k-inline.h"

#include "longtab68k.h"

void cpu_op_6000a(void) /* Bcc */ {
  /* mask ffff, bits 6000, mnemonic 62, priv 0, endblk -1, imm_notzero 0, used -1
     set 0, size 2, stype 12, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  const uint8 cc = 1;

  if (cc)
    PC = srcdata;
  else
    PC+= 4;
}

void cpu_op_6001a(void) /* Bcc */ {
  /* mask ffff, bits 6000, mnemonic 62, priv 0, endblk -1, imm_notzero 0, used -1
     set 0, size 2, stype 12, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  const uint8 cc = 1;

  if (cc)
    PC = srcdata;
  else
    PC+= 4;
}

void cpu_op_6100a(void) /* BSR */ {
  /* mask ffff, bits 6100, mnemonic 63, priv 0, endblk -1, imm_notzero 0, used -1
     set 0, size 2, stype 12, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;

  ADDRREG(7)-= 4;
  storelong(ADDRREG(7), PC+4);
  PC = srcdata;
}

void cpu_op_6101a(void) /* BSR */ {
  /* mask ffff, bits 6100, mnemonic 63, priv 0, endblk -1, imm_notzero 0, used -1
     set 0, size 2, stype 12, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;

  ADDRREG(7)-= 4;
  storelong(ADDRREG(7), PC+4);
  PC = srcdata;
}

void cpu_op_6200a(void) /* Bcc */ {
  /* mask ffff, bits 6200, mnemonic 62, priv 0, endblk -1, imm_notzero 0, used -1
     set 0, size 2, stype 12, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  uint8 cc = !(CFLAG || ZFLAG);

  if (cc)
    PC = srcdata;
  else
    PC+= 4;
}

void cpu_op_6201a(void) /* Bcc */ {
  /* mask ffff, bits 6200, mnemonic 62, priv 0, endblk -1, imm_notzero 0, used -1
     set 0, size 2, stype 12, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  uint8 cc = !(CFLAG || ZFLAG);

  if (cc)
    PC = srcdata;
  else
    PC+= 4;
}

void cpu_op_6300a(void) /* Bcc */ {
  /* mask ffff, bits 6300, mnemonic 62, priv 0, endblk -1, imm_notzero 0, used -1
     set 0, size 2, stype 12, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  uint8 cc = CFLAG || ZFLAG;

  if (cc)
    PC = srcdata;
  else
    PC+= 4;
}

void cpu_op_6301a(void) /* Bcc */ {
  /* mask ffff, bits 6300, mnemonic 62, priv 0, endblk -1, imm_notzero 0, used -1
     set 0, size 2, stype 12, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  uint8 cc = CFLAG || ZFLAG;

  if (cc)
    PC = srcdata;
  else
    PC+= 4;
}

void cpu_op_6400a(void) /* Bcc */ {
  /* mask ffff, bits 6400, mnemonic 62, priv 0, endblk -1, imm_notzero 0, used -1
     set 0, size 2, stype 12, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  uint8 cc = !CFLAG;

  if (cc)
    PC = srcdata;
  else
    PC+= 4;
}

void cpu_op_6401a(void) /* Bcc */ {
  /* mask ffff, bits 6400, mnemonic 62, priv 0, endblk -1, imm_notzero 0, used -1
     set 0, size 2, stype 12, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  uint8 cc = !CFLAG;

  if (cc)
    PC = srcdata;
  else
    PC+= 4;
}

void cpu_op_6500a(void) /* Bcc */ {
  /* mask ffff, bits 6500, mnemonic 62, priv 0, endblk -1, imm_notzero 0, used -1
     set 0, size 2, stype 12, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  uint8 cc = CFLAG;

  if (cc)
    PC = srcdata;
  else
    PC+= 4;
}

void cpu_op_6501a(void) /* Bcc */ {
  /* mask ffff, bits 6500, mnemonic 62, priv 0, endblk -1, imm_notzero 0, used -1
     set 0, size 2, stype 12, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  uint8 cc = CFLAG;

  if (cc)
    PC = srcdata;
  else
    PC+= 4;
}

void cpu_op_6600a(void) /* Bcc */ {
  /* mask ffff, bits 6600, mnemonic 62, priv 0, endblk -1, imm_notzero 0, used -1
     set 0, size 2, stype 12, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  uint8 cc = !ZFLAG;

  if (cc)
    PC = srcdata;
  else
    PC+= 4;
}

void cpu_op_6601a(void) /* Bcc */ {
  /* mask ffff, bits 6600, mnemonic 62, priv 0, endblk -1, imm_notzero 0, used -1
     set 0, size 2, stype 12, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  uint8 cc = !ZFLAG;

  if (cc)
    PC = srcdata;
  else
    PC+= 4;
}

void cpu_op_6700a(void) /* Bcc */ {
  /* mask ffff, bits 6700, mnemonic 62, priv 0, endblk -1, imm_notzero 0, used -1
     set 0, size 2, stype 12, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  uint8 cc = ZFLAG;

  if (cc)
    PC = srcdata;
  else
    PC+= 4;
}

void cpu_op_6701a(void) /* Bcc */ {
  /* mask ffff, bits 6700, mnemonic 62, priv 0, endblk -1, imm_notzero 0, used -1
     set 0, size 2, stype 12, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  uint8 cc = ZFLAG;

  if (cc)
    PC = srcdata;
  else
    PC+= 4;
}

void cpu_op_6800a(void) /* Bcc */ {
  /* mask ffff, bits 6800, mnemonic 62, priv 0, endblk -1, imm_notzero 0, used -1
     set 0, size 2, stype 12, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  uint8 cc = !VFLAG;

  if (cc)
    PC = srcdata;
  else
    PC+= 4;
}

void cpu_op_6801a(void) /* Bcc */ {
  /* mask ffff, bits 6800, mnemonic 62, priv 0, endblk -1, imm_notzero 0, used -1
     set 0, size 2, stype 12, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  uint8 cc = !VFLAG;

  if (cc)
    PC = srcdata;
  else
    PC+= 4;
}

void cpu_op_6900a(void) /* Bcc */ {
  /* mask ffff, bits 6900, mnemonic 62, priv 0, endblk -1, imm_notzero 0, used -1
     set 0, size 2, stype 12, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  uint8 cc = VFLAG;

  if (cc)
    PC = srcdata;
  else
    PC+= 4;
}

void cpu_op_6901a(void) /* Bcc */ {
  /* mask ffff, bits 6900, mnemonic 62, priv 0, endblk -1, imm_notzero 0, used -1
     set 0, size 2, stype 12, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  uint8 cc = VFLAG;

  if (cc)
    PC = srcdata;
  else
    PC+= 4;
}

void cpu_op_6a00a(void) /* Bcc */ {
  /* mask ffff, bits 6a00, mnemonic 62, priv 0, endblk -1, imm_notzero 0, used -1
     set 0, size 2, stype 12, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  uint8 cc = !NFLAG;

  if (cc)
    PC = srcdata;
  else
    PC+= 4;
}

void cpu_op_6a01a(void) /* Bcc */ {
  /* mask ffff, bits 6a00, mnemonic 62, priv 0, endblk -1, imm_notzero 0, used -1
     set 0, size 2, stype 12, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  uint8 cc = !NFLAG;

  if (cc)
    PC = srcdata;
  else
    PC+= 4;
}

void cpu_op_6b00a(void) /* Bcc */ {
  /* mask ffff, bits 6b00, mnemonic 62, priv 0, endblk -1, imm_notzero 0, used -1
     set 0, size 2, stype 12, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  uint8 cc = NFLAG;

  if (cc)
    PC = srcdata;
  else
    PC+= 4;
}

void cpu_op_6b01a(void) /* Bcc */ {
  /* mask ffff, bits 6b00, mnemonic 62, priv 0, endblk -1, imm_notzero 0, used -1
     set 0, size 2, stype 12, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  uint8 cc = NFLAG;

  if (cc)
    PC = srcdata;
  else
    PC+= 4;
}

void cpu_op_6c00a(void) /* Bcc */ {
  /* mask ffff, bits 6c00, mnemonic 62, priv 0, endblk -1, imm_notzero 0, used -1
     set 0, size 2, stype 12, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  uint8 cc = (NFLAG == VFLAG);

  if (cc)
    PC = srcdata;
  else
    PC+= 4;
}

void cpu_op_6c01a(void) /* Bcc */ {
  /* mask ffff, bits 6c00, mnemonic 62, priv 0, endblk -1, imm_notzero 0, used -1
     set 0, size 2, stype 12, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  uint8 cc = (NFLAG == VFLAG);

  if (cc)
    PC = srcdata;
  else
    PC+= 4;
}

void cpu_op_6d00a(void) /* Bcc */ {
  /* mask ffff, bits 6d00, mnemonic 62, priv 0, endblk -1, imm_notzero 0, used -1
     set 0, size 2, stype 12, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  uint8 cc = (NFLAG != VFLAG);

  if (cc)
    PC = srcdata;
  else
    PC+= 4;
}

void cpu_op_6d01a(void) /* Bcc */ {
  /* mask ffff, bits 6d00, mnemonic 62, priv 0, endblk -1, imm_notzero 0, used -1
     set 0, size 2, stype 12, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  uint8 cc = (NFLAG != VFLAG);

  if (cc)
    PC = srcdata;
  else
    PC+= 4;
}

void cpu_op_6e00a(void) /* Bcc */ {
  /* mask ffff, bits 6e00, mnemonic 62, priv 0, endblk -1, imm_notzero 0, used -1
     set 0, size 2, stype 12, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  uint8 cc = !ZFLAG && (NFLAG == VFLAG);

  if (cc)
    PC = srcdata;
  else
    PC+= 4;
}

void cpu_op_6e01a(void) /* Bcc */ {
  /* mask ffff, bits 6e00, mnemonic 62, priv 0, endblk -1, imm_notzero 0, used -1
     set 0, size 2, stype 12, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  uint8 cc = !ZFLAG && (NFLAG == VFLAG);

  if (cc)
    PC = srcdata;
  else
    PC+= 4;
}

void cpu_op_6f00a(void) /* Bcc */ {
  /* mask ffff, bits 6f00, mnemonic 62, priv 0, endblk -1, imm_notzero 0, used -1
     set 0, size 2, stype 12, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  uint8 cc = ZFLAG || (NFLAG != VFLAG);

  if (cc)
    PC = srcdata;
  else
    PC+= 4;
}

void cpu_op_6f01a(void) /* Bcc */ {
  /* mask ffff, bits 6f00, mnemonic 62, priv 0, endblk -1, imm_notzero 0, used -1
     set 0, size 2, stype 12, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  uint8 cc = ZFLAG || (NFLAG != VFLAG);

  if (cc)
    PC = srcdata;
  else
    PC+= 4;
}

