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
	} packed;
	uint32_t pixels;
} packpix_t; 

extern int AC68080;
extern int neogeo_fix_bank_type;

// 	movel sp@(8),d1
// 	movel sp@(12),d0
// 	movel sp@(16),a1
#define REG(x,y) register y __asm(#x)

#define REPEAT2(X) X X
#define REPEAT4(X) REPEAT2(X) REPEAT2(X)
#define REPEAT8(X) REPEAT4(X) REPEAT4(X)

INLINE static void draw_fix_char_ammx(uint32_t tile, uint32_t color, uint16_t *screen) {
	static uint32_t last_color = -1ul;
	uint32_t *gfxdata = (uint32_t*)&current_fix[tile << 5];	
	//int16_t rept = 8;

	// Load palette contents into E8-E23
	if(color != last_color) {
		uint32_t *palette = (uint32_t *)&current_pc_pal[color * 16];
		last_color = color;
		__asm__ volatile (
			"dc.w    0x7102\n" // E8...
			"move.l  (%0)+,d0\n"
			"dc.w    0x7102\n"
			"move.l  (%0)+,d1\n"
			"dc.w    0x7102\n"
			"move.l  (%0)+,d2\n"
			"dc.w    0x7102\n"
			"move.l  (%0)+,d3\n"
			"dc.w    0x7102\n"
			"move.l  (%0)+,d4\n"
			"dc.w    0x7102\n"
			"move.l  (%0)+,d5\n"
			"dc.w    0x7102 \n"
			"move.l  (%0)+,d6 \n"
			"dc.w    0x7102 \n"
			"move.l  (%0)+,d7 \n"
			"dc.w    0x7103 \n" // E16...
			"move.l  (%0)+,d0 \n"
			"dc.w    0x7103 \n"
			"move.l  (%0)+,d1 \n"
			"dc.w    0x7103 \n"
			"move.l  (%0)+,d2 \n"
			"dc.w    0x7103 \n"
			"move.l  (%0)+,d3 \n"
			"dc.w    0x7103 \n"
			"move.l  (%0)+,d4 \n"
			"dc.w    0x7103 \n"
			"move.l  (%0)+,d5 \n"
			"dc.w    0x7103 \n"
			"move.l  (%0)+,d6 \n"
			"dc.w    0x7103 \n"
			"move.l  (%0)+,d7 \n"
			: "+a"(palette)
		);
	}

	// Awkward...
	//__asm__ volatile ("lea (%0),A1" :: "a"(screen));
	//do {
	__asm__ volatile (
		"lea (%1),A1\n"
//		"moveq #7,d2\n"

		"move.w (%0)+,D1 \n" 
		"move.w (%0)+,D0 \n" 
		
		REPEAT8(
		// TRANSi takes 8, 4-bit values from source and uses
		// words stored in E8 thru E23 to write the dest
		// since this needs 128-bit, this uses a register pair
		"dc.w 	0xfe01,0x1A03 \n" // TRANSi-LO D1, E2:E3
		"move.w (%0)+,D1 \n" 

		"dc.w 	0xfe00,0x1803 \n" // TRANSi-LO D0, E0:E1
		"move.w (%0)+,D0 \n" 

		// VPERM to shuffle the colours to match the fix layer format
		"dc.w 	0xfe3F,0xBB0B,0x6745,0x2301 \n" // VPERM #IMMD,E3,E3,E1
		"adda.w #768,A1 \n"  // advance pointer further

		"dc.w 	0xfe3F,0x9909,0x6745,0x2301 \n" // VPERM #IMMD,E1,E1,E1

		// STOREM3 will conditionally store each word
		"dc.w 	0xFE29,0xBB26,0xFD08 \n" // STOREM3.W E3,E3,-760(A1)
		"dc.w 	0xFE29,0x9926,0xFD00 \n" // STOREM3.W E1,E1,-768(A1)
		)
		//".endr \n"

		//"dbra d2,.loop\n"

		: "+a"(gfxdata)
		: "a"(screen)
		: "d0","d1","a0","a1"
	);
	//} while(--rept);

}

#define DO_NOT_REORDER() asm volatile("": : :"memory")

