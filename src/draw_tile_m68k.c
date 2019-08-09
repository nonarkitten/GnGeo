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

#define PITCH 768

#define PEN_USAGE(tileno) ((((Uint32*) memory.rom.spr_usage.p)[tileno>>4]>>((tileno&0xF)*2))&0x3)

typedef union {
	struct {
		uint32_t p0 : 4;
		uint32_t p1 : 4;
		uint32_t p2 : 4;
		uint32_t p3 : 4;
		uint32_t p4 : 4;
		uint32_t p5 : 4;
		uint32_t p6 : 4;
		uint32_t p7 : 4;
	} p;
	uint32_t pixel;
} packpix_t; 

extern const uint16_t ddaxskip_i[17];

extern uint8_t line_limit[256], *limit;
static uint32_t scalex;

#define DO_NOT_REORDER() asm volatile("": : :"memory")

INLINE void __attribute__((regparm(4))) draw_tile_m68k_norm (uint32_t*palbase,uint16_t*screen,uint32_t*gfxdata,int scaley) {
	packpix_t pixels0;
	int16_t y = 16;
    uint32_t pixel1, pixel2;
	
	while(y--) {
		if(scaley & 0x8000) {
			if(*limit < 96) {
				*limit += 1; 

                if((pixels0.pixel = gfxdata[0])) {                 pixel1 = pixels0.p.p0; DO_NOT_REORDER();
                                                                   pixel2 = pixels0.p.p1; DO_NOT_REORDER();
					if(pixel1) screen[0x0] = palbase[pixel1];  pixel1 = pixels0.p.p2;
					if(pixel2) screen[0x1] = palbase[pixel2];  pixel2 = pixels0.p.p3;
					if(pixel1) screen[0x2] = palbase[pixel1];  pixel1 = pixels0.p.p4;
					if(pixel2) screen[0x3] = palbase[pixel2];  pixel2 = pixels0.p.p5;
					if(pixel1) screen[0x4] = palbase[pixel1];  pixel1 = pixels0.p.p6;
					if(pixel2) screen[0x5] = palbase[pixel2];  pixel2 = pixels0.p.p7;
					if(pixel1) screen[0x6] = palbase[pixel1];
					if(pixel2) screen[0x7] = palbase[pixel2];
                }

                if((pixels0.pixel = gfxdata[1])) {                 pixel1 = pixels0.p.p0; DO_NOT_REORDER();
                                                                   pixel2 = pixels0.p.p1; DO_NOT_REORDER();
					if(pixel1) screen[0x8] = palbase[pixel1];  pixel1 = pixels0.p.p2;
					if(pixel2) screen[0x9] = palbase[pixel2];  pixel2 = pixels0.p.p3;
					if(pixel1) screen[0xA] = palbase[pixel1];  pixel1 = pixels0.p.p4;
					if(pixel2) screen[0xB] = palbase[pixel2];  pixel2 = pixels0.p.p5;
					if(pixel1) screen[0xC] = palbase[pixel1];  pixel1 = pixels0.p.p6;
					if(pixel2) screen[0xD] = palbase[pixel2];  pixel2 = pixels0.p.p7;
					if(pixel1) screen[0xE] = palbase[pixel1];
					if(pixel2) screen[0xF] = palbase[pixel2];
                }
			}
			screen += PITCH / 2;
			limit++;
		}
		scaley <<= 1;
		gfxdata += 2;
	}
}
INLINE void __attribute__((regparm(4))) draw_tile_m68k_xflip_norm (uint32_t*palbase,uint16_t*screen,uint32_t*gfxdata,int scaley) {
	packpix_t pixels0;
	uint16_t y = 16;
    uint16_t pixel1, pixel2;
	
	while(y--) {
		if(scaley & 0x8000) {
			if(*limit < 96) {
				*limit += 1; 

                if((pixels0.pixel = gfxdata[1])) {                 pixel1 = pixels0.p.p7; DO_NOT_REORDER();
                                                                   pixel2 = pixels0.p.p6; DO_NOT_REORDER();
					if(pixel1) screen[0x0] = palbase[pixel1];  pixel1 = pixels0.p.p5;
					if(pixel2) screen[0x1] = palbase[pixel2];  pixel2 = pixels0.p.p4;
					if(pixel1) screen[0x2] = palbase[pixel1];  pixel1 = pixels0.p.p3;
					if(pixel2) screen[0x3] = palbase[pixel2];  pixel2 = pixels0.p.p2;
					if(pixel1) screen[0x4] = palbase[pixel1];  pixel1 = pixels0.p.p1;
					if(pixel2) screen[0x5] = palbase[pixel2];  pixel2 = pixels0.p.p0;
					if(pixel1) screen[0x6] = palbase[pixel1];
					if(pixel2) screen[0x7] = palbase[pixel2];
                }

                if((pixels0.pixel = gfxdata[0])) {                 pixel1 = pixels0.p.p7; DO_NOT_REORDER();
                                                                   pixel2 = pixels0.p.p6; DO_NOT_REORDER();
					if(pixel1) screen[0x8] = palbase[pixel1];  pixel1 = pixels0.p.p5;
					if(pixel2) screen[0x9] = palbase[pixel2];  pixel2 = pixels0.p.p4;
					if(pixel1) screen[0xA] = palbase[pixel1];  pixel1 = pixels0.p.p3;
					if(pixel2) screen[0xB] = palbase[pixel2];  pixel2 = pixels0.p.p2;
					if(pixel1) screen[0xC] = palbase[pixel1];  pixel1 = pixels0.p.p1;
					if(pixel2) screen[0xD] = palbase[pixel2];  pixel2 = pixels0.p.p0;
					if(pixel1) screen[0xE] = palbase[pixel1];
					if(pixel2) screen[0xF] = palbase[pixel2];
                }
			}
			screen += PITCH / 2;
			limit++;
		}
		scaley <<= 1;
		gfxdata += 2;
	}
}
INLINE void __attribute__((regparm(4))) draw_tile_m68k_yflip_norm  (uint32_t*palbase,uint16_t*screen,uint32_t*gfxdata,int scaley) { 
	packpix_t pixels0;
	uint16_t y = 16;
    uint16_t pixel1, pixel2;
	
	gfxdata += 30;
	while(y--) {
		if(scaley & 0x8000) {
			if(*limit < 96) {
				*limit += 1; 
				
                if((pixels0.pixel = gfxdata[0])) {                 pixel1 = pixels0.p.p0; DO_NOT_REORDER();
                                                                   pixel2 = pixels0.p.p1; DO_NOT_REORDER();
					if(pixel1) screen[0x0] = palbase[pixel1];  pixel1 = pixels0.p.p2;
					if(pixel2) screen[0x1] = palbase[pixel2];  pixel2 = pixels0.p.p3;
					if(pixel1) screen[0x2] = palbase[pixel1];  pixel1 = pixels0.p.p4;
					if(pixel2) screen[0x3] = palbase[pixel2];  pixel2 = pixels0.p.p5;
					if(pixel1) screen[0x4] = palbase[pixel1];  pixel1 = pixels0.p.p6;
					if(pixel2) screen[0x5] = palbase[pixel2];  pixel2 = pixels0.p.p7;
					if(pixel1) screen[0x6] = palbase[pixel1];
					if(pixel2) screen[0x7] = palbase[pixel2];
                }

                if((pixels0.pixel = gfxdata[1])) {                 pixel1 = pixels0.p.p0; DO_NOT_REORDER();
                                                                   pixel2 = pixels0.p.p1; DO_NOT_REORDER();
					if(pixel1) screen[0x8] = palbase[pixel1];  pixel1 = pixels0.p.p2;
					if(pixel2) screen[0x9] = palbase[pixel2];  pixel2 = pixels0.p.p3;
					if(pixel1) screen[0xA] = palbase[pixel1];  pixel1 = pixels0.p.p4;
					if(pixel2) screen[0xB] = palbase[pixel2];  pixel2 = pixels0.p.p5;
					if(pixel1) screen[0xC] = palbase[pixel1];  pixel1 = pixels0.p.p6;
					if(pixel2) screen[0xD] = palbase[pixel2];  pixel2 = pixels0.p.p7;
					if(pixel1) screen[0xE] = palbase[pixel1];
					if(pixel2) screen[0xF] = palbase[pixel2];
                }
							
			}
			screen += PITCH / 2;
			limit++;
		}
		scaley <<= 1;
		gfxdata -= 2;
	}
}
INLINE void __attribute__((regparm(4))) draw_tile_m68k_xyflip_norm (uint32_t*palbase,uint16_t*screen,uint32_t*gfxdata,int scaley) { 
	packpix_t pixels0;
	uint16_t y = 16;
    uint16_t pixel1, pixel2;
	
	gfxdata += 30;
	while(y--) {
		if(scaley & 0x8000) {
			if(*limit < 96) {
				*limit += 1; 

                if((pixels0.pixel = gfxdata[1])) {                 pixel1 = pixels0.p.p7; DO_NOT_REORDER();
                                                                   pixel2 = pixels0.p.p6; DO_NOT_REORDER();
					if(pixel1) screen[0x0] = palbase[pixel1];  pixel1 = pixels0.p.p5;
					if(pixel2) screen[0x1] = palbase[pixel2];  pixel2 = pixels0.p.p4;
					if(pixel1) screen[0x2] = palbase[pixel1];  pixel1 = pixels0.p.p3;
					if(pixel2) screen[0x3] = palbase[pixel2];  pixel2 = pixels0.p.p2;
					if(pixel1) screen[0x4] = palbase[pixel1];  pixel1 = pixels0.p.p1;
					if(pixel2) screen[0x5] = palbase[pixel2];  pixel2 = pixels0.p.p0;
					if(pixel1) screen[0x6] = palbase[pixel1];
					if(pixel2) screen[0x7] = palbase[pixel2];
                }

                if((pixels0.pixel = gfxdata[0])) {                 pixel1 = pixels0.p.p7; DO_NOT_REORDER();
                                                                   pixel2 = pixels0.p.p6; DO_NOT_REORDER();
					if(pixel1) screen[0x8] = palbase[pixel1];  pixel1 = pixels0.p.p5;
					if(pixel2) screen[0x9] = palbase[pixel2];  pixel2 = pixels0.p.p4;
					if(pixel1) screen[0xA] = palbase[pixel1];  pixel1 = pixels0.p.p3;
					if(pixel2) screen[0xB] = palbase[pixel2];  pixel2 = pixels0.p.p2;
					if(pixel1) screen[0xC] = palbase[pixel1];  pixel1 = pixels0.p.p1;
					if(pixel2) screen[0xD] = palbase[pixel2];  pixel2 = pixels0.p.p0;
					if(pixel1) screen[0xE] = palbase[pixel1];
					if(pixel2) screen[0xF] = palbase[pixel2];
                }
			}
			screen += PITCH / 2;
			limit++;
		}
		scaley <<= 1;
		gfxdata -= 2;
	}
}

