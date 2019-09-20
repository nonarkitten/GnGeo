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
#include <signal.h>

//#include "SDL.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "2610intf.h"
#include "font.h"
#include "fileio.h"
#include "video.h"
#include "screen.h"
#include "emu.h"
#include "conf.h"
#include "sound.h"
#include "messages.h"
#include "memory.h"
#include "debug.h"
#include "blitter.h"
// #include "effect.h"
// #include "gngeo_icon.h"
#include "event.h"
#include "menu.h"
// #include "frame_skip.h"

#include <graphics/gfx.h>
#include <graphics/gfxbase.h>
#include <proto/lowlevel.h>

#ifdef __AMIGA__
# include <proto/exec.h>
#endif

void calculate_hotkey_bitmasks()
{
//     int *p;
//     int i, j, mask;
//     const char *p1_key_list[] = { "p1hotkey0", "p1hotkey1", "p1hotkey2", "p1hotkey3" };
//     const char *p2_key_list[] = { "p2hotkey0", "p2hotkey1", "p2hotkey2", "p2hotkey3" };
// 
// 
//     for ( i = 0; i < 4; i++ ) {
// 		p=CF_ARRAY(cf_get_item_by_name(p1_key_list[i]));
// 		for ( mask = 0, j = 0; j < 4; j++ ) mask |= p[j];
// 		conf.p1_hotkey[i] = mask;
//     }
// 
//     for ( i = 0; i < 4; i++ ) {
// 		p=CF_ARRAY(cf_get_item_by_name(p2_key_list[i]));
// 		for ( mask = 0, j = 0; j < 4; j++ ) mask |= p[j];
// 		conf.p2_hotkey[i] = mask;
//     }

}

void sdl_set_title(char *name) { }

void init_sdl(void /*char *rom_name*/) {
    if (screen_init() == FALSE) {
		debug("Screen initialization failed.\n");
		exit(-1);
    }
    //calculate_hotkey_bitmasks();   
	//init_event();
}

static void suspend_os(void) {
	struct TagItem suspend_tags = { SCON_TakeOverSys, 1, TAG_END };
	SystemControlA( &suspend_tags );
}

static void resume_os(void) {
	struct TagItem resume_tags = { SCON_TakeOverSys, 0, TAG_END };
	SystemControlA( &resume_tags );
}

static void cleanup(void) {
	resume_os();
	close_audio();
	screen_close();
    close_game();
	//CloseLibrary("lowlevel.library");
}

// static void catch_me(int signo) {
// 	debug("Catch a sigsegv\n");
// 	//SDL_Quit();
// 	exit(-1);
// }
static struct GfxBase *GfxBase;
extern struct Library *LowLevelBase;

int HostCpuClock = 0;
int HostPAL = 0;
int AC68080 = 0;
int real_AC68080 = 0;

extern void ParseArguments(int argc, char *argv[]);
extern void convert_audio_rom(void);
extern const uint8_t gngeo_logo[];
extern const uint8_t gngeo_logo_clut[];
static void load_logo(void) {
	uint16_t pal[16] = { 0 };
	uint8_t *logo = gngeo_logo;
	uint16_t *pix = bufferpixels;
	int i, x;

	for(i=0;i<16;i++) {
		uint8_t r = gngeo_logo_clut[i * 4 + 0] & 0xF8;
		uint8_t g = gngeo_logo_clut[i * 4 + 1] & 0xFC;
		uint8_t b = gngeo_logo_clut[i * 4 + 2] & 0xF8;
		uint16_t c = (r << 8) | (g << 3) | b;
		pal[i] = c;
	}
	for(i=0;i<200;i++) {
		for(x=0;x<320;x++) {
			*pix++ = pal[*logo++];
		}
		pix += (PITCH >> 1) - 320;
	}
	screen_update();
}
int main(int argc, char *argv[]) {
    char *rom_name;
    BPTR file_lock;
    int bench = 0;
    uint32_t initStart;

    if(!LowLevelBase) LowLevelBase = (struct Library *) OpenLibrary("lowlevel.library",0);
	if(!LowLevelBase) exit(-1);
    
	if(NULL != (GfxBase = (struct GfxBase *)OpenLibrary("graphics.library",0L))) {
		HostCpuClock = (GfxBase->DisplayFlags & PAL) ? 3546895L : 3579545L;
		HostPAL = (GfxBase->DisplayFlags & PAL) ? 1 : 0;
		CloseLibrary( (struct Library *) GfxBase);
	} else {
		return -1;
	}
	AC68080 = !!(SysBase->AttnFlags & (1<<10));
	real_AC68080 = AC68080;
		
	ParseArguments(argc, argv);
	if(!arg[OPTION_DEBUG]) {
		init_sdl();
		load_logo();
	}

	file_lock = GetProgramDir();
	SetProgramDir(file_lock);
	
	rom_name = arg[OPTION_FILE];
	if (!rom_name) {
		error("No file specified\n");
	}

	atexit(cleanup);

	timer_init();
	initStart = timer_get_time_ms();
	if (init_game(rom_name)!=TRUE) {
		error("Can't init %s...\n",rom_name);
	}

	//convert_audio_rom();
	if(arg[OPTION_DEBUG]) init_sdl();
	
	printf("%d\n", __LINE__);
	debug("Startup took %u ms, ", (uint32_t)((int)timer_get_time_ms() - (int)initStart));
	suspend_os();
	main_loop();

    return 0;
}
