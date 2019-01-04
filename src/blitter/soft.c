#include <stdio.h>
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
 
#include <exec/exec.h>
#include <dos/dos.h>
#include <graphics/gfx.h>
#include <graphics/gfxbase.h>
#include <graphics/gfxmacros.h>
#include <intuition/intuition.h>
#include <libraries/asl.h>
#include <libraries/lowlevel.h>
#include <devices/gameport.h>
#include <devices/timer.h>
#include <devices/keymap.h>
#include <devices/input.h>


#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/graphics.h>
 
 
#include <cybergraphx/cybergraphics.h>
#include <inline/cybergraphics.h>

 
#include "../emu.h"
#include "../screen.h"
#include "../video.h"
#include "../effect.h"
#include "../conf.h"
#ifdef GP2X
#include "../gp2x.h"

#define SC_STATUS     0x1802>>1
#define SC_DISP_FIELD (1<<8)

#endif

#ifdef DEVKIT8000
static Rect screen_rect;
#else
static Rect screen_rect;
#endif
 
static int vsync;

 
BYTE       *bufferpixels = NULL;
uint8_t        *videoBuffer = NULL;
static int initialized = 0;
static UWORD emptypointer[] = {
  0x0000, 0x0000,    /* reserved, must be NULL */
  0x0000, 0x0000,     /* 1 row of image data */
  0x0000, 0x0000    /* reserved, must be NULL */
};

/** Hardware window */
struct Window *_hardwareWindow;
/** Hardware screen */
struct Screen *_hardwareScreen;
 
 
static int    firsttime=1;

extern Uint32 *current_pc_pal;
struct RastPort *theRastPort = NULL;
struct RastPort *theTmpRastPort = NULL;
struct BitMap *theBitMap = NULL;
struct BitMap *theTmpBitMap = NULL;
struct Window *theWindow = NULL;

struct Library *CyberGfxBase=0;

void initAmigaGraphics(void)
{
struct Rectangle rect;


    if (firsttime)
    {
        
  
    uint i = 0;
    ULONG modeId = INVALID_ID;

    if(!CyberGfxBase) CyberGfxBase = (struct Library *) OpenLibrary((UBYTE *)"cybergraphics.library",41L);


    _hardwareWindow = NULL;
 
    _hardwareScreen = NULL;
        firsttime = 0;
        bufferpixels = (unsigned char *)malloc(352*256*2);
 


       modeId = BestCModeIDTags(CYBRBIDTG_NominalWidth, 320,
				      CYBRBIDTG_NominalHeight, 240,
				      CYBRBIDTG_Depth,16,
				      TAG_DONE );


        if(modeId == INVALID_ID) {
          printf("Could not find a valid screen mode");
          exit(-1);
        }

    rect.MinX = 16;
    rect.MinY = 16;
    rect.MaxX = 304;
    rect.MaxY = 224;
    
         _hardwareScreen = OpenScreenTags(NULL,
                         SA_Depth, 16,
                         SA_DisplayID, modeId,
                         SA_Width, 320,
                         SA_Height,240,
                         SA_Type, CUSTOMSCREEN,
                         SA_Overscan, OSCAN_TEXT,
                         SA_Quiet,TRUE,
                         SA_ShowTitle, FALSE,
                         SA_Draggable, FALSE,
                         SA_Exclusive, TRUE,
                         SA_AutoScroll, FALSE,
                          SA_DClip,       (ULONG)&rect,
                         TAG_END);
 

        _hardwareWindow = OpenWindowTags(NULL,
                      	    WA_Left, 16,
                			WA_Top, 16,
                			WA_Width, 320,
                			WA_Height, 240,
                			WA_Title, NULL,
        					SA_AutoScroll, FALSE,
                			WA_CustomScreen, (ULONG)_hardwareScreen,
                			WA_Backdrop, TRUE,
                			WA_Borderless, TRUE,
                			WA_DragBar, FALSE,
                			WA_Activate, TRUE,
                			WA_SimpleRefresh, TRUE,
                			WA_NoCareRefresh, TRUE, 
                            WA_IDCMP,           IDCMP_RAWKEY|IDCMP_MOUSEBUTTONS|IDCMP_MOUSEMOVE,    
                            WA_Flags,           WFLG_REPORTMOUSE|WFLG_RMBTRAP,                   		      		 
                      	    TAG_END);

        
    
        theRastPort=_hardwareWindow->RPort;
        theBitMap=theRastPort->BitMap;
    
        SetPointer (_hardwareWindow, emptypointer, 0, 0, 0, 0);
 
        initialized = 1;
        
        memset (bufferpixels,0,304*224*2);

 
 

    }
}

int
blitter_soft_init()
{
	Uint32 width;
	Uint32 height;
	
    screen_rect.x = 16;
    screen_rect.y = 16;
    screen_rect.w = 304;
    screen_rect.h = 224;

    visible_area.x = 0;
    visible_area.y = 0 ;
    visible_area.w = 304;
    visible_area.h = 224;

	width = visible_area.w;
	height = visible_area.h;


	if (vsync) {
		height=240;
		screen_rect.y = 8;

	} else {
		height=visible_area.h;
		screen_rect.y = 0;
		yscreenpadding=0;
	}

	screen_rect.w=visible_area.w;
	screen_rect.h=visible_area.h;


	if (neffect!=0)	scale =1;
	if (scale == 1) {
	    width *=effect[neffect].x_ratio;
	    height*=effect[neffect].y_ratio;
	} else {
	    if (scale > 3) scale=3;
	    width *=scale;
	    height *=scale;
	}


	initAmigaGraphics();
	
	return TRUE;
}