INLINE void __attribute__((regparm(4))) draw_tile_m68k_xzoom (uint32_t*palbase,uint16_t*tilepos,uint32_t*gfxdata,int scaley) {
	packpix_t pixeldata;
	uint16_t color, y = 16;
	uint16_t* org_tilepos = tilepos;
	for(;;) {
		tilepos = org_tilepos;
		if(scaley & 0x8000) {
			if(*limit < 96) {
				*limit += 1;
			
				pixeldata.pixel = gfxdata[0]; 
				if(scalex & 0x8000) { if(pixeldata.p.p0) *tilepos = (uint16_t)palbase[pixeldata.p.p0]; tilepos++; }
				if(scalex & 0x4000) { if(pixeldata.p.p1) *tilepos = (uint16_t)palbase[pixeldata.p.p1]; tilepos++; }
				if(scalex & 0x2000) { if(pixeldata.p.p2) *tilepos = (uint16_t)palbase[pixeldata.p.p2]; tilepos++; }
				if(scalex & 0x1000) { if(pixeldata.p.p3) *tilepos = (uint16_t)palbase[pixeldata.p.p3]; tilepos++; }
				if(scalex & 0x0800) { if(pixeldata.p.p4) *tilepos = (uint16_t)palbase[pixeldata.p.p4]; tilepos++; }
				if(scalex & 0x0400) { if(pixeldata.p.p5) *tilepos = (uint16_t)palbase[pixeldata.p.p5]; tilepos++; }
				if(scalex & 0x0200) { if(pixeldata.p.p6) *tilepos = (uint16_t)palbase[pixeldata.p.p6]; tilepos++; }
				if(scalex & 0x0100) { if(pixeldata.p.p7) *tilepos = (uint16_t)palbase[pixeldata.p.p7]; tilepos++; }
 
				pixeldata.pixel = gfxdata[1]; 
				if(scalex & 0x0080) { if(pixeldata.p.p0) *tilepos = (uint16_t)palbase[pixeldata.p.p0]; tilepos++; }
				if(scalex & 0x0040) { if(pixeldata.p.p1) *tilepos = (uint16_t)palbase[pixeldata.p.p1]; tilepos++; }
				if(scalex & 0x0020) { if(pixeldata.p.p2) *tilepos = (uint16_t)palbase[pixeldata.p.p2]; tilepos++; }
				if(scalex & 0x0010) { if(pixeldata.p.p3) *tilepos = (uint16_t)palbase[pixeldata.p.p3]; tilepos++; }
				if(scalex & 0x0008) { if(pixeldata.p.p4) *tilepos = (uint16_t)palbase[pixeldata.p.p4]; tilepos++; }
				if(scalex & 0x0004) { if(pixeldata.p.p5) *tilepos = (uint16_t)palbase[pixeldata.p.p5]; tilepos++; }
				if(scalex & 0x0002) { if(pixeldata.p.p6) *tilepos = (uint16_t)palbase[pixeldata.p.p6]; tilepos++; }
				if(scalex & 0x0001) { if(pixeldata.p.p7) *tilepos = (uint16_t)palbase[pixeldata.p.p7]; tilepos++; }
			
			}
			org_tilepos += PITCH / 2;
			limit++;
		}
		if(!y) break;
		scaley <<= 1;
		gfxdata += 2;
		y -= 1;
	}
}
INLINE void __attribute__((regparm(4))) draw_tile_m68k_xzoomX  (uint32_t*palbase,uint16_t*tilepos,uint32_t*gfxdata,int scaley) { 
	packpix_t pixeldata;
	uint16_t color, y = 16;
	uint16_t* org_tilepos = tilepos;
	for(;;) {
		tilepos = org_tilepos;
		if(scaley & 0x8000) {
			if(*limit < 96) {
				*limit += 1;
			
				pixeldata.pixel = gfxdata[1];
				if(scalex & 0x0001) { if(pixeldata.p.p7) *tilepos = (uint16_t)palbase[pixeldata.p.p7]; tilepos++; }
				if(scalex & 0x0002) { if(pixeldata.p.p6) *tilepos = (uint16_t)palbase[pixeldata.p.p6]; tilepos++; }
				if(scalex & 0x0004) { if(pixeldata.p.p5) *tilepos = (uint16_t)palbase[pixeldata.p.p5]; tilepos++; }
				if(scalex & 0x0008) { if(pixeldata.p.p4) *tilepos = (uint16_t)palbase[pixeldata.p.p4]; tilepos++; }
				if(scalex & 0x0010) { if(pixeldata.p.p3) *tilepos = (uint16_t)palbase[pixeldata.p.p3]; tilepos++; }
				if(scalex & 0x0020) { if(pixeldata.p.p2) *tilepos = (uint16_t)palbase[pixeldata.p.p2]; tilepos++; }
				if(scalex & 0x0040) { if(pixeldata.p.p1) *tilepos = (uint16_t)palbase[pixeldata.p.p1]; tilepos++; }
				if(scalex & 0x0080) { if(pixeldata.p.p0) *tilepos = (uint16_t)palbase[pixeldata.p.p0]; tilepos++; }

				pixeldata.pixel = gfxdata[0];
				if(scalex & 0x0100) { if(pixeldata.p.p7) *tilepos = (uint16_t)palbase[pixeldata.p.p7]; tilepos++; }
				if(scalex & 0x0200) { if(pixeldata.p.p6) *tilepos = (uint16_t)palbase[pixeldata.p.p6]; tilepos++; }
				if(scalex & 0x0400) { if(pixeldata.p.p5) *tilepos = (uint16_t)palbase[pixeldata.p.p5]; tilepos++; }
				if(scalex & 0x0800) { if(pixeldata.p.p4) *tilepos = (uint16_t)palbase[pixeldata.p.p4]; tilepos++; }
				if(scalex & 0x1000) { if(pixeldata.p.p3) *tilepos = (uint16_t)palbase[pixeldata.p.p3]; tilepos++; }
				if(scalex & 0x2000) { if(pixeldata.p.p2) *tilepos = (uint16_t)palbase[pixeldata.p.p2]; tilepos++; }
				if(scalex & 0x4000) { if(pixeldata.p.p1) *tilepos = (uint16_t)palbase[pixeldata.p.p1]; tilepos++; }
				if(scalex & 0x8000) { if(pixeldata.p.p0) *tilepos = (uint16_t)palbase[pixeldata.p.p0]; tilepos++; }
			}
			
			org_tilepos += PITCH / 2;
			limit++;
		}
		if(!y) break;
		scaley <<= 1;
		gfxdata += 2;
		y -= 1;
	}
}
INLINE void __attribute__((regparm(4))) draw_tile_m68k_xzoomY  (uint32_t*palbase,uint16_t*tilepos,uint32_t*gfxdata,int scaley) { 
	packpix_t pixeldata;
	uint16_t color, y = 16;
	uint16_t* org_tilepos = tilepos;
	
	gfxdata += 30;
	for(;;) {
		tilepos = org_tilepos;
		if(scaley & 0x8000) {
			if(*limit < 96) {
				*limit += 1;
						
				pixeldata.pixel = gfxdata[0]; 
				if(scalex & 0x8000) { if(pixeldata.p.p0) *tilepos = (uint16_t)palbase[pixeldata.p.p0]; tilepos++; }
				if(scalex & 0x4000) { if(pixeldata.p.p1) *tilepos = (uint16_t)palbase[pixeldata.p.p1]; tilepos++; }
				if(scalex & 0x2000) { if(pixeldata.p.p2) *tilepos = (uint16_t)palbase[pixeldata.p.p2]; tilepos++; }
				if(scalex & 0x1000) { if(pixeldata.p.p3) *tilepos = (uint16_t)palbase[pixeldata.p.p3]; tilepos++; }
				if(scalex & 0x0800) { if(pixeldata.p.p4) *tilepos = (uint16_t)palbase[pixeldata.p.p4]; tilepos++; }
				if(scalex & 0x0400) { if(pixeldata.p.p5) *tilepos = (uint16_t)palbase[pixeldata.p.p5]; tilepos++; }
				if(scalex & 0x0200) { if(pixeldata.p.p6) *tilepos = (uint16_t)palbase[pixeldata.p.p6]; tilepos++; }
				if(scalex & 0x0100) { if(pixeldata.p.p7) *tilepos = (uint16_t)palbase[pixeldata.p.p7]; tilepos++; }

				pixeldata.pixel = gfxdata[1];
				if(scalex & 0x0080) { if(pixeldata.p.p0) *tilepos = (uint16_t)palbase[pixeldata.p.p0]; tilepos++; }
				if(scalex & 0x0040) { if(pixeldata.p.p1) *tilepos = (uint16_t)palbase[pixeldata.p.p1]; tilepos++; }
				if(scalex & 0x0020) { if(pixeldata.p.p2) *tilepos = (uint16_t)palbase[pixeldata.p.p2]; tilepos++; }
				if(scalex & 0x0010) { if(pixeldata.p.p3) *tilepos = (uint16_t)palbase[pixeldata.p.p3]; tilepos++; }
				if(scalex & 0x0008) { if(pixeldata.p.p4) *tilepos = (uint16_t)palbase[pixeldata.p.p4]; tilepos++; }
				if(scalex & 0x0004) { if(pixeldata.p.p5) *tilepos = (uint16_t)palbase[pixeldata.p.p5]; tilepos++; }
				if(scalex & 0x0002) { if(pixeldata.p.p6) *tilepos = (uint16_t)palbase[pixeldata.p.p6]; tilepos++; }
				if(scalex & 0x0001) { if(pixeldata.p.p7) *tilepos = (uint16_t)palbase[pixeldata.p.p7]; tilepos++; }
			}
			
			org_tilepos += PITCH / 2;
			limit++;
		}
		if(!y) break;
		scaley <<= 1;
		gfxdata -= 2;
		y -= 1;
	}
}
INLINE void __attribute__((regparm(4))) draw_tile_m68k_xzoomXY (uint32_t*palbase,uint16_t*tilepos,uint32_t*gfxdata,int scaley) { 
	packpix_t pixeldata;
	uint16_t color, y = 16;
	uint16_t* org_tilepos = tilepos;
	
	gfxdata += 30;
	for(;;) {
		tilepos = org_tilepos;
		if(scaley & 0x8000) {
			if(*limit < 96) {
				*limit += 1;
						
				pixeldata.pixel = gfxdata[1];
				if(scalex & 0x0001) { if(pixeldata.p.p7) *tilepos = (uint16_t)palbase[pixeldata.p.p7]; tilepos++; }
				if(scalex & 0x0002) { if(pixeldata.p.p6) *tilepos = (uint16_t)palbase[pixeldata.p.p6]; tilepos++; }
				if(scalex & 0x0004) { if(pixeldata.p.p5) *tilepos = (uint16_t)palbase[pixeldata.p.p5]; tilepos++; }
				if(scalex & 0x0008) { if(pixeldata.p.p4) *tilepos = (uint16_t)palbase[pixeldata.p.p4]; tilepos++; }
				if(scalex & 0x0010) { if(pixeldata.p.p3) *tilepos = (uint16_t)palbase[pixeldata.p.p3]; tilepos++; }
				if(scalex & 0x0020) { if(pixeldata.p.p2) *tilepos = (uint16_t)palbase[pixeldata.p.p2]; tilepos++; }
				if(scalex & 0x0040) { if(pixeldata.p.p1) *tilepos = (uint16_t)palbase[pixeldata.p.p1]; tilepos++; }
				if(scalex & 0x0080) { if(pixeldata.p.p0) *tilepos = (uint16_t)palbase[pixeldata.p.p0]; tilepos++; }

				pixeldata.pixel = gfxdata[0];
				if(scalex & 0x0100) { if(pixeldata.p.p7) *tilepos = (uint16_t)palbase[pixeldata.p.p7]; tilepos++; }
				if(scalex & 0x0200) { if(pixeldata.p.p6) *tilepos = (uint16_t)palbase[pixeldata.p.p6]; tilepos++; }
				if(scalex & 0x0400) { if(pixeldata.p.p5) *tilepos = (uint16_t)palbase[pixeldata.p.p5]; tilepos++; }
				if(scalex & 0x0800) { if(pixeldata.p.p4) *tilepos = (uint16_t)palbase[pixeldata.p.p4]; tilepos++; }
				if(scalex & 0x1000) { if(pixeldata.p.p3) *tilepos = (uint16_t)palbase[pixeldata.p.p3]; tilepos++; }
				if(scalex & 0x2000) { if(pixeldata.p.p2) *tilepos = (uint16_t)palbase[pixeldata.p.p2]; tilepos++; }
				if(scalex & 0x4000) { if(pixeldata.p.p1) *tilepos = (uint16_t)palbase[pixeldata.p.p1]; tilepos++; }
				if(scalex & 0x8000) { if(pixeldata.p.p0) *tilepos = (uint16_t)palbase[pixeldata.p.p0]; tilepos++; }
			}
			org_tilepos += PITCH / 2;
			limit++;
		}
		if(!y) break;
		scaley <<= 1;
		gfxdata -= 2;
		y -= 1;
	}
}

