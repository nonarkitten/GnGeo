#ifndef _MEM68K_H_
#define _MEM68K_H_

typedef enum {
  mem_byte, mem_word, mem_long
} t_memtype;

typedef struct {
  uint16_t start;
  uint16_t end;
  uint8_t *(*memptr)(uint32_t addr);
  uint8_t (*fetch_byte)(uint32_t addr);
  uint16_t (*fetch_word)(uint32_t addr);
  uint32_t (*fetch_long)(uint32_t addr);
  void (*store_byte)(uint32_t addr, uint8_t data);
  void (*store_word)(uint32_t addr, uint16_t data);
  void (*store_long)(uint32_t addr, uint32_t data);
} t_mem68k_def;

typedef struct {
  uint32_t a;
  uint32_t b;
  uint32_t c;
  uint32_t up;
  uint32_t down;
  uint32_t left;
  uint32_t right;
  uint32_t start;
} t_keys;

extern t_mem68k_def mem68k_def[];
extern t_keys mem68k_cont[2];

int mem68k_init(void);

extern uint8_t *(*mem68k_memptr[0x1000])(uint32_t addr);
extern uint8_t (*mem68k_fetch_byte[0x1000])(uint32_t addr);
extern uint16_t (*mem68k_fetch_word[0x1000])(uint32_t addr);
extern uint32_t (*mem68k_fetch_long[0x1000])(uint32_t addr);
extern void (*mem68k_store_byte[0x1000])(uint32_t addr, uint8_t data);
extern void (*mem68k_store_word[0x1000])(uint32_t addr, uint16_t data);
extern void (*mem68k_store_long[0x1000])(uint32_t addr, uint32_t data);

#ifdef DIRECTRAMEN

static __inline__ uint8_t fetchbyte(uint32_t addr) {
    int adup=((addr) & 0xFFFFFF)>>12;

    if (adup >=0x100 && adup <=0x10F) { /* RAM */
	addr&=0xffff;
	return (*(uint8_t *) (memory.ram + addr));
    }
    if (adup >=0x200 && adup <=0x2ff) { /* banked cpu */
	addr&=0xfffff;
	return (*(uint8_t *) (memory.cpu + bankaddress + addr));
    }
    if (adup >=0x000 && adup <=0x0ff) { /* cpu bank 0 */
	addr&=0xfffff;
	return (*(uint8_t *) (memory.cpu + addr));
    }
    if (adup >=0xc00 && adup <=0xc1f) { /* bios */
	addr&=0x1ffff;
	return (*(uint8_t *) (memory.bios + addr));
    }
    return mem68k_fetch_byte[((addr) & 0xFFFFFF)>>12]((addr) & 0xFFFFFF);
}
static __inline__ uint16_t fetchword(uint32_t addr) {
    int adup=((addr) & 0xFFFFFF)>>12;

    if (adup >=0x100 && adup <=0x10F) { /* RAM */
	addr&=0xffff;
	return LOCENDIAN16(*(uint16_t *) (memory.ram + addr));
    }
    if (adup >=0x200 && adup <=0x2ff) { /* banked cpu */
	addr&=0xfffff;
	return LOCENDIAN16(*(uint16_t *) (memory.cpu + bankaddress + addr));
    }
    if (adup >=0x000 && adup <=0x0ff) { /* cpu bank 0 */
	addr&=0xfffff;
	return LOCENDIAN16(*(uint16_t *) (memory.cpu + addr));
    }
    if (adup >=0xc00 && adup <=0xc1f) { /* bios */
	addr&=0x1ffff;
	return LOCENDIAN16(*(uint16_t *) (memory.bios + addr));
    }
    return mem68k_fetch_word[((addr) & 0xFFFFFF)>>12]((addr) & 0xFFFFFF);
}
static __inline__ uint32_t fetchlong(uint32_t addr) {
    int adup=((addr) & 0xFFFFFF)>>12;
#ifdef ALIGNLONGS
    if (adup >=0x100 && adup <=0x10F) { /* RAM */
	addr&=0xffff;
	return (LOCENDIAN16(*(uint16_t *) (memory.ram + addr))<< 16) |
	    LOCENDIAN16(*(uint16_t *) (memory.ram + addr + 2));
    }
    if (adup >=0x200 && adup <=0x2ff) { /* banked cpu */
	addr&=0xfffff;
	return (LOCENDIAN16(*(uint16_t *) (memory.cpu + bankaddress + addr))<< 16) |
	    LOCENDIAN16(*(uint16_t *) (memory.cpu + bankaddress + addr + 2 ));
    }
    if (adup >=0x000 && adup <=0x0ff) { /* cpu bank 0 */
	addr&=0xfffff;
	return (LOCENDIAN16(*(uint16_t *) (memory.cpu + addr))<< 16) |
	    LOCENDIAN16(*(uint16_t *) (memory.cpu + addr + 2));
    }
    if (adup >=0xc00 && adup <=0xc1f) { /* bios */
	addr&=0x1ffff;
	return (LOCENDIAN16(*(uint16_t *) (memory.bios + addr))<< 16) |
	    LOCENDIAN16(*(uint16_t *) (memory.bios + addr + 2));
    }
#else
    if (adup >=0x100 && adup <=0x10F) { /* RAM */
	addr&=0xffff;
	return LOCENDIAN32(*(uint32_t *) (memory.ram + addr));
    }
    if (adup >=0x200 && adup <=0x2ff) { /* banked cpu */
	addr&=0xfffff;
	return LOCENDIAN32(*(uint32_t *) (memory.cpu + bankaddress + addr));
    }
    if (adup >=0x000 && adup <=0x0ff) { /* cpu bank 0 */
	addr&=0xfffff;
	return LOCENDIAN32(*(uint32_t *) (memory.cpu + addr));
    }
    if (adup >=0xc00 && adup <=0xc1f) { /* bios */
	addr&=0x1ffff;
	return LOCENDIAN32(*(uint32_t *) (memory.bios + addr));
    }
#endif
    return mem68k_fetch_long[((addr) & 0xFFFFFF)>>12]((addr) & 0xFFFFFF);
}
#else

