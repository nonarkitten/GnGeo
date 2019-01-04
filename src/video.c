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


#ifdef PROCESSOR_ARM
/* global declaration for video_arm.S */
Uint8 *mem_gfx = NULL; /*=memory.rom.tiles.p;*/
Uint8 *mem_video = NULL; //memory.vid.ram;
//#define TOTAL_GFX_BANK 4096
Uint32 *mem_bank_usage;

//GFX_CACHE gcache;

void draw_one_char_arm(int byte1, int byte2, unsigned short *br);
int draw_tile_arm_norm(unsigned int tileno, int color, unsigned char *bmp, int zy);
#endif

#ifdef I386_ASM
/* global declaration for video_i386.asm */
Uint8 **mem_gfx; //=&memory.rom.tiles.p;
Uint8 *mem_video; //=memory.vid.ram;

/* prototype */
void draw_tile_i386_norm(unsigned int tileno, int sx, int sy, int zx, int zy,
		int color, int xflip, int yflip, unsigned char *bmp);
void draw_tile_i386_50(unsigned int tileno, int sx, int sy, int zx, int zy,
		int color, int xflip, int yflip, unsigned char *bmp);
void draw_one_char_i386(int byte1, int byte2, unsigned short *br);

void draw_scanline_tile_i386_norm(unsigned int tileno, int yoffs, int sx, int line, int zx,
		int color, int xflip, unsigned char *bmp);

void draw_scanline_tile_i386_50(unsigned int tileno, int yoffs, int sx, int line, int zx,
		int color, int xflip, unsigned char *bmp);
#endif

//Uint8 strip_usage[0x300];
#define PEN_USAGE(tileno) ((((Uint32*) memory.rom.spr_usage.p)[tileno>>4]>>((tileno&0xF)*2))&0x3)