void draw_tiles_m68k(uint16_t *bufferpixels) {
	int sx = 0, sy = 0, oy = 0, my = 0, zx = 1, rzy = 1;
	unsigned int offs, i, count, y;
	unsigned int tileno, tileatr, t1, t2, t3;
	char fullmode = 0;
	int ddax = 0, dday = 0, rzx = 15, yskip = 0;
	Uint8 *vidram = memory.vid.ram;
	Uint8 penusage;

	for (count = 0; count < 0x300; count += 2) {
		t3 = READ_WORD(&vidram[0x10000 + count]);
		t1 = READ_WORD(&vidram[0x10400 + count]);
		t2 = READ_WORD(&vidram[0x10800 + count]);

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
			//dda_x_skip_i = ddaxskip_i[zx];
			rzx = zx + 1;

		} else {
			rzx = 16;
		}

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
				for (i = 0; i < 16; i++) {
					dday -= (rzy + 1);
					if (dday <= 0) {
						dday += 256;
						yskip++;
					}
				}
			}

			if (sx >= -16 && sx <= 336 && sy >= 0 && sy <= 240) {
				if (PEN_USAGE(tileno) != TILE_INVISIBLE) {	
					uint32_t color = tileatr >> 8;
					uint32_t *palbase = (uint32_t*)&current_pc_pal[16 * color];
					uint32_t *gfxdata = (uint32_t*)&memory.rom.tiles.p[(tileno % memory.nb_of_tiles)<<7];
					uint16_t scaley = ddaxskip_i[yskip];
					const int pitch = PITCH / 2;
	
					limit = &line_limit[sy];
	
					if (rzx==16) {
						if (tileatr & 0x01) {
							if (tileatr & 0x02)
								draw_tile_m68k_xyflip_norm(palbase, (unsigned short*) bufferpixels + (sy) * pitch + sx, gfxdata, scaley);
							else
								draw_tile_m68k_xflip_norm(palbase, (unsigned short*) bufferpixels + (sy) * pitch + sx, gfxdata, scaley);
		  
						} else {
							if (tileatr & 0x02)
								draw_tile_m68k_yflip_norm(palbase, (unsigned short*) bufferpixels + (sy) * pitch + sx, gfxdata, scaley); 
							else			
								draw_tile_m68k_norm(palbase, (unsigned short*) bufferpixels + (sy) * pitch + sx, gfxdata, scaley);
						}
					} else {
						//dda_x_skip_i = ddaxskip_i[zx];
						scalex = ddaxskip_i[rzx];
		
						if (!(tileatr & 0x01)) {
							if (!(tileatr & 0x02)) {
								draw_tile_m68k_xzoom(palbase, (unsigned short*) bufferpixels + (sy) * pitch + sx, gfxdata, scaley);
							} else {
								draw_tile_m68k_xzoomY(palbase, (unsigned short*) bufferpixels + (sy) * pitch + sx, gfxdata, scaley);
							}
						} else {
							if (!(tileatr & 0x02)) {
								draw_tile_m68k_xzoomX(palbase, (unsigned short*) bufferpixels + (sy) * pitch + sx, gfxdata, scaley);
							} else {
								draw_tile_m68k_xzoomXY(palbase, (unsigned short*) bufferpixels + (sy) * pitch + sx, gfxdata, scaley);
							}
						}
					}	
				}
			}

			sy += yskip;
		} /* for y */
	} /* for count */
}
