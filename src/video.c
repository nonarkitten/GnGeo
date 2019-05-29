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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>
#include <stdlib.h>
#include <zlib.h>
#include "video.h"
#include "memory.h"
#include "emu.h"
#include "messages.h"
#include "screen.h"
#include "frame_skip.h"
#include "transpack.h"
#include <cybergraphx/cybergraphics.h>
#include <proto/cybergraphics.h>
extern int neogeo_fix_bank_type;


// #ifdef PROCESSOR_ARM
// /* global declaration for video_arm.S */
// Uint8 *mem_gfx = NULL; /*=memory.rom.tiles.p;*/
// Uint8 *mem_video = NULL; //memory.vid.ram;
// //#define TOTAL_GFX_BANK 4096
// Uint32 *mem_bank_usage;
// 
// //GFX_CACHE gcache;
// 
// void draw_one_char_arm(int byte1, int byte2, unsigned short *br);
// int draw_tile_arm_norm(unsigned int tileno, int color, unsigned char *bmp, int zy);
// #endif

// #ifdef I386_ASM
// /* global declaration for video_i386.asm */
// Uint8 **mem_gfx; //=&memory.rom.tiles.p;
// Uint8 *mem_video; //=memory.vid.ram;
// 
// /* prototype */
// void draw_tile_i386_norm(unsigned int tileno, int sx, int sy, int zx, int zy,
// 		int color, int xflip, int yflip, unsigned char *bmp);
// void draw_tile_i386_50(unsigned int tileno, int sx, int sy, int zx, int zy,
// 		int color, int xflip, int yflip, unsigned char *bmp);
// void draw_one_char_i386(int byte1, int byte2, unsigned short *br);
// 
// void draw_scanline_tile_i386_norm(unsigned int tileno, int yoffs, int sx, int line, int zx,
// 		int color, int xflip, unsigned char *bmp);
// 
// void draw_scanline_tile_i386_50(unsigned int tileno, int yoffs, int sx, int line, int zx,
// 		int color, int xflip, unsigned char *bmp);
// #endif

//Uint8 strip_usage[0x300];
#define PEN_USAGE(tileno) ((((Uint32*) memory.rom.spr_usage.p)[tileno>>4]>>((tileno&0xF)*2))&0x3)


static Uint8 dr, dg, db, sr, sg, sb;
// static __inline__ Uint16 alpha_blend(Uint16 dest, Uint16 src, Uint8 a) {
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

//#define INLINE static inline
#define INLINE extern

char dda_y_skip[17];
Uint32 dda_y_skip_i;
Uint32 full_y_skip_i = 0xFFFE;
char full_y_skip[16] = {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
unsigned int neogeo_frame_counter_speed = 8;
static Uint16 fix_addr[40][32];
static Uint8 fix_shift[40];

static uint16_t dda_x_skip_i;
const uint16_t ddaxskip_i[17] = {
	0x0000, 0x0080, 0x0880, 0x0888, 0x2888, 0x288a, 0x2a8a, 0x2aaa, 0xaaaa,
	0xaaea, 0xbaea, 0xbaeb, 0xbbeb, 0xbbef, 0xfbef, 0xfbff, 0xffff
};
#define dda_x_skip(N) (dda_x_skip_i & (1 << N))
static void fix_value_init(void) {
	int x, y;
	for (x = 0; x < 40; x++) {
		for (y = 0; y < 32; y++) {
			fix_addr[x][y] = 0xea00 + (y << 1) + 64 * (x / 6);
		}
		fix_shift[x] = (5 - (x % 6));
	}
}


/* Drawing function generation */
#define RENAME(name) name##_tile
#define PUTPIXEL(dst,src) dst=src
#include "video_template.h"

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

extern void draw_fix_char(unsigned char *buf, int start, int end);

extern struct RastPort *theRastPort;

uint8_t line_limit[256] = {0}, *limit;

//(palbase, tilepos, gfxdata, rzx, yskip)
extern void draw_tile_m68k(unsigned int tileno,int sx,int sy,int zx,int zy, int color,int xflip,int yflip,unsigned char *bmp);
extern void draw_tile_ammx(unsigned int tileno,int sx,int sy,int zx,int zy, int color,int xflip,int yflip,unsigned char *bmp);

extern int AC68080;
extern void draw_tiles_m68k(uint16_t *bufferpixels);

void draw_screen(void) { 
 	if(screen_prerender()) { 
		bzero( line_limit, 256 );		
		clr_screen_m68k(bufferpixels, current_pc_pal[4095] );

		/* Draw sprites */
		if(AC68080) 
			draw_tiles_ammx(bufferpixels);
		else
			draw_tiles_m68k(bufferpixels);
		
		/* Draw fix layer */
		draw_fix_char(bufferpixels, 0, 0); 
		
		clr_border_m68k(bufferpixels, 0 );
		screen_update();
		
	} else {
		printf("Unable to lock screen (%p)\n", bufferpixels);
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
