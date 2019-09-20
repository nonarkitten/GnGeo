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

#include <config.h>
#include <string.h>
#include <stdlib.h>
//#include <zlib.h>
#include "video.h"
#include "memory.h"
#include "emu.h"
#include "messages.h"
#include "conf.h"
#include "screen.h"
// #include "frame_skip.h"
// #include "transpack.h"
#include <cybergraphx/cybergraphics.h>
#include <proto/cybergraphics.h>
extern int neogeo_fix_bank_type;


// #ifdef PROCESSOR_ARM
// /* global declaration for video_arm.S */
// uint8_t *mem_gfx = NULL; /*=memory.rom.tiles.p;*/
// uint8_t *mem_video = NULL; //memory.vid.ram;
// //#define TOTAL_GFX_BANK 4096
// uint32_t *mem_bank_usage;
// 
// //GFX_CACHE gcache;
// 
// void draw_one_char_arm(int byte1, int byte2, uint16_t *br);
// int draw_tile_arm_norm(uint32_t tileno, int color, uint8_t *bmp, int zy);
// #endif

// #ifdef I386_ASM
// /* global declaration for video_i386.asm */
// uint8_t **mem_gfx; //=&memory.rom.tiles.p;
// uint8_t *mem_video; //=memory.vid.ram;
// 
// /* prototype */
// void draw_tile_i386_norm(uint32_t tileno, int sx, int sy, int zx, int zy,
// 		int color, int xflip, int yflip, uint8_t *bmp);
// void draw_tile_i386_50(uint32_t tileno, int sx, int sy, int zx, int zy,
// 		int color, int xflip, int yflip, uint8_t *bmp);
// void draw_one_char_i386(int byte1, int byte2, uint16_t *br);
// 
// void draw_scanline_tile_i386_norm(uint32_t tileno, int yoffs, int sx, int line, int zx,
// 		int color, int xflip, uint8_t *bmp);
// 
// void draw_scanline_tile_i386_50(uint32_t tileno, int yoffs, int sx, int line, int zx,
// 		int color, int xflip, uint8_t *bmp);
// #endif

//uint8_t strip_usage[0x300];
#define PEN_USAGE(tileno) ((((uint32_t*) memory.rom.spr_usage.p)[tileno>>4]>>((tileno&0xF)*2))&0x3)


static uint8_t dr, dg, db, sr, sg, sb;
// static __inline__ uint16_t alpha_blend(uint16_t dest, uint16_t src, uint8_t a) {
// 	
// 
// 	dr = ((dest & 0xF800) >> 11) << 3;
// 	dg = ((dest & 0x7E0) >> 5) << 2;
// 	db = ((dest & 0x1F)) << 3;
// 
// 	sr = ((src & 0xF800) >> 11) << 3;
// 	sg = ((src & 0x7E0) >> 5) << 2;
// 	sb = ((src & 0x1F)) << 3;
// 
// 	dr = (((sr - dr)*(a)) >> 8) + dr;
// 	dg = (((sg - dg)*(a)) >> 8) + dg;
// 	db = (((sb - db)*(a)) >> 8) + db;
// 
// 	return ((dr >> 3) << 11) | ((dg >> 2) << 5) | (db >> 3);
// }
// #define BLEND16_50(a,b) ((((a)&0xf7de)>>1)+(((b)&0xf7de)>>1))
// #define BLEND16_25(a,b) alpha_blend(a,b,63)


char dda_y_skip[17];
uint32_t dda_y_skip_i;
uint32_t full_y_skip_i = 0xFFFE;
char full_y_skip[16] = {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
uint32_t neogeo_frame_counter_speed = 8;

// static uint16_t fix_addr[40][32];
// static uint8_t fix_shift[40];

static uint16_t dda_x_skip_i;
const uint16_t ddaxskip_i[17] = {
	0x0000, 0x0080, 0x0880, 0x0888, 0x2888, 0x288a, 0x2a8a, 0x2aaa, 0xaaaa,
	0xaaea, 0xbaea, 0xbaeb, 0xbbeb, 0xbbef, 0xfbef, 0xfbff, 0xffff
};
#define dda_x_skip(N) (dda_x_skip_i & (1 << N))
static void fix_value_init(void) {
	// int x, y;
	// for (x = 0; x < 40; x++) {
	// 	for (y = 0; y < 32; y++) {
	// 		fix_addr[x][y] = 0xea00 + (y << 1) + 64 * (x / 6);
	// 	}
	// 	fix_shift[x] = (5 - (x % 6));
	// }
}


/* Drawing function generation */
// #define RENAME(name) name##_tile
// #define PUTPIXEL(dst,src) dst=src
// #include "video_template.h"

// #define RENAME(name) name##_tile_50
// #define PUTPIXEL(dst,src) dst=BLEND16_50(src,dst)
// #include "video_template.h"
// 
// #define RENAME(name) name##_tile_25
// #define PUTPIXEL(dst,src) dst=BLEND16_25(src,dst)
// #include "video_template.h"
// 
// #ifdef PROCESSOR_ARM
// 
//  
// #endif
const int overscan = 1;


extern struct RastPort *theRastPort;

uint8_t line_limit[256] = {0xFF}, *limit;
uint32_t even_odd = 0;

//(palbase, tilepos, gfxdata, rzx, yskip)

//OPTION_INTERLEAVED

void draw_screen(void) { 
 	if(screen_prerender()) { 
		if(arg[OPTION_INTERLEAVED]) {
			uint8_t c = even_odd - 1;
			uint32_t i, x;
			uint16_t *clear = (uint16_t*)bufferpixels;

			for(i=0;i<224;i++) {
				line_limit[i] = c;
				if(!c) {
					uint16_t pixel = current_pc_pal[4095];
					for(x=0;x<320;x++) clear[x] = pixel;
					c = 255;
				} else {
					c = 0;
				}
				clear += 384;
			}
			even_odd = !even_odd;

		} else {
			bzero( line_limit, 224 );		
			clr_screen_m68k(bufferpixels, current_pc_pal[4095] );
		}

		/* Draw sprites */
		if(AC68080) draw_tiles_ammx();
		else draw_tiles_m68k();
		
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
#ifdef PROCESSOR_ARM
	if (!mem_gfx) {
		mem_gfx = memory.rom.tiles.p;
	}
	if (!mem_video) {
		mem_video = memory.vid.ram;
	}
#elif I386_ASM
	mem_gfx = &memory.rom.tiles.p;
	mem_video = memory.vid.ram;
#endif
	fix_value_init();
	memory.vid.modulo = 1;
}
