
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

 
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

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
#include <proto/layers.h>
#include <proto/intuition.h>
#include <proto/asl.h>
#include <proto/keymap.h>
#include <proto/lowlevel.h>

#include "emu.h"
#include "screen.h"
#include "video.h"

#include "blitter.h"
#include "effect.h"


int screen_init() {
	return blitter_soft_init();
}

void screen_change_blitter_and_effect(void) { }

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

static inline void do_interpolation() { }

int screen_prerender() {
	return blitter_soft_prerender();
}

void screen_update() {
	blitter_soft_update();
}

void screen_close() {
	blitter_soft_close();
}

void screen_fullscreen() { }