#define fetchbyte(addr) mem68k_fetch_byte[((addr) & 0xFFFFFF)>>12]((addr) & 0xFFFFFF)
#define fetchword(addr) mem68k_fetch_word[((addr) & 0xFFFFFF)>>12]((addr) & 0xFFFFFF)
#define fetchlong(addr) mem68k_fetch_long[((addr) & 0xFFFFFF)>>12]((addr) & 0xFFFFFF)

#endif

/* XXX BUG: these direct routines do not check for over-run of the 64k
   cpu68k_ram block - so writing a long at $FFFF corrupts 3 bytes of data -
   this is compensated for in the malloc() but is bad nonetheless. */

#ifdef DIRECTRAM

/* chances are a store is to RAM - optimise for this case */

static __inline__ void storebyte(uint32_t addr, uint8_t data)
{
  if ((addr>>16) == 0x10) {
    addr&= 0xffff;
    *(uint8_t *)(cpu68k_ram + addr) = data;
  } else {
    mem68k_store_byte[((addr) & 0xFFFFFF)>>12]((addr) & 0xFFFFFF,data);
  }
}

static __inline__ void storeword(uint32_t addr, uint16_t data)
{
  /* in an ideal world we'd check bit 0 of addr, but speed is everything */
  if ((addr >>16) == 0x10) {
    addr&= 0xffff;
    *(uint16_t *)(cpu68k_ram + addr) = LOCENDIAN16(data);
  } else {
    mem68k_store_word[((addr) & 0xFFFFFF)>>12]((addr) & 0xFFFFFF,data);
  }
}

static __inline__ void storelong(uint32_t addr, uint32_t data)
{
  /* in an ideal world we'd check bit 0 of addr, but speed is everything */
  if ((addr >>16) == 0x10) {
    addr&= 0xffff;
#ifdef ALIGNLONGS
    *(uint16_t *)(cpu68k_ram + addr) = LOCENDIAN16((uint16_t)(data >> 16));
    *(uint16_t *)(cpu68k_ram + addr + 2) = LOCENDIAN16((uint16_t)(data));
#else
    *(uint32_t *)(cpu68k_ram + addr) = LOCENDIAN32(data);
#endif
  } else {
    mem68k_store_long[((addr) & 0xFFFFFF)>>12]((addr) & 0xFFFFFF,data);
  }
}

#else

#define storebyte(addr,data) mem68k_store_byte[((addr) & 0xFFFFFF)>>12]((addr) & 0xFFFFFF,data)
#define storeword(addr,data) mem68k_store_word[((addr) & 0xFFFFFF)>>12]((addr) & 0xFFFFFF,data)
#define storelong(addr,data) mem68k_store_long[((addr) & 0xFFFFFF)>>12]((addr) & 0xFFFFFF,data)

#endif

#endif
