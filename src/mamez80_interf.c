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

#include "emu.h"
#include "memory.h"
#include "mamez80/z80.h"
#include "state.h"

static uint8_t *z80map1, *z80map2, *z80map3, *z80map4;

uint8_t mame_z80mem[0x10000];
static Z80_STATE z80_st;

#if 0
/* Memory and port IO handler */
void mame_z80_writemem16(uint16_t addr, uint8_t val)
{
    //  debug("Writemem %x=%x\n",addr,val);
    if (addr >= 0xf800)
        memory.z80_ram[addr - 0xf800] = val;
    
}

uint8_t mame_z80_readmem16(uint16_t addr)
{
    if (addr <= 0x7fff)
        return memory.rom.cpu_z80.p[addr];
    if (addr <= 0xbfff)
        return z80map1[addr - 0x8000];
    if (addr <= 0xdfff)
        return z80map2[addr - 0xc000];
    if (addr <= 0xefff)
        return z80map3[addr - 0xe000];
    if (addr <= 0xf7ff)
        return z80map4[addr - 0xf000];
    return memory.z80_ram[addr - 0xf800];
}


uint8_t mame_z80_readop(uint16_t addr)
{
    return mame_z80_readmem16(addr);
}

uint8_t mame_z80_readop_arg(uint16_t addr)
{
    return mame_z80_readmem16(addr);
}
#endif

void mame_z80_writeport16(uint16_t port, uint8_t value)
{
    //debug("Write port %d=%d\n",port,value);
    z80_port_write(port, value);
}

uint8_t mame_z80_readport16(uint16_t port)
{
    //debug("Read port %d\n",port);
    return z80_port_read(port);
}


/* cpu interface implementation */
void cpu_z80_switchbank(uint8_t bank, uint16_t PortNo)
{
    if (bank<=3)
        z80_bank[bank]=PortNo;
    
    switch (bank) {
        case 0:
            z80map1 = memory.rom.cpu_z80.p + (0x4000 * ((PortNo >> 8) & 0x0f));
            if ((0x4000 * ((PortNo >> 8) & 0x0f))<memory.rom.cpu_z80.size)
			    memcpy(mame_z80mem + 0x8000, z80map1, 0x4000);
            break;
        case 1:
            z80map2 = memory.rom.cpu_z80.p + (0x2000 * ((PortNo >> 8) & 0x1f));
            if ((0x2000 * ((PortNo >> 8) & 0x1f))<memory.rom.cpu_z80.size)
		        memcpy(mame_z80mem + 0xc000, z80map2, 0x2000);
            break;
        case 2:
            z80map3 = memory.rom.cpu_z80.p + (0x1000 * ((PortNo >> 8) & 0x3f));
            if ((0x1000 * ((PortNo >> 8) & 0x3f))<memory.rom.cpu_z80.size)
	            memcpy(mame_z80mem + 0xe000, z80map3, 0x1000);
            break;
        case 3:
            z80map4 = memory.rom.cpu_z80.p + (0x0800 * ((PortNo >> 8) & 0x7f));
            if ((0x0800 * ((PortNo >> 8) & 0x7f))<memory.rom.cpu_z80.size)
	            memcpy(mame_z80mem + 0xf000, z80map4, 0x0800);
            break;
    }
}

int mame_z80_irq_callback(int a)
{
    return 0;
}

//static void* mz80_context;

static void pre_save_state(void) {
    //z80_get_context(mz80_context);
    z80_st.PC=z80_get_reg(Z80_PC);
    z80_st.SP=z80_get_reg(Z80_SP);
    z80_st.AF=z80_get_reg(Z80_AF);
    z80_st.BC=z80_get_reg(Z80_BC);
    z80_st.DE=z80_get_reg(Z80_DE);
    z80_st.HL=z80_get_reg(Z80_HL);
    z80_st.IX=z80_get_reg(Z80_IX);
    z80_st.IY=z80_get_reg(Z80_IY);
    
    z80_st.AF2=z80_get_reg(Z80_AF2);
    z80_st.BC2=z80_get_reg(Z80_BC2);
    z80_st.DE2=z80_get_reg(Z80_DE2);
    z80_st.HL2=z80_get_reg(Z80_HL2);
    
    z80_st.IFF1=z80_get_reg(Z80_IFF1);
    z80_st.IFF2=z80_get_reg(Z80_IFF2);
    z80_st.IM=  z80_get_reg(Z80_IM);
    
    memcpy(memory.z80_ram,mame_z80mem+0xf800,0x800);
    
}

static void post_load_state(void) {
    int i;
    //z80_set_context(mz80_context);
    z80_set_reg(Z80_PC,z80_st.PC);
    z80_set_reg(Z80_SP,z80_st.SP);
    z80_set_reg(Z80_AF,z80_st.AF);
    z80_set_reg(Z80_BC,z80_st.BC);
    z80_set_reg(Z80_DE,z80_st.DE);
    z80_set_reg(Z80_HL,z80_st.HL);
    z80_set_reg(Z80_IX,z80_st.IX);
    z80_set_reg(Z80_IY,z80_st.IY);
    
    z80_set_reg(Z80_AF2,z80_st.AF2);
    z80_set_reg(Z80_BC2,z80_st.BC2);
    z80_set_reg(Z80_DE2,z80_st.DE2);
    z80_set_reg(Z80_HL2,z80_st.HL2);
    
    z80_set_reg(Z80_IFF1,z80_st.IFF1);
    z80_set_reg(Z80_IFF2,z80_st.IFF2);
    z80_set_reg(Z80_IM,z80_st.IM);
    
    for (i=0;i<4;i++) {
        cpu_z80_switchbank(i,z80_bank[i]);
    }
    memcpy(mame_z80mem+0xf800,memory.z80_ram,0x800);
}

