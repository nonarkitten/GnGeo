#include <stdio.h>
#include <config.h>

#include <devices/gameport.h>
#include <devices/input.h>
#include <devices/keymap.h>
#include <devices/timer.h>
#include <dos/dos.h>
#include <exec/exec.h>
#include <graphics/gfx.h>
#include <graphics/gfxbase.h>
#include <graphics/gfxmacros.h>
#include <intuition/intuition.h>
#include <libraries/asl.h>
#include <libraries/lowlevel.h>

#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/graphics.h>

#include <cybergraphx/cybergraphics.h>
#include <inline/cybergraphics.h>

#include "../conf.h"
#include "../effect.h"
#include "../emu.h"
#include "../screen.h"
#include "../video.h"

static Rect screen_rect;
static int vsync;

uint8_t *bufferpixels = NULL;
uint8_t *videoBuffer = NULL;
static bool initialized = FALSE;
static uint16_t emptypointer[] = {
    0x0000, 0x0000, /* reserved, must be NULL */
    0x0000, 0x0000, /* 1 row of image data */
    0x0000, 0x0000  /* reserved, must be NULL */
};

extern int AC68080;
static void * lockHandle;
static uint32_t bytesperrow;

struct Library *CyberGfxBase = 0;
//struct IntuitionBase  *IntuitionBase = NULL;

/** Have we already done the init */
static int firsttime = 1;

uint32_t pixFormat;

/** Hardware window */
struct Window *_hardwareWindow;
/** Hardware screen */
struct Screen *_hardwareScreen;
// Hardware double buffering.
struct ScreenBuffer *_hardwareScreenBuffer[2];

static uint8_t _currentScreenBuffer;
static struct BitMap bm[2];

static void initAmigaGraphics(void) {
  if (firsttime) {
    uint32_t modeId = INVALID_ID;
    uint32_t i, size;
    struct BitMap *bitmap;
	extern int AC68080;

    firsttime = 0;

    if (!CyberGfxBase)
      CyberGfxBase =
          (struct Library *)OpenLibrary((Uuint8_t *)"cybergraphics.library", 41L);

    debug("Opened CyberGraphX library\n");

    modeId = BestCModeIDTags(
		CYBRBIDTG_NominalWidth, 320, 
		CYBRBIDTG_NominalHeight, 240, 
		CYBRBIDTG_Depth, 16, TAG_DONE);

    debug("ModeID: %08X\n", modeId);

    _hardwareScreen = OpenScreenTags(
        NULL, SA_Depth, 16, SA_DisplayID, modeId, SA_Width, 384, SA_Height, 256,
        SA_Type, CUSTOMSCREEN, SA_Overscan, OSCAN_TEXT, SA_ShowTitle, FALSE,
        SA_Draggable, FALSE, SA_Exclusive, TRUE, SA_AutoScroll, TRUE, TAG_END);

    _hardwareScreenBuffer[0] = AllocScreenBuffer(_hardwareScreen, NULL, SB_SCREEN_BITMAP);
    _hardwareScreenBuffer[1] = AllocScreenBuffer(_hardwareScreen, NULL, 0);

    debug("Buffer alignments %p, %p\n",
           _hardwareScreenBuffer[0]->sb_BitMap->Planes[0],
           _hardwareScreenBuffer[1]->sb_BitMap->Planes[0]);

    _currentScreenBuffer = 1;

    _hardwareWindow = OpenWindowTags(
        NULL, WA_Left, 0, WA_Top, 0, WA_Width, 320, WA_Height, 240, WA_Title,
        NULL, WA_CustomScreen, (uint32_t)_hardwareScreen, WA_Backdrop, TRUE,
        WA_Borderless, TRUE, WA_DragBar, FALSE, WA_Activate, TRUE,
        WA_SimpleRefresh, TRUE, WA_NoCareRefresh, TRUE, WA_IDCMP,
        IDCMP_RAWKEY | IDCMP_MOUSEBUTTONS | IDCMP_MOUSEMOVE, WA_Flags,
        WFLG_REPORTMOUSE | WFLG_RMBTRAP, TAG_END);

    debug("Opened screen *Handle: %p\n", _hardwareScreen);

    if (AC68080) *(uint16_t_t *)0xDFF1F4 = 0x0703;
  }
}

