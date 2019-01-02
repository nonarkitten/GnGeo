/*  gngeo a neogeo emulator
 *  Copyright (C) 2001 Peponas Mathieu
 * 
 *  This program is free software; you can redistribute it and/or modify  
 *  it under the terms of the GNU General Public License as published by   
 *  the Free Software Foundation; either version 2 of the License, or    
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA. 
 */

/* cyclone interface */



#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef USE_CYCLONE

#include <stdlib.h>

#include "cyclone/Cyclone.h"
#include "memory.h"
#include "emu.h"
#include "state.h"
#include "debug.h"
#include "video.h"
#include "conf.h"

#define MEMHANDLER_READ(start,end,func) {if (a>=start && a<=end) return func(a);} 
#define MEMHANDLER_WRITE(start,end,func) {if (a>=start && a<=end) {func(a,d);return;}}

unsigned int m68k_read_memory_8  (unsigned int a) {
	unsigned int addr=a&0xFFFFF;
	unsigned int b=((a&0xF0000)>>16);
	a&=0xFFFFFF;
	switch((a&0xFF0000)>>20) {
	case 0x0:
		return (READ_BYTE_ROM(memory.rom.cpu_m68k.p + addr))&0xFF;
		break;
	case 0x2:
		if (memory.bksw_unscramble)
			return mem68k_fetch_bk_normal_byte(a);
		return (READ_BYTE_ROM(memory.rom.cpu_m68k.p + bankaddress + addr))&0xFF;
		break;
	case 0x1:
		return (READ_BYTE_ROM(memory.ram + (addr&0xFFFF)))&0xFF;
		break;
	case 0xC:
		if (b<=1) return (READ_BYTE_ROM(memory.rom.bios_m68k.p + addr))&0xFF;
		break;
	case 0xd:
		if (b==0) return mem68k_fetch_sram_byte(a)&0xFF;
		break;
	case 0x4:
		if (b==0) return mem68k_fetch_pal_byte(a)&0xFF;
		break;
	case 0x3:
		if (b==0xC) return mem68k_fetch_video_byte(a)&0xFF;
		if (b==0) return mem68k_fetch_ctl1_byte(a)&0xFF;
		if (b==4) return mem68k_fetch_ctl2_byte(a)&0xFF;
		if (b==8) return mem68k_fetch_ctl3_byte(a)&0xFF;
		if (b==2) return mem68k_fetch_coin_byte(a)&0xFF;
		break;
	case 0x8:
		if (b==0) return mem68k_fetch_memcrd_byte(a)&0xFF;
		break;
	}

	return 0xFF;
}
unsigned int m68k_read_memory_16 (unsigned int a) {
	unsigned int addr=a&0xFFFFF;
	unsigned int b=((a&0xF0000)>>16);
	//printf("read 32 %08x\n",a);
	a&=0xFFFFFF;

	switch((a&0xFF0000)>>20) {
	case 0x0:
		return (READ_WORD_ROM(memory.rom.cpu_m68k.p + addr))&0xFFFF;
		break;
	case 0x2:
		if (memory.bksw_unscramble)
			return mem68k_fetch_bk_normal_word(a);
		return (READ_WORD_ROM(memory.rom.cpu_m68k.p + bankaddress + addr))&0xFFFF;

		break;
	case 0x1:
		return (READ_WORD_ROM(memory.ram + (addr&0xFFFF)))&0xFFFF;
		break;
	case 0xC:
		if (b<=1) return (READ_WORD_ROM(memory.rom.bios_m68k.p + addr))&0xFFFF;
		break;

	case 0xd:
		if (b==0) return mem68k_fetch_sram_word(a)&0xFFFF;
		break;
	case 0x4:
		if (b==0) return mem68k_fetch_pal_word(a)&0xFFFF;
		break;
	case 0x3:
		if (b==0xC) return mem68k_fetch_video_word(a)&0xFFFF;
		if (b==0) return mem68k_fetch_ctl1_word(a)&0xFFFF;
		if (b==4) return mem68k_fetch_ctl2_word(a)&0xFFFF;
		if (b==8) return mem68k_fetch_ctl3_word(a)&0xFFFF;
		if (b==2) return mem68k_fetch_coin_word(a)&0xFFFF;
		break;
	case 0x8:
		if (b==0) return mem68k_fetch_memcrd_word(a)&0xFFFF;
		break;
	}

	return 0xF0F0;
}
unsigned int m68k_read_memory_32 (unsigned int a) {
	//int i;
	unsigned int addr=a&0xFFFFF;
	unsigned int b=((a&0xF0000)>>16);
	a&=0xFFFFFF;

	switch((a&0xFF0000)>>20) {
	case 0x0:
		//return mem68k_fetch_cpu_long(a);
		return ((READ_WORD_ROM(memory.rom.cpu_m68k.p + addr))<<16) | 
			(READ_WORD_ROM(memory.rom.cpu_m68k.p + (addr+2)));
		break;
	case 0x2:
		//return mem68k_fetch_bk_normal_long(a);
		if (memory.bksw_unscramble)
			return mem68k_fetch_bk_normal_long(a);
		return ((READ_WORD_ROM(memory.rom.cpu_m68k.p + bankaddress + addr))<<16) | 
			(READ_WORD_ROM(memory.rom.cpu_m68k.p + bankaddress + (addr+2)));
		break;
	case 0x1:
		//return mem68k_fetch_ram_long(a);
		addr&=0xFFFF;
		return ((READ_WORD_ROM(memory.ram + addr))<<16) | 
			(READ_WORD_ROM(memory.ram + (addr+2)));
		break;
	case 0xC:
		//return mem68k_fetch_bios_long(a);
		if (b<=1) return ((READ_WORD_ROM(memory.rom.bios_m68k.p + addr))<<16) | 
				 (READ_WORD_ROM(memory.rom.bios_m68k.p + (addr+2)));
		break;

	case 0xd:
		if (b==0) return mem68k_fetch_sram_long(a);
		break;
	case 0x4:
		if (b==0) return mem68k_fetch_pal_long(a);
		break;
	case 0x3:
		if (b==0xC) return mem68k_fetch_video_long(a);
		if (b==0) return mem68k_fetch_ctl1_long(a);
		if (b==4) return mem68k_fetch_ctl2_long(a);
		if (b==8) return mem68k_fetch_ctl3_long(a);
		if (b==2) return mem68k_fetch_coin_long(a);
		break;
	case 0x8:
		if (b==0) return mem68k_fetch_memcrd_long(a);
		break;
	}

	return 0xFF00FF00;
}