char *ldda_y_skip;
char *dda_x_skip;
char ddaxskip[16][16] = {
	{ 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0},
	{ 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0},
	{ 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0},
	{ 0, 0, 1, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0},
	{ 0, 0, 1, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 1, 0},
	{ 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0, 0, 1, 0, 1, 0},
	{ 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0},
	{ 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0},
	{ 1, 0, 1, 0, 1, 0, 1, 0, 1, 1, 1, 0, 1, 0, 1, 0},
	{ 1, 0, 1, 1, 1, 0, 1, 0, 1, 1, 1, 0, 1, 0, 1, 0},
	{ 1, 0, 1, 1, 1, 0, 1, 0, 1, 1, 1, 0, 1, 0, 1, 1},
	{ 1, 0, 1, 1, 1, 0, 1, 1, 1, 1, 1, 0, 1, 0, 1, 1},
	{ 1, 0, 1, 1, 1, 0, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1},
	{ 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1},
	{ 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
	{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}
};
Uint32 ddaxskip_i[16] = {
	0x0080, 0x0880, 0x0888, 0x2888, 0x288a, 0x2a8a, 0x2aaa, 0xaaaa,
	0xaaea, 0xbaea, 0xbaeb, 0xbbeb, 0xbbef, 0xfbef, 0xfbff, 0xffff
};
Uint32 dda_x_skip_i;
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


char dda_y_skip[17];
Uint32 dda_y_skip_i;
Uint32 full_y_skip_i = 0xFFFE;
char full_y_skip[16] = {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
unsigned int neogeo_frame_counter_speed = 8;
static Uint16 fix_addr[40][32];
static Uint8 fix_shift[40];

#if 0
int init_sprite_cache(Uint32 size, Uint32 bsize) {
	GFX_CACHE *gcache = &memory.vid.spr_cache;
	int i;

	if (gcache->data != NULL) { /* We allready have a cache, just reset it */
		memset(gcache->ptr, 0, gcache->total_bank * sizeof (Uint8*));
		for (i = 0; i < gcache->max_slot; i++)
			gcache->usage[i] = -1;
		return 0;
	}

	/* Create our video cache */
	gcache->slot_size = bsize;
	printf("gfx_size=%08x\n", memory.rom.tiles.size);
	gcache->total_bank = memory.rom.tiles.size / gcache->slot_size;
	gcache->ptr = malloc(gcache->total_bank * sizeof (Uint8*));
	if (gcache->ptr == NULL)
		return 1;
	//gcache->z_pos=malloc(gcache->total_bank*sizeof(unz_file_pos ));
	memset(gcache->ptr, 0, gcache->total_bank * sizeof (Uint8*));

	gcache->size = size;
	gcache->data = malloc(gcache->size);
	if (gcache->data == NULL) {
		free(gcache->ptr);
		return 1;
	}
	printf("INIT CACHE %p\n", gcache->data);

	//gcache->max_slot=((float)gcache->size/0x4000000)*TOTAL_GFX_BANK;
	//gcache->max_slot=((float)gcache->size/memory.rom.tiles.size)*gcache->total_bank;
	gcache->max_slot = size / gcache->slot_size;
	//gcache->slot_size=0x4000000/TOTAL_GFX_BANK;
	printf("Allocating %08x for gfx cache (%d %d slot)\n", gcache->size, gcache->max_slot, gcache->slot_size);
	gcache->usage = malloc(gcache->max_slot * sizeof (Uint32));
	for (i = 0; i < gcache->max_slot; i++)
		gcache->usage[i] = -1;
	//printf("inbuf size= %d\n",compressBound(bsize));
//#ifdef WIZ
	gcache->in_buf = malloc(bsize + 1024);
//#else
//	gcache->in_buf = malloc(compressBound(bsize));
//#endif
	return 0;
}

void free_sprite_cache(void) {
	GFX_CACHE *gcache = &memory.vid.spr_cache;
	if (gcache->data) {
		free(gcache->data);
		gcache->data = NULL;
	}
	if (gcache->ptr) {
		free(gcache->ptr);
		gcache->ptr = NULL;
	}
	if (gcache->usage) {
		free(gcache->usage);
		gcache->usage = NULL;
	}
	if (gcache->in_buf) {
		free(gcache->in_buf);
		gcache->in_buf = NULL;
	}
}

Uint8 *get_cached_sprite_ptr(Uint32 tileno) {
	GFX_CACHE *gcache = &memory.vid.spr_cache;
	static int pos = 0;
	static int init = 1;
	int tile_sh = ~((gcache->slot_size >> 7) - 1);

	int bank = ((tileno & tile_sh) / (gcache->slot_size >> 7));
	int a;
	int r;
	Uint32 cmp_size;
	uLongf dst_size;

	if (gcache->ptr[bank]) {
		/* The bank is present in the cache */
		return gcache->ptr[bank];
	}
	/* We have to find a slot for this bank */
	a = pos;
	pos++;
	if (pos >= gcache->max_slot) pos = 0;
	//printf("Offset for bank is %d\n",gcache->offset[bank]);

	fseek(gcache->gno, gcache->offset[bank], SEEK_SET);
	r = fread(&cmp_size, sizeof (Uint32), 1, gcache->gno);
	r = fread(gcache->in_buf, cmp_size, 1, gcache->gno);
	dst_size = gcache->slot_size;
	r = uncompress(gcache->data + a * gcache->slot_size, &dst_size, gcache->in_buf, cmp_size);

	gcache->ptr[bank] = gcache->data + a * gcache->slot_size;

	if (gcache->usage[a] != -1) {
		gcache->ptr[gcache->usage[a]] = 0;
	}
	gcache->usage[a] = bank;
	return gcache->ptr[bank];
}
#endif

static void fix_value_init(void) {
	int x, y;
	for (x = 0; x < 40; x++) {
		for (y = 0; y < 32; y++) {
			fix_addr[x][y] = 0xea00 + (y << 1) + 64 * (x / 6);
		}
		fix_shift[x] = (5 - (x % 6));
	}
}

#define fix_add(x, y) ((((READ_WORD(memory.vid.ram + 0xEA00 + (((y-1)&31)*2 + 64 * (x/6))) >> (5-(x%6))*2) & 3) ^ 3))

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

static __inline__ void draw_fix_char(unsigned char *buf, int start, int end) {
	unsigned int *gfxdata, myword;
	int x, y, yy;
	unsigned char col;
	unsigned short *br;
	unsigned short *paldata;
	unsigned int byte1, byte2;
	int banked, garouoffsets[32];
	Rect clip;
	int ystart = 1, yend = 32;
	uint16_t test = 0;

	test += 1;
	//banked = (current_fix == memory.rom.game_sfix.p && memory.rom.game_sfix.size > 0x1000) ? 1 : 0;
	banked = (current_fix == memory.rom.game_sfix.p && neogeo_fix_bank_type && memory.rom.game_sfix.size > 0x1000) ? 1 : 0;

	//if (banked && conf.rom_type==MVS_CMC42)
	if (banked && neogeo_fix_bank_type == 1) {
		int garoubank = 0;
		int k = 0;
		y = 0;

		while (y < 32) {
			if (READ_WORD(&memory.vid.ram[0xea00 + (k << 1)]) == 0x0200 &&
					(READ_WORD(&memory.vid.ram[0xeb00 + (k << 1)]) & 0xff00) == 0xff00) {

				garoubank = READ_WORD(&memory.vid.ram[0xeb00 + (k << 1)]) & 3;
				garouoffsets[y++] = garoubank;
			}
			garouoffsets[y++] = garoubank;
			k += 2;
		}
	}

	for (x = 1; x < 39; x++) {
		br = (unsigned short*)buf + ((ystart << 3)) * (PITCH >> 1) + (x << 3) + 16;
		for (y = ystart; y < yend; y++) {
			byte1 = (READ_WORD(&memory.vid.ram[0xE000 + ((y + (x << 5)) << 1)]));
			byte2 = byte1 >> 12;
			byte1 = byte1 & 0xfff;

			br[0] = test;
			
			if (banked) {
				/* Garou, MSlug 3 */
				if(neogeo_fix_bank_type == 1)
					byte1 += 0x1000 * (garouoffsets[(y - 2)&31] ^ 3);
				
				else if(neogeo_fix_bank_type == 2)
					byte1 += 0x1000 * fix_add(x, y);
			}

			if ((byte1 >= (memory.rom.game_sfix.size >> 5)) || (fix_usage[byte1] == 0x00)) continue;

			paldata = (uint16_t *) &current_pc_pal[16 * byte2];
			gfxdata = (uint32_t *) &current_fix[byte1 << 5];
 
			#define ROW(n) \
				myword = gfxdata[n]; \
				col = (myword >> 28)&0xf; if (col) br[7] = paldata[col]; \
				col = (myword >> 24)&0xf; if (col) br[6] = paldata[col]; \
				col = (myword >> 20)&0xf; if (col) br[5] = paldata[col]; \
				col = (myword >> 16)&0xf; if (col) br[4] = paldata[col]; \
				col = (myword >> 12)&0xf; if (col) br[3] = paldata[col]; \
				col = (myword >>  8)&0xf; if (col) br[2] = paldata[col]; \
				col = (myword >>  4)&0xf; if (col) br[1] = paldata[col]; \
				col = (myword >>  0)&0xf; if (col) br[0] = paldata[col]; \
				br += 320;
	
				ROW(0)
				ROW(1)
				ROW(2)
				ROW(3)
				ROW(4)
				ROW(5)
				ROW(6)
				ROW(7)
			#undef ROW

		}
	}
//	if (start != 0 && end != 0) SDL_SetClipRect(buffer, NULL);
}
extern struct RastPort *theRastPort;

void clr_screen_m68k( unsigned long* ptr, unsigned short col) { 
    int y; 
    int x;
    unsigned int col2; 
    col2 = (col <<16) | col; 
    ptr += 320 * 2 * 16 / 4;
    for (y=240-16;y;y--){ 
      ptr+=16;
      for (x=320/4 - 8;x;x--){ 
        *ptr++ = col2; 
        *ptr++ = col2;
      }
    }
}

void clr_border_m68k( unsigned long* ptr, unsigned short col) { 
    int y; 
    int x;
    unsigned int col2; 
    col2 = (col <<16) | col; 
    
    ptr+=16;
	for(x=320/4 - 8;x;x--) {
		*ptr++ = col2; 
		*ptr++ = col2;
	}
    
    for (y=15;y;y--) {
    	for(x=320/4;x;x--) {
			*ptr++ = col2; 
			*ptr++ = col2;
		}
	}		
    for (y=240-16;y;y--){ 
		*ptr++ = col2; 
		*ptr++ = col2;
		*ptr++ = col2; 
		*ptr++ = col2;
		*ptr++ = col2; 
		*ptr++ = col2;
		*ptr++ = col2; 
		*ptr++ = col2;
		*ptr++ = col2; 
		*ptr++ = col2;
		*ptr++ = col2; 
		*ptr++ = col2;
		*ptr++ = col2; 
		*ptr++ = col2;
		*ptr++ = col2; 
		*ptr++ = col2;
		ptr += 320/2 - 16;
    }
}

void draw_screen(void) {
	int sx = 0, sy = 0, oy = 0, my = 0, zx = 1, rzy = 1;
	unsigned int offs, i, count, y;
	unsigned int tileno, tileatr, t1, t2, t3;
	char fullmode = 0;
	int ddax = 0, dday = 0, rzx = 15, yskip = 0;
	Uint8 *vidram = memory.vid.ram;
	Uint8 penusage;
 
 	if(screen_prerender()) { 
		bufferpixels -= 32; // HACK HACK HACK
		
		clr_screen_m68k(bufferpixels, current_pc_pal[4095] );

#if 0
		/* Draw sprites */
		for (count = 0; count < 0x300; count += 2) {
			t3 = READ_WORD(&vidram[0x10000 + count]);
			t1 = READ_WORD(&vidram[0x10400 + count]);
			t2 = READ_WORD(&vidram[0x10800 + count]);

			//printf("%04x %04x %04x\n",t3,t1,t2);
			/* If this bit is set this new column is placed next to last one */
			if (t1 & 0x40) {
				sx += rzx; /* new x */

				/* Get new zoom for this column */
				zx = (t3 >> 8) & 0x0f;
				sy = oy; /* y pos = old y pos */
			
			} else { /* nope it is a new block */
				/* Sprite scaling */
				zx = (t3 >> 8) & 0x0f; /* zomm x */
				rzy = t3 & 0xff; /* zoom y */
				if (rzy == 0) continue;
				sx = (t2 >> 7); /* x pos */

				/* Number of tiles in this strip */
				my = t1 & 0x3f;

				if (my == 0x20) fullmode = 1;
				else if (my >= 0x21) fullmode = 2; /* most games use 0x21, but */
					/* Alpha Mission II uses 0x3f */
				else fullmode = 0;

				sy = 0x200 - (t1 >> 7); /* sprite bank position */

				if (sy > 0x110) sy -= 0x200;

				if (fullmode == 2 || (fullmode == 1 && rzy == 0xff)) {
					while (sy < 0) sy += ((rzy + 1) << 1);
				}

				oy = sy; /* on se souvient de la position y */

				if (rzy < 0xff && my < 0x10 && my) {
					//my = (my<<8)/(rzy+1);
					my = (my << 8) - my;
					my /= rzy;
					//my = my * 255 / rzy;
					if (my > 0x10) my = 0x10;
				}

				if (my > 0x20) my = 0x20;

				ddax = 0; /* =16; NS990110 neodrift fix */ /* setup x zoom */
			}

			/* No point doing anything if tile strip is 0 */
			if (my == 0) continue;

			/* Process x zoom */
			if (zx != 15) {
				dda_x_skip = ddaxskip[zx];
				rzx = zx + 1;

			} else rzx = 16;

			if (sx >= 0x1F0) sx -= 0x200;
			if (sx >= 320) continue;

			/* Setup y zoom */
			if (rzy == 255) yskip = 16;
			else dday = 0; /* =256; NS990105 mslug fix */

			offs = count << 6;

			/* my holds the number of tiles in each vertical multisprite block */
			for (y = 0; y < my; y++) {
				tileno = READ_WORD(&vidram[offs]);
				offs += 2;
				tileatr = READ_WORD(&vidram[offs]);
				offs += 2;

				if (memory.nb_of_tiles > 0x10000 && tileatr & 0x10) tileno += 0x10000;
				if (memory.nb_of_tiles > 0x20000 && tileatr & 0x20) tileno += 0x20000;
				if (memory.nb_of_tiles > 0x40000 && tileatr & 0x40) tileno += 0x40000;

				/* animation automatique */
				/*if (tileatr&0x80) printf("PLOP\n");*/
				if (tileatr & 0x8) {
					tileno = (tileno&~7)+((tileno + neogeo_frame_counter)&7);
				} else {
					if (tileatr & 0x4) {
						tileno = (tileno&~3)+((tileno + neogeo_frame_counter)&3);
					}
				}

				if (tileno > memory.nb_of_tiles) continue;

				if (fullmode == 2 || (fullmode == 1 && rzy == 0xff)) {
					if (sy >= 248) sy -= ((rzy + 1) << 1);
				
				} else if (fullmode == 1) {
					if (y == 0x10) sy -= ((rzy + 1) << 1);
				
				} else if (sy > 0x110) {
					sy -= 0x200; // NS990105 mslug2 fix
				
				}

				if (rzy != 255) {
					yskip = 0;
					dda_y_skip_i = 0;
					dda_y_skip[0] = 0;
					for (i = 0; i < 16; i++) {
						dda_y_skip[i + 1] = 0;
						dday -= (rzy + 1);
						if (dday <= 0) {
							dday += 256;
							yskip++;
							dda_y_skip[yskip]++;
						} else dda_y_skip[yskip]++;
					}
				}

				if (sx >= -16 && sx + 15 < 336 && sy >= 0 && sy + 15 < 256) {
// 					if (memory.vid.spr_cache.data) {
// 						memory.rom.tiles.p = get_cached_sprite_ptr(tileno);
// 						tileno = (tileno & ((memory.vid.spr_cache.slot_size >> 7) - 1));
// 					}

					if (PEN_USAGE(tileno) != TILE_INVISIBLE)
						draw_tile(tileno, sx + 16, sy, rzx, yskip, tileatr >> 8,
							tileatr & 0x01, tileatr & 0x02, bufferpixels);
						
				}

				sy += yskip;
			} /* for y */
		} /* for count */
#endif
		draw_fix_char(bufferpixels, 0, 0); 
		
		clr_border_m68k(bufferpixels, current_pc_pal[4095] );
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