static void killAmigaGraphics(void) {
  if (_hardwareWindow) {
    CloseWindow(_hardwareWindow);
    _hardwareWindow = NULL;
  }

  if (_hardwareScreenBuffer[0]) {
    WaitBlit();
    FreeScreenBuffer(_hardwareScreen, _hardwareScreenBuffer[0]);
  }

  if (_hardwareScreenBuffer[1]) {
    WaitBlit();
    FreeScreenBuffer(_hardwareScreen, _hardwareScreenBuffer[1]);
  }

  if (_hardwareScreen) {
    CloseScreen(_hardwareScreen);
    _hardwareScreen = NULL;
  }

  if (CyberGfxBase) {
    CloseLibrary(CyberGfxBase);
    CyberGfxBase = NULL;
  }
}

static int blitter_soft_init() {
  uint32_t width, height;

  screen_rect.x = 16;
  screen_rect.y = 16;
  screen_rect.w = 304;
  screen_rect.h = 224;

  visible_area.x = 0;
  visible_area.y = 0;
  visible_area.w = 304;
  visible_area.h = 224;

  width = visible_area.w;
  height = visible_area.h;

  if (vsync) {
    height = 240;
    screen_rect.y = 8;

  } else {
    height = visible_area.h;
    screen_rect.y = 0;
    yscreenpadding = 0;
  }

  screen_rect.w = visible_area.w;
  screen_rect.h = visible_area.h;

  if (neffect != 0) scale = 1;
  if (scale == 1) {
    width *= effect[neffect].x_ratio;
    height *= effect[neffect].y_ratio;
  } else {
    if (scale > 3) scale = 3;
    width *= scale;
    height *= scale;
  }

  initAmigaGraphics();

  return TRUE;
}



static void blitchar(uint16_t *buffer, Uuint8_t *f, uint32_t sx, uint32_t sy) {
  uint32_t x, y, b;
  for (x = 0; x < 6; x++) {
    for (b = 1, y = 0; y < 8; y++) {
      if (*f & b) buffer[sx + x + (sy + y) * 384] = 0xFFFF;  // : 0x0000;
      b <<= 1;
    }
    f++;
  }
}

/**********************************************************************/
extern void dim_screen_m68k(void*);
static void video_dim_screen(uint8_t *buffer) {
	dim_screen_m68k(buffer);
}

static void video_do_fps(void) {
	static uint32_t last_time = 0;
	static uint32_t fps_avg = 60;
	uint32_t time = timer_get_time_ms();
	uint32_t this_fps = 1000 / ((int)time - (int)last_time);
	fps_avg = (fps_avg >> 1) + (this_fps >> 1);
	render_message(fps_avg);
	last_time = time;
}

static int blitter_soft_prerender() {
  bufferpixels = _hardwareScreenBuffer[_currentScreenBuffer]->sb_BitMap->Planes[0];
  return 1;
}

static void blitter_soft_update() {
	if (paused) video_dim_screen(bufferpixels);
	video_do_fps();

	if (ChangeScreenBuffer(_hardwareScreen,
		_hardwareScreenBuffer[_currentScreenBuffer])) {

    _currentScreenBuffer ^= 1;
    if (arg[OPTION_VSYNC]) WaitTOF();
  }

  if (AC68080) *(uint16_t_t *)0xDFF1F4 = 0x0703;
}

static void blitter_soft_close() {
	Forbid();
	killAmigaGraphics();
	Permit();
}

static void blitter_soft_fullscreen() {}




int screen_init() {
	return blitter_soft_init();
}

void screen_change_blitter_and_effect(void) {}

int screen_reinit(void) {
	visible_area.x = 16;
	visible_area.y = 16;
	visible_area.w = 320;
	visible_area.h = 224;

	/* Initialization of some variables */
	return TRUE;
}

int screen_resize(int w, int h) {
	return TRUE;
}

static inline void do_interpolation() {}

int screen_prerender() {
	return blitter_soft_prerender();
}

void screen_update() {
	blitter_soft_update();
}

void screen_close() {
	blitter_soft_close();
}

void screen_fullscreen() {}