void m68k_write_memory_8 (unsigned int a,unsigned int d) {
	unsigned int b=((a&0xF0000)>>16);
	a&=0xFFFFFF;
    d&=0xFF;
	switch((a&0xFF0000)>>20) {
	case 0x1:
		WRITE_BYTE_ROM(memory.ram + (a&0xffff),d);
		return ;
		break;
	case 0x3:
		if (b==0xc) {mem68k_store_video_byte(a,d);return;}
		if (b==8) {mem68k_store_pd4990_byte(a,d);return;}
		if (b==2) {mem68k_store_z80_byte(a,d);return;}
		if (b==0xA) {mem68k_store_setting_byte(a,d);return;}
		break;
	case 0x4:
		if (b==0) mem68k_store_pal_byte(a,d);
		return;
		break;
	case 0xD:
		if (b==0) mem68k_store_sram_byte(a,d);return;
		break;
	case 0x2:
		if (b==0xF) mem68k_store_bk_normal_byte(a,d);return;
		break;
	case 0x8:
		if (b==0) mem68k_store_memcrd_byte(a,d);return;
		break;

	}

	if(a==0x300001) memory.watchdog=0; // Watchdog

	//printf("Unhandled write8  @ %08x = %02x\n",a,d);
}
void m68k_write_memory_16(unsigned int a,unsigned int d) {
	unsigned int b=((a&0xF0000)>>16);
	a&=0xFFFFFF;
    d&=0xFFFF;
    //if (d&0x8000) printf("WEIRD %x %x\n",a,d);

	switch((a&0xFF0000)>>20) {
	case 0x1:
		WRITE_WORD_ROM(memory.ram + (a&0xffff),d);
		return;
		//mem68k_store_ram_word(a,d);return;
		break;
	case 0x3:
		if (b==0xc) {
            mem68k_store_video_word(a,d);return;}
		if (b==8) {mem68k_store_pd4990_word(a,d);return;}
		if (b==2) {mem68k_store_z80_word(a,d);return;}
		if (b==0xA) {mem68k_store_setting_word(a,d);return;}
		break;	
	case 0x4:
		if (b==0) mem68k_store_pal_word(a,d);return;
		break;
	case 0xD:
		if (b==0) mem68k_store_sram_word(a,d);return;
		break;
	case 0x2:
		if (b==0xF) mem68k_store_bk_normal_word(a,d);return;
		break;
	case 0x8:
		if (b==0) mem68k_store_memcrd_word(a,d);return;
		break;
	}

	//printf("Unhandled write16 @ %08x = %04x\n",a,d);
}
void m68k_write_memory_32(unsigned int a,unsigned int d) {
	unsigned int b=((a&0xF0000)>>16);
	a&=0xFFFFFF;
    d&=0xFFFFFFFF;
	
	switch((a&0xFF0000)>>20) {
	case 0x1:
		WRITE_WORD_ROM(memory.ram + (a&0xffff),d>>16);
		WRITE_WORD_ROM(memory.ram + (a&0xffff)+2,d&0xFFFF);
		return;
		break;
	case 0x3:
		if (b==0xc) {
			mem68k_store_video_word(a,d>>16);
			mem68k_store_video_word(a+2,d & 0xffff);
			return;
		}
		if (b==0x8) {mem68k_store_pd4990_long(a,d);return;}
		if (b==0x2) {mem68k_store_z80_long(a,d);return;}
		if (b==0xA) {mem68k_store_setting_long(a,d);return;}
		break;	
	case 0x4:
		if (b==0) mem68k_store_pal_long(a,d);return;
		break;
	case 0xD:
		if (b==0) mem68k_store_sram_long(a,d);return;
		break;
	case 0x2:
		if (b==0xF) mem68k_store_bk_normal_long(a,d);return;
		break;
	case 0x8:
		if (b==0) mem68k_store_memcrd_long(a,d);return;
		break;
	}

	//printf("Unhandled write32 @ %08x = %08x\n",a,d);
}