ULONG eclocks_per_second; /* EClock frequency in Hz */
extern char fps_str[32];

static const UBYTE tiny_font[] = {
	0x7E, // 0b01111110,
	0x81, // 0b10000001,
	0x81, // 0b10000001,
	0x81, // 0b10000001,
	0x7E, // 0b01111110,
	0x00, // 0b00000000,
	
	0x00, // 0b00000000,
	0x82, // 0b10000010,
	0xFF, // 0b11111111,
	0x80, // 0b10000000,
	0x00, // 0b00000000,
	0x00, // 0b00000000,
	
	0xC2, // 0b11000010,
	0xA1, // 0b10100001,
	0x91, // 0b10010001,
	0x89, // 0b10001001,
	0x86, // 0b10000110,
	0x00, // 0b00000000,
	
	0x42, // 0b01000010,
	0x81, // 0b10000001,
	0x89, // 0b10001001,
	0x89, // 0b10001001,
	0x76, // 0b01110110,
	0x00, // 0b00000000,
	
	0x1E, // 0b00011110,
	0x10, // 0b00010000,
	0x10, // 0b00010000,
	0x10, // 0b00010000,
	0xFF, // 0b11111111,
	0x00, // 0b00000000,
	
	0x4F, // 0b01001111,
	0x89, // 0b10001001,
	0x89, // 0b10001001,
	0x89, // 0b10001001,
	0x71, // 0b01110001,
	0x00, // 0b00000000,
	
	0x7E, // 0b01111110,
	0x89, // 0b10001001,
	0x89, // 0b10001001,
	0x89, // 0b10001001,
	0x72, // 0b01110010,
	0x00, // 0b00000000,
	
	0x81, // 0b10000001,
	0x41, // 0b01000001,
	0x21, // 0b00100001,
	0x11, // 0b00010001,
	0x0F, // 0b00001111,
	0x00, // 0b00000000,
	
	0x76, // 0b01110110,
	0x89, // 0b10001001,
	0x89, // 0b10001001,
	0x89, // 0b10001001,
	0x76, // 0b01110110,
	0x00, // 0b00000000,
	
	0x0E, // 0b00001110,
	0xA1, // 0b10010001,
	0xA1, // 0b10010001,
	0xA1, // 0b01010001,
	0x3E, // 0b00111110,
	0x00, // 0b00000000,
	
	0x00, // 0b00000000,
	0x00, // 0b00000000,
	0xC0, // 0b11000000,
	0xC0, // 0b11000000,
	0x00, // 0b00000000,
	0x00, // 0b00000000,

};

static void blitchar(UWORD *buffer, UBYTE *f, ULONG sx, ULONG sy) {
	ULONG x, y, b;
	for(x=0; x<6; x++) {
		for(b=1, y=0; y<8; y++) {
			if(*f & b) buffer[sx + x + (sy + y) * 320] = 0xFFFF;// : 0x0000;
			b <<= 1; 
		}
		f++;
	}
}

/**********************************************************************/
static void video_do_fps (BYTE *buffer, int yoffset) {
	ULONG x, sx;
	static ULONG fps = 600;
	static struct EClockVal start_time = {0, 0};
	struct EClockVal end_time;
	//char msg[6];

	eclocks_per_second = ReadEClock (&end_time);
	x = (int)end_time.ev_lo - (int)start_time.ev_lo;
	
	if (x != 0) {
		//char *p = &msg[5];
		UBYTE *f;
		sx = 100;
		
		//*p-- = 0;
		
		x = (eclocks_per_second * 10 + (x >> 1)) / x;
		fps -= fps / 16; fps += x / 16; // Kalman filter
		x = fps;
	
				f = &tiny_font[(x % 10) * 6]; blitchar(buffer, f, sx, 0); x /= 10; sx -= 6;
				f = &tiny_font[10 * 6]; 	  blitchar(buffer, f, sx, 0); 		   sx -= 6;
				f = &tiny_font[(x % 10) * 6]; blitchar(buffer, f, sx, 0); x /= 10; sx -= 6;

		if(x) { f = &tiny_font[(x % 10) * 6]; blitchar(buffer, f, sx, 0); x /= 10; sx -= 6; }
		if(x) { f = &tiny_font[(x % 10) * 6]; blitchar(buffer, f, sx, 0); x /= 10; sx -= 6; }

	}
	start_time = end_time;
}

 
 
void blitter_soft_update() {    
    ULONG  DestMod = 0;
    ULONG * pDest  = NULL;
    struct TagItem mesTags[] = {{LBMI_BASEADDRESS, (ULONG) &pDest},
       {LBMI_BYTESPERROW, (ULONG) &DestMod},
       {TAG_END}};

    APTR  handle  = LockBitMapTagList( theRastPort->BitMap, &mesTags[0] );
    CopyMemQuick(bufferpixels + 32    ,pDest,  0x26000);
    video_do_fps(theRastPort,0);        
    UnLockBitMap( handle );  
}

    
void blitter_soft_close(void) {
 
}    
void blitter_soft_prerender(void) {
 
}

void blitter_soft_fullscreen(void) {

}
                  
