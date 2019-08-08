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

#include <signal.h>

//#include "SDL.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "ym2610/2610intf.h"
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
#include "effect.h"
#include "gngeo_icon.h"
#include "event.h"
#include "menu.h"
#include "frame_skip.h"

#include <graphics/gfx.h>
#include <graphics/gfxbase.h>

#ifdef USE_GUI
#include "gui_interf.h"
#endif
#ifdef GP2X
#include "gp2x.h"
#include "ym2610-940/940shared.h"
#endif
#ifdef WII
extern bool fatInitDefault(void);
#endif

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
		printf("Screen initialization failed.\n");
		exit(-1);
    }
    calculate_hotkey_bitmasks();   
	init_event();
}

static void cleanup(void) {
	close_audio();
	screen_close();
    close_game();
}

// static void catch_me(int signo) {
// 	printf("Catch a sigsegv\n");
// 	//SDL_Quit();
// 	exit(-1);
// }
static struct GfxBase *GfxBase;

int HostCpuClock = 0;
int HostPAL = 0;
int AC68080 = 0;
int real_AC68080 = 0;

extern void ParseArguments(int argc, char *argv[]);
extern void convert_audio_rom(void);

int main(int argc, char *argv[]) {
    char *rom_name;
    BPTR file_lock;
    int bench = 0;
    ULONG initStart;
    
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

	file_lock = GetProgramDir();
	SetProgramDir(file_lock);
	
	rom_name = arg[OPTION_FILE];
	if (!rom_name) {
		printf("No file specified\n");
		exit(1);
	}

	atexit(cleanup);

    I_InitTimer();
	reset_frame_skip();

	initStart = getMilliseconds();
	if (init_game(rom_name)!=TRUE) {
		printf("Can't init %s...\n",rom_name);
		exit(1);
	}

	//convert_audio_rom();
	init_sdl();
	
	printf("Startup took %u ms, ", (ULONG)((int)getMilliseconds() - (int)initStart));
	main_loop();

    return 0;
}