// EXTERNS
void cpu_68k_mkstate(gzFile *gzf,int mode) {
	/* Get the size of the cpu context in bytes */
	int size = m68k_context_size();
	void *context;
	
	m68k_get_context(&context);
	mkstate_data(gzf, context, size, mode);
}

int cpu_68k_getcycle(void) {
	return m68k_cycles_remaining();
}

void bankswitcher_init() {
	bankaddress=0;
}

// int cyclone_debug(unsigned short o) {
// 	printf("CYCLONE DEBUG %04x\n",o);
// 	return 0;
// }

void cpu_68k_init(void) {
	m68k_set_cpu_type(M68K_CPU_TYPE_68000);
	m68k_init();
	m68k_pulse_reset();

	bankswitcher_init();
	if (memory.rom.cpu_m68k.size > 0x100000)
		bankaddress = 0x100000;

// 	time_slice=(overclk==0?
// 		    200000:200000+(overclk*200000/100.0))/262.0;
}

void cpu_68k_reset(void) {
	m68k_pulse_reset();
}

int cpu_68k_run(Uint32 nb_cycle) {
	return -m68k_execute(nb_cycle);
}

void cpu_68k_interrupt(int a) {
	m68k_set_irq(a);
}

void cpu_68k_bankswitch(Uint32 address) {
	bankaddress = address;
}

void cpu_68k_disassemble(int pc, int nb_instr) {
	/* TODO */
}

void cpu_68k_dumpreg(void) {
	/* TODO */
}

int cpu_68k_run_step(void) {
	return -m68k_execute(0);
}

Uint32 cpu_68k_getpc(void) {
	return (Uint32)m68k_get_reg(NULL, M68K_REG_PC);
}

void cpu_68k_fill_state(M68K_STATE *st) {
}

void cpu_68k_set_state(M68K_STATE *st) {
}

int cpu_68k_debuger(void (*execstep)(void),void (*dump)(void)) {
	/* TODO */
	return 0;
}



#endif