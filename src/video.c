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

#include "amiga.h"

#include <config.h>
#include <string.h>
#include <stdlib.h>
#include "video.h"
#include "memory.h"
#include "emu.h"
#include "messages.h"
#include "conf.h"
#include "screen.h"

#define PEN_USAGE(tileno) ((((uint32_t*) memory.rom.spr_usage.p)[tileno>>4]>>((tileno&0xF)*2))&0x3)

extern int neogeo_fix_bank_type;
const uint16_t ddaxskip_i[17] = {
	0x0000, 0x0080, 0x0880, 0x0888, 0x2888, 0x288a, 0x2a8a, 0x2aaa, 0xaaaa,
	0xaaea, 0xbaea, 0xbaeb, 0xbbeb, 0xbbef, 0xfbef, 0xfbff, 0xffff
};
const int overscan = 1;

extern struct RastPort *theRastPort;

uint8_t line_limit[256] = {0xFF}, *limit;
uint32_t neogeo_frame_counter_speed = 8;
uint32_t even_odd = 0;

static uint8_t dr, dg, db, sr, sg, sb;

void draw_screen(void) { 
 	if(screen_prerender()) { 
		clr_screen_m68k(bufferpixels, current_pc_pal[4095] );

		/* Draw sprites */
		if(AC68080) 
			draw_tiles_ammx();
		else 
			draw_tiles_m68k();
		
		/* Draw fix layer */
		draw_fix_char(bufferpixels, 0, 0); 
		
		clr_border_m68k(bufferpixels, 0 );
		screen_update();		
	} else {
		debug("Unable to lock screen (%p)\n", bufferpixels);
		exit(-1);
	}
}

void init_video(void) {
	memory.vid.modulo = 1;
}
