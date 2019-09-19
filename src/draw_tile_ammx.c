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

//#undef HONOUR_SPRITE_LIMIT
#define HONOUR_SPRITE_LIMIT

extern const uint16_t ddaxskip_i[17];

extern uint8_t line_limit[256], *limit;
static uint32_t scalex;

static uint32_t* last_palbase = 0;
INLINE void handle_palette(uint32_t* palbase) {
	if(palbase == last_palbase) return;
	last_palbase = palbase;
	__asm__ volatile (
		"\tdc.w    0x7102\n"
		"\tmove.l  (%0)+,d0\n"
		"\tdc.w    0x7102\n"
		"\tmove.l  (%0)+,d1\n"
		"\tdc.w    0x7102\n"
		"\tmove.l  (%0)+,d2\n"
		"\tdc.w    0x7102\n"
		"\tmove.l  (%0)+,d3\n"
		"\tdc.w    0x7102\n"
		"\tmove.l  (%0)+,d4\n"
		"\tdc.w    0x7102\n"
		"\tmove.l  (%0)+,d5\n"
		"\tdc.w    0x7102 \n"
		"\tmove.l  (%0)+,d6 \n"
		"\tdc.w    0x7102 \n"
		"\tmove.l  (%0)+,d7 \n"
		"\tdc.w    0x7103 \n"
		"\tmove.l  (%0)+,d0 \n"
		"\tdc.w    0x7103 \n"
		"\tmove.l  (%0)+,d1 \n"
		"\tdc.w    0x7103 \n"
		"\tmove.l  (%0)+,d2 \n"
		"\tdc.w    0x7103 \n"
		"\tmove.l  (%0)+,d3 \n"
		"\tdc.w    0x7103 \n"
		"\tmove.l  (%0)+,d4 \n"
		"\tdc.w    0x7103 \n"
		"\tmove.l  (%0)+,d5 \n"
		"\tdc.w    0x7103 \n"
		"\tmove.l  (%0)+,d6 \n"
		"\tdc.w    0x7103 \n"
		"\tmove.l  (%0)+,d7 \n"
		: "+a"(palbase)
	);
}

