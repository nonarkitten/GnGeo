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
#include "../conf.h"
#include "../screen.h"
#include "../video.h"
#include "../effect.h"

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


BYTE        *bufferpixels = NULL;
BYTE        *videoBuffer = NULL;
static BOOL initialized = FALSE;
static UWORD emptypointer[] = {
    0x0000, 0x0000,    /* reserved, must be NULL */
    0x0000, 0x0000,     /* 1 row of image data */
    0x0000, 0x0000    /* reserved, must be NULL */
};

struct Library *CyberGfxBase=0;

/** Have we already done the init */
static int firsttime=1;

ULONG pixFormat;

/** Hardware window */
struct Window *_hardwareWindow;
/** Hardware screen */
struct Screen *_hardwareScreen;
// Hardware double buffering.
struct ScreenBuffer *_hardwareScreenBuffer[2];

static BYTE _currentScreenBuffer;
static struct MsgPort *_dispPort;
static struct MsgPort *_safePort;

static void *AllocAligned(ULONG size,ULONG chunk) {
	void	*data;

	if (data=AllocMem(size+chunk,MEMF_PUBLIC)) {
		Forbid();
		FreeMem(data,size+chunk);
		data=AllocAbs(size,(APTR)((((ULONG)data)+chunk)&(~chunk)));
		Permit();
	}
	return(data);
}
    
static void initAmigaGraphics(void) {	
    if (firsttime) {
		ULONG modeId = INVALID_ID;
		ULONG i, size;

        firsttime = 0;

		if(!CyberGfxBase)
			CyberGfxBase = (struct Library *) OpenLibrary((UBYTE *)"cybergraphics.library",41L);
			
		printf("Opened CyberGraphX library\n");

        modeId = BestCModeIDTags(
			CYBRBIDTG_NominalWidth, 320,
			CYBRBIDTG_NominalHeight, 240,
			CYBRBIDTG_Depth,16,
			TAG_DONE );
			
		printf("ModeID: %08X\n", modeId);

		_hardwareScreen = OpenScreenTags(NULL,
			SA_Depth, 16,
			SA_DisplayID, modeId,
			SA_Width, 320,
			SA_Height, 240,
			SA_Type, CUSTOMSCREEN,
			SA_Overscan, OSCAN_TEXT,
			SA_ShowTitle, FALSE,
			SA_Draggable, FALSE,
			SA_Exclusive, TRUE,
			SA_AutoScroll, FALSE,
			TAG_END);

		_hardwareScreenBuffer[0] = AllocScreenBuffer(_hardwareScreen, NULL, SB_SCREEN_BITMAP);
		_hardwareScreenBuffer[1] = AllocScreenBuffer(_hardwareScreen, NULL, SB_SCREEN_BITMAP);

		printf("Buffer alignments %p, %p\n", 
			_hardwareScreenBuffer[0]->sb_BitMap->Planes[0], 
			_hardwareScreenBuffer[1]->sb_BitMap->Planes[0]);

// 		_dispPort = CreateMsgPort();
// 		_safePort = CreateMsgPort();
//     		
// 		for (i = 0; i < 2; i++) {
// 			_hardwareScreenBuffer[i]->sb_DBufInfo->dbi_DispMessage.mn_ReplyPort = _dispPort;
// 			_hardwareScreenBuffer[i]->sb_DBufInfo->dbi_SafeMessage.mn_ReplyPort = _safePort;
// 		}
		//_safeToWrite = _safeToChange = true;
		_currentScreenBuffer = 1;    		
    		
		_hardwareWindow = OpenWindowTags(NULL,
			WA_Left, 0,
			WA_Top, 0,
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
			WA_IDCMP, IDCMP_RAWKEY|IDCMP_MOUSEBUTTONS|IDCMP_MOUSEMOVE,    
			WA_Flags, WFLG_REPORTMOUSE|WFLG_RMBTRAP,                   		      		 
			TAG_END);
				
		//bufferpixels = _hardwareScreenBuffer[_currentScreenBuffer]->sb_BitMap;
		printf("Opened screen *Handle: %p\n", _hardwareScreen);		
    }
}

static void killAmigaGraphics(void) {
	if(_hardwareWindow) {
		CloseWindow(_hardwareWindow);
		_hardwareWindow = NULL;
	}

	if(_hardwareScreenBuffer[0]) {
		WaitBlit();
		FreeScreenBuffer(_hardwareScreen, _hardwareScreenBuffer[0]);
	}

	if(_hardwareScreenBuffer[1]) {
		WaitBlit();
		FreeScreenBuffer(_hardwareScreen, _hardwareScreenBuffer[1]);
	}
		
    if (_safePort) {
        DeleteMsgPort(_safePort);
        _safePort = NULL;
    }
    
    if (_dispPort) {
        DeleteMsgPort(_dispPort);
        _dispPort = NULL;
    }
    
    if(_hardwareScreen) {
    	CloseScreen(_hardwareScreen);
		_hardwareScreen = NULL;
	}
	
	if(CyberGfxBase) {
		CloseLibrary(CyberGfxBase);
		CyberGfxBase = NULL;
	}
}

int blitter_soft_init() {
    Uint32 width, height;
    
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

    if (neffect!=0) scale =1;
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

UBYTE font[] = {
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
    
				f = &font[(x % 10) * 6]; blitchar(buffer, f, sx, 0); x /= 10; sx -= 6;
				f = &font[10 * 6]; 		 blitchar(buffer, f, sx, 0); 		  sx -= 6;
				f = &font[(x % 10) * 6]; blitchar(buffer, f, sx, 0); x /= 10; sx -= 6;

        if(x) { f = &font[(x % 10) * 6]; blitchar(buffer, f, sx, 0); x /= 10; sx -= 6; }
        if(x) { f = &font[(x % 10) * 6]; blitchar(buffer, f, sx, 0); x /= 10; sx -= 6; }

    }
    start_time = end_time;
}

static APTR lockHandle;	
static ULONG bytesperrow;

int blitter_soft_prerender() {
	// lock bitmap and allow direct rendering
	bufferpixels = NULL;
	lockHandle = LockBitMapTags(
		_hardwareScreenBuffer[_currentScreenBuffer]->sb_BitMap,
		//_hardwareScreen->ViewPort.RasInfo->BitMap,
		LBMI_BASEADDRESS, (ULONG)&bufferpixels,
		TAG_END);
		
	return (lockHandle && bufferpixels);
}

void blitter_soft_update() {
	video_do_fps(bufferpixels,0);
	
	UnLockBitMap( lockHandle );
	lockHandle = NULL;

	if(arg[OPTION_VSYNC]) WaitTOF();

 	ChangeScreenBuffer(_hardwareScreen, _hardwareScreenBuffer[_currentScreenBuffer]);
 	_currentScreenBuffer ^= 1;	
}

void blitter_soft_close() { 
	Forbid();
	killAmigaGraphics();
	Permit();
}

void blitter_soft_fullscreen() { }