INLINE static void draw_fix_char_m68k(uint32_t tile, uint32_t color, uint16_t *screen) {
	uint32_t *palbase = (uint32_t *)&current_pc_pal[color * 16];
    uint32_t *gfxdata = (uint32_t*)&current_fix[tile << 5];	
	packpix_t pixels0;
	uint16_t pixel1,pixel2;
	int32_t y = 8;
	
	while(y--) {
		if((pixels0.pixels = *gfxdata++)) {            pixel1 = pixels0.packed.p7; DO_NOT_REORDER();
													   pixel2 = pixels0.packed.p6; DO_NOT_REORDER();
			if(pixel1) screen[0x0] = palbase[pixel1];  pixel1 = pixels0.packed.p5;
			if(pixel2) screen[0x1] = palbase[pixel2];  pixel2 = pixels0.packed.p4;
			if(pixel1) screen[0x2] = palbase[pixel1];  pixel1 = pixels0.packed.p3;
			if(pixel2) screen[0x3] = palbase[pixel2];  pixel2 = pixels0.packed.p2;
			if(pixel1) screen[0x4] = palbase[pixel1];  pixel1 = pixels0.packed.p1;
			if(pixel2) screen[0x5] = palbase[pixel2];  pixel2 = pixels0.packed.p0;
			if(pixel1) screen[0x6] = palbase[pixel1];
			if(pixel2) screen[0x7] = palbase[pixel2];
		}		
		screen += PITCH / 2;
	}
}

#define fix_add(x, y) ((((READ_WORD(memory.vid.ram + 0xEA00 + (((y-1)&31)*2 + 64 * (x/6))) >> (5-(x%6))*2) & 3) ^ 3))

void draw_fix_char(unsigned char *buf, int start, int end) {
	unsigned int *gfxdata, myword;
	int x, y, yy;
	unsigned char col;
	unsigned short *br;
	unsigned int *paldata;
	unsigned int byte1, byte2;
	int banked, garouoffsets[32];
	Rect clip;
	const int ystart = 2, yend = 32;
	int p;

	banked = (current_fix == memory.rom.game_sfix.p && neogeo_fix_bank_type && memory.rom.game_sfix.size > 0x1000) ? 1 : 0;

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

	if(AC68080) {	
		for (y = ystart; y < yend; y++) {
			for (x = 0; x < 40; x++) {
				byte1 = (READ_WORD(&memory.vid.ram[0xE000 + ((y + (x << 5)) << 1)]));
				byte2 = byte1 >> 12; byte1 = byte1 & 0xfff;
		
				if (banked) {
					/* Garou, MSlug 3 */
					if(neogeo_fix_bank_type == 1)
						byte1 += 0x1000 * (garouoffsets[(y - 2)&31] ^ 3);
			
					else if(neogeo_fix_bank_type == 2)
						byte1 += 0x1000 * fix_add(x, y);
				}

				if ((byte1 >= (memory.rom.game_sfix.size >> 5)) || (fix_usage[byte1] == 0x00)) continue;
		
				br = (unsigned short*)buf + ((y << 3)) * (PITCH >> 1) + (x << 3);// + 16;
				draw_fix_char_ammx(byte1, byte2, br);
			}
		}
	} else {
		for (y = ystart; y < yend; y++) {
			for (x = 0; x < 40; x++) {
				byte1 = (READ_WORD(&memory.vid.ram[0xE000 + ((y + (x << 5)) << 1)]));
				byte2 = byte1 >> 12; byte1 = byte1 & 0xfff;
		
				if (banked) {
					/* Garou, MSlug 3 */
					if(neogeo_fix_bank_type == 1)
						byte1 += 0x1000 * (garouoffsets[(y - 2)&31] ^ 3);
			
					else if(neogeo_fix_bank_type == 2)
						byte1 += 0x1000 * fix_add(x, y);
				}

				if ((byte1 >= (memory.rom.game_sfix.size >> 5)) || (fix_usage[byte1] == 0x00)) continue;
		
				br = (unsigned short*)buf + ((y << 3)) * (PITCH >> 1) + (x << 3);// + 16;
				draw_fix_char_m68k(byte1, byte2, br);
			}
		}
	}
	
//	if (start != 0 && end != 0) SDL_SetClipRect(buffer, NULL);
}
