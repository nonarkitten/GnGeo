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

#ifndef _VIDEO_H_
#define _VIDEO_H_

#include <stdio.h>
#include <config.h> 
#include "emu.h"

typedef struct gfx_cache {
	uint8_t *data;  /* The cache */
	uint32_t size;  /* Tha allocated size of the cache */      
	uint32_t total_bank;  /* total number of rom bank */
	uint8_t **ptr/*[TOTAL_GFX_BANK]*/; /* ptr[i] Contain a pointer to cached data for bank i */
	int max_slot; /* Maximal numer of bank that can be cached (depend on cache size) */
	int slot_size;
	int *usage;   /* contain index to the banks in used order */
	FILE *gno;
    uint32_t *offset;
    uint8_t* in_buf;
}GFX_CACHE;

typedef struct VIDEO {
	/* Video Ram&Pal */
	uint8_t ram[0x20000];
	uint8_t pal_neo[2][0x2000];
	uint8_t pal_host[2][0x4000];
	uint8_t currentpal;
        uint8_t currentfix; /* 0=bios fix */
	uint16_t rbuf;

	/* Auto anim counter */
	uint32_t fc;
	uint32_t fc_speed;

	uint32_t vptr;
	int16_t modulo;

	uint32_t current_line;

	/* IRQ2 related */
	uint32_t irq2control;
	uint32_t irq2taken;
	uint32_t irq2start;
	uint32_t irq2pos;

    GFX_CACHE spr_cache;
}VIDEO;

#define RASTER_LINES 261

uint32_t neogeo_frame_counter;
extern uint32_t neogeo_frame_counter_speed;

void init_video(void);

extern void draw_tiles_m68k(void);
extern void draw_tiles_ammx(void);
extern void draw_fix_char(uint8_t *buf, int start, int end);

void draw_screen(void);

#endif
