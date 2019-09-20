/* the ordering of these includes is important - stdio can have inline
   functions - so can mem68k.h - and registers.h must appear before them */

#include "generator.h"
#include "registers.h"

#include <stdio.h>

#include "cpu68k.h"
#include "mem68k.h"
#include "reg68k.h"

#define DATAREG(a) (regs.regs[a])
#define ADDRREG(a) (regs.regs[8+(a)])
#define PC (regs.pc)
#define SR (regs.sr.sr_int)
#define SP (regs.sp)
#define STOP (regs.stop)
#define TFLAG (regs.sr.sr_struct.t)
#define SFLAG (regs.sr.sr_struct.s)
#define XFLAG (regs.sr.sr_struct.x)
#define NFLAG (regs.sr.sr_struct.n)
#define ZFLAG (regs.sr.sr_struct.z)
#define VFLAG (regs.sr.sr_struct.v)
#define CFLAG (regs.sr.sr_struct.c)

static __inline__ int32_t idxval_dst(t_ipc *ipc) {
  switch( ((ipc->dst>>27) & 1) | ((ipc->dst>>30) & 2) ) {
  case 0: /* data, word */
    return ((int16_t)DATAREG((ipc->dst>>28)&7))+((((int32_t)(ipc->dst<<8)))>>8);
  case 1: /* data, long */
    return ((int32_t)DATAREG((ipc->dst>>28)&7))+((((int32_t)(ipc->dst<<8)))>>8);
  case 2: /* addr, word */
    return ((int16_t)ADDRREG((ipc->dst>>28)&7))+((((int32_t)(ipc->dst<<8)))>>8);
  case 3: /* addr, long */
    return ((int32_t)ADDRREG((ipc->dst>>28)&7))+((((int32_t)(ipc->dst<<8)))>>8);
  }
  return 0;
}

static __inline__ int32_t idxval_src(t_ipc *ipc) {
  switch( ((ipc->src>>27) & 1) | ((ipc->src>>30) & 2) ) {
  case 0: /* data, word */
    return ((int16_t)DATAREG((ipc->src>>28)&7))+((((int32_t)(ipc->src<<8)))>>8);
  case 1: /* data, long */
    return ((int32_t)DATAREG((ipc->src>>28)&7))+((((int32_t)(ipc->src<<8)))>>8);
  case 2: /* addr, word */
    return ((int16_t)ADDRREG((ipc->src>>28)&7))+((((int32_t)(ipc->src<<8)))>>8);
  case 3: /* addr, long */
    return ((int32_t)ADDRREG((ipc->src>>28)&7))+((((int32_t)(ipc->src<<8)))>>8);
  }
  return 0;
}