static void z80_init_save_state(void) {
    /*
     int size=z80_get_context(NULL);
     mz80_context=(void*)malloc(size);
     */
    create_state_register(ST_Z80,"pc",1,(void *)&z80_st.PC,sizeof(uint16_t),REG_uint16_t);
    create_state_register(ST_Z80,"sp",1,(void *)&z80_st.SP,sizeof(uint16_t),REG_uint16_t);
    create_state_register(ST_Z80,"af",1,(void *)&z80_st.AF,sizeof(uint16_t),REG_uint16_t);
    create_state_register(ST_Z80,"bc",1,(void *)&z80_st.BC,sizeof(uint16_t),REG_uint16_t);
    create_state_register(ST_Z80,"de",1,(void *)&z80_st.DE,sizeof(uint16_t),REG_uint16_t);
    create_state_register(ST_Z80,"hl",1,(void *)&z80_st.HL,sizeof(uint16_t),REG_uint16_t);
    create_state_register(ST_Z80,"ix",1,(void *)&z80_st.IX,sizeof(uint16_t),REG_uint16_t);
    create_state_register(ST_Z80,"iy",1,(void *)&z80_st.IY,sizeof(uint16_t),REG_uint16_t);
    
    create_state_register(ST_Z80,"af2",1,(void *)&z80_st.AF2,sizeof(uint16_t),REG_uint16_t);
    create_state_register(ST_Z80,"bc2",1,(void *)&z80_st.BC2,sizeof(uint16_t),REG_uint16_t);
    create_state_register(ST_Z80,"de2",1,(void *)&z80_st.DE2,sizeof(uint16_t),REG_uint16_t);
    create_state_register(ST_Z80,"hl2",1,(void *)&z80_st.HL2,sizeof(uint16_t),REG_uint16_t);
    
    create_state_register(ST_Z80,"iff1",1,(void *)&z80_st.IFF1,sizeof(uint8_t),REG_uint8_t);
    create_state_register(ST_Z80,"iff2",1,(void *)&z80_st.IFF2,sizeof(uint8_t),REG_uint8_t);
    create_state_register(ST_Z80,"im",1,(void *)&z80_st.IM,sizeof(uint8_t),REG_uint8_t);
    
    
    create_state_register(ST_Z80,"bank",1,(void *)z80_bank,sizeof(uint16_t)*4,REG_uint16_t);
    create_state_register(ST_Z80,"z80_ram",1,(void *)memory.z80_ram,sizeof(uint8_t)*0x800,REG_uint8_t);
    
    set_post_load_function(ST_Z80,post_load_state);
    set_pre_save_function(ST_Z80,pre_save_state);
}

void cpu_z80_mkstate(gzFile *gzf,int mode) {
    mkstate_data(gzf, &z80_st, sizeof (z80_st), mode);
    mkstate_data(gzf, mame_z80mem, 0x10000, mode);
    if (mode==STREAD) {
        int i;
        for (i = 0; i < 4; i++) {
            cpu_z80_switchbank(i, z80_bank[i]);
        }
        //        memcpy(mame_z80mem + 0xf800, memory.z80_ram, 0x800);
    }
}

void cpu_z80_init(void)
{
    //  init_mamez80_mem();
    z80_init();
    
    /* bank initalisation */
    z80map1 = memory.rom.cpu_z80.p + 0x8000;
    z80map2 = memory.rom.cpu_z80.p + 0xc000;
    z80map3 = memory.rom.cpu_z80.p + 0xe000;
    z80map4 = memory.rom.cpu_z80.p + 0xf000;
    
    z80_bank[0]=0x8000;
    z80_bank[1]=0xc000;
    z80_bank[2]=0xe000;
    z80_bank[3]=0xf000;
    
    memcpy(mame_z80mem, memory.rom.cpu_z80.p, 0xf800);
    z80_reset(NULL);
    z80_set_irq_callback(mame_z80_irq_callback);
    z80_init_save_state();
}

void cpu_z80_run(int nbcycle)
{
    //debug("%x\n",z80_get_reg(Z80_PC));
    z80_execute(nbcycle);
}
void cpu_z80_nmi(void)
{
    //z80_set_irq_line(IRQ_LINE_NMI, 1/*PULSE_LINE- INTERNAL_CLEAR_LINE*/);
    z80_set_irq_line(IRQ_LINE_NMI, ASSERT_LINE);
    z80_set_irq_line(IRQ_LINE_NMI, CLEAR_LINE);
}
void cpu_z80_raise_irq(int l)
{
    z80_set_irq_line(l, ASSERT_LINE);
}
void cpu_z80_lower_irq(void)
{
    z80_set_irq_line(0, CLEAR_LINE);
}

uint16_t cpu_z80_get_pc(void)
{
    return 0;
}