INLINE void draw_tile_m68k_norm (uint16_t*tilepos asm("a2"), uint32_t*gfxdata asm("a3"), uint16_t scaley asm("d5")) {
	int16_t y;
    
	for(y = 16; y >= 0; y--) {
		if(scaley & 0x8000) {
			#ifdef HONOUR_SPRITE_LIMIT
            if(*limit < 96) {
				*limit += 1;
			#endif
 				__asm__ volatile ( "\n"
                    "\tmove.w  (%0),d0 \n" 
                    "\tmove.w  2(%0),d1 \n"
                    "\tmove.w  4(%0),d2 \n" 
                    "\tmove.w  6(%0),d3 \n" 

                    // TRANSi takes 8, 4-bit values from source and uses
                    // words stored in E8 thru E23 to write the dest
                    // since this needs 128-bit, this uses a register pair
                    "\tdc.w 	0xfe00,0x1803 \n" // TRANSi-LO D0, E0:E1
                    "\tdc.w 	0xfe01,0x1A03 \n" // TRANSi-LO D1, E2:E3
                    "\tdc.w 	0xfe02,0x1C03 \n" // TRANSi-LO D2, E4:E5
                    "\tdc.w 	0xfe03,0x1E03 \n" // TRANSi-LO D3, E6:E7

                    // STOREM3 will conditionally store each word
                    "\tdc.w 	0xFE12,0x9926 \n" // STOREM3.W E1,E1,(A2)
                    "\tdc.w 	0xFE2A,0xBB26,0x0008 \n" // STOREM3.W E3,E3,(8,A2)
                    "\tdc.w 	0xFE2A,0xDD26,0x0010 \n" // STOREM3.W E5,E5,(16,A2)
                    "\tdc.w 	0xFE2A,0xFF26,0x0018 \n" // STOREM3.W E7,E7,(24,A2)

                    : "+a"(gfxdata),"+a"(tilepos)
                    :: "d0","d1","d2","d3"
                );			
            #ifdef HONOUR_SPRITE_LIMIT
			}
            limit++;          
			#endif
			tilepos += PITCH / 2;
        }
		scaley <<= 1;
        gfxdata += 2;
	}
}
INLINE void draw_tile_m68k_xflip_norm (uint16_t*tilepos asm("a2"), uint32_t*gfxdata asm("a3"), uint16_t scaley asm("d5")) {
	int16_t y;
    
	for(y = 16; y >= 0; y--) {
		if(scaley & 0x8000) {
			#ifdef HONOUR_SPRITE_LIMIT
            if(*limit < 96) {
				*limit += 1;
			#endif
                __asm__ volatile ( "\n"
                    "\tmove.w  (%0),d3 \n" 
                    "\tmove.w  2(%0),d2 \n"
                    "\tmove.w  4(%0),d1 \n" 
                    "\tmove.w  6(%0),d0 \n" 

                    // TRANSi takes 8, 4-bit values from source and uses
                    // words stored in E8 thru E23 to write the dest
                    // since this needs 128-bit, this uses a register pair
                    "\tdc.w 	0xfe00,0x1803 \n" // TRANSi-LO D0, E0:E1
                    "\tdc.w 	0xfe01,0x1A03 \n" // TRANSi-LO D1, E2:E3
                    "\tdc.w 	0xfe02,0x1C03 \n" // TRANSi-LO D2, E4:E5
                    "\tdc.w 	0xfe03,0x1E03 \n" // TRANSi-LO D3, E6:E7

					// VPERM to shuffle the colours to match the fix layer format
					"dc.w 	0xfe3F,0x9909,0x6745,0x2301 \n" // VPERM #IMMD,E1,E1,E1
					"dc.w 	0xfe3F,0xBB0B,0x6745,0x2301 \n" // VPERM #IMMD,E3,E3,E3
					"dc.w 	0xfe3F,0xDD0D,0x6745,0x2301 \n" // VPERM #IMMD,E5,E5,E5
					"dc.w 	0xfe3F,0xFF0F,0x6745,0x2301 \n" // VPERM #IMMD,E7,E7,E7

                    // STOREM3 will conditionally store each word
                    "\tdc.w 	0xFE12,0x9926 \n" // STOREM3.W E1,E1,(A2)
                    "\tdc.w 	0xFE2A,0xBB26,0x0008 \n" // STOREM3.W E3,E3,(8,A2)
                    "\tdc.w 	0xFE2A,0xDD26,0x0010 \n" // STOREM3.W E5,E5,(16,A2)
                    "\tdc.w 	0xFE2A,0xFF26,0x0018 \n" // STOREM3.W E7,E7,(24,A2)

                    : "+a"(gfxdata),"+a"(tilepos)
                    :: "d0","d1","d2","d3"
                );	
			#ifdef HONOUR_SPRITE_LIMIT		
            }
            limit++;          
			#endif
			tilepos += PITCH / 2;
        }
		scaley <<= 1;
        gfxdata += 2;
	}
}
INLINE void draw_tile_m68k_yflip_norm  (uint16_t*tilepos asm("a2"), uint32_t*gfxdata asm("a3"), uint16_t scaley asm("d5")) { 
	int16_t y;
    
	gfxdata += 30;
	for(y = 16; y >= 0; y--) {
		if(scaley & 0x8000) {
			#ifdef HONOUR_SPRITE_LIMIT
            if(*limit < 96) {
				*limit += 1;
			#endif
                __asm__ volatile ( "\n"
                    "\tmove.w  (%0),d0 \n" 
                    "\tmove.w  2(%0),d1 \n"
                    "\tmove.w  4(%0),d2 \n" 
                    "\tmove.w  6(%0),d3 \n" 

                    // TRANSi takes 8, 4-bit values from source and uses
                    // words stored in E8 thru E23 to write the dest
                    // since this needs 128-bit, this uses a register pair
                    "\tdc.w 	0xfe00,0x1803 \n" // TRANSi-LO D0, E0:E1
                    "\tdc.w 	0xfe01,0x1A03 \n" // TRANSi-LO D1, E2:E3
                    "\tdc.w 	0xfe02,0x1C03 \n" // TRANSi-LO D2, E4:E5
                    "\tdc.w 	0xfe03,0x1E03 \n" // TRANSi-LO D3, E6:E7

                    // STOREM3 will conditionally store each word
                    "\tdc.w 	0xFE12,0x9926 \n" // STOREM3.W E1,E1,(A2)
                    "\tdc.w 	0xFE2A,0xBB26,0x0008 \n" // STOREM3.W E3,E3,(8,A2)
                    "\tdc.w 	0xFE2A,0xDD26,0x0010 \n" // STOREM3.W E5,E5,(16,A2)
                    "\tdc.w 	0xFE2A,0xFF26,0x0018 \n" // STOREM3.W E7,E7,(24,A2)

                    : "+a"(gfxdata),"+a"(tilepos)
                    :: "d0","d1","d2","d3"
                );			
			#ifdef HONOUR_SPRITE_LIMIT		
            }
            limit++;          
			#endif
			tilepos += PITCH / 2;
        }
		scaley <<= 1;
        gfxdata -= 2;
	}	
}
INLINE void draw_tile_m68k_xyflip_norm (uint16_t*tilepos asm("a2"), uint32_t*gfxdata asm("a3"), uint16_t scaley asm("d5")) { 
	int16_t y;
    
	gfxdata += 30;
	for(y = 16; y >= 0; y--) {
		if(scaley & 0x8000) {
			#ifdef HONOUR_SPRITE_LIMIT
            if(*limit < 96) {
				*limit += 1;
			#endif
                __asm__ volatile ( "\n"
                    "\tmove.w  (%0),d3 \n" 
                    "\tmove.w  2(%0),d2 \n"
                    "\tmove.w  4(%0),d1 \n" 
                    "\tmove.w  6(%0),d0 \n" 

                    // TRANSi takes 8, 4-bit values from source and uses
                    // words stored in E8 thru E23 to write the dest
                    // since this needs 128-bit, this uses a register pair
                    "\tdc.w 	0xfe00,0x1803 \n" // TRANSi-LO D0, E0:E1
                    "\tdc.w 	0xfe01,0x1A03 \n" // TRANSi-LO D1, E2:E3
                    "\tdc.w 	0xfe02,0x1C03 \n" // TRANSi-LO D2, E4:E5
                    "\tdc.w 	0xfe03,0x1E03 \n" // TRANSi-LO D3, E6:E7

					// VPERM to shuffle the colours to match the fix layer format
					"dc.w 	0xfe3F,0x9909,0x6745,0x2301 \n" // VPERM #IMMD,E1,E1,E1
					"dc.w 	0xfe3F,0xBB0B,0x6745,0x2301 \n" // VPERM #IMMD,E3,E3,E3
					"dc.w 	0xfe3F,0xDD0D,0x6745,0x2301 \n" // VPERM #IMMD,E5,E5,E5
					"dc.w 	0xfe3F,0xFF0F,0x6745,0x2301 \n" // VPERM #IMMD,E7,E7,E7

                    // STOREM3 will conditionally store each word
                    "\tdc.w 	0xFE12,0x9926 \n" // STOREM3.W E1,E1,(A2)
                    "\tdc.w 	0xFE2A,0xBB26,0x0008 \n" // STOREM3.W E3,E3,(8,A2)
                    "\tdc.w 	0xFE2A,0xDD26,0x0010 \n" // STOREM3.W E5,E5,(16,A2)
                    "\tdc.w 	0xFE2A,0xFF26,0x0018 \n" // STOREM3.W E7,E7,(24,A2)

                    : "+a"(gfxdata),"+a"(tilepos)
                    :: "d0","d1","d2","d3"
                );			
			#ifdef HONOUR_SPRITE_LIMIT		
            }
            limit++;          
			#endif
			tilepos += PITCH / 2;
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

void draw_tiles_ammx(unsigned int tileno,int sx,int sy,int zx,int zy, int color,int xflip,int yflip,unsigned char *bmp) {
	int sx = 0, sy = 0, oy = 0, my = 0, zx = 1, rzy = 1;
	unsigned int offs, i, count, y;
	unsigned int tileno, tileatr, t1, t2, t3;
	char fullmode = 0;
	int ddax = 0, dday = 0, rzx = 15, yskip = 0;
	Uint8 *vidram = memory.vid.ram;
	Uint8 penusage;

	last_palbase = 0;

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
			/*if (tileatr&0x80) debug("PLOP\n");*/
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
						handle_palette(palbase);
						if (tileatr & 0x01) {
							if (tileatr & 0x02)
								draw_tile_m68k_xyflip_norm((unsigned short*) bufferpixels + (sy) * pitch + sx, gfxdata, scaley);
							else
								draw_tile_m68k_xflip_norm((unsigned short*) bufferpixels + (sy) * pitch + sx, gfxdata, scaley);
		  
						} else {
							if (tileatr & 0x02)
								draw_tile_m68k_yflip_norm((unsigned short*) bufferpixels + (sy) * pitch + sx, gfxdata, scaley); 
							else			
								draw_tile_m68k_norm((unsigned short*) bufferpixels + (sy) * pitch + sx, gfxdata, scaley);
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
