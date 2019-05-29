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
#include <time.h>

#include "emu.h"
#include "conf.h"
#include "memory.h"
#include "frame_skip.h"
#include "pd4990a.h"
#include "messages.h"
#include "profiler.h"
#include "debug.h"
#include "emu.h"

#include "timer.h"
//#include "streams.h"
#include "ym2610/2610intf.h"
#include "sound.h"
#include "screen.h"
#include "neocrypt.h"

#ifdef FULL_GL
#include "videogl.h"
#endif
#ifdef GP2X
#include "gp2x.h"

#include "ym2610-940/940shared.h"
#include "ym2610-940/940private.h"
#endif
#include "menu.h"
#include "event.h"

#include <devices/timer.h>
#include <proto/exec.h>


int frame;
const int nb_interlace = 1;
int current_line;
static int arcade;

extern int irq2enable, irq2start, irq2repeat, irq2control, irq2taken;
extern int lastirq2line;
extern int irq2repeat_limit;
extern Uint32 irq2pos_value;
extern ULONG getMicroseconds();

void setup_misc_patch(char *name) {


	if (!strcmp(name, "ssideki")) {
		WRITE_WORD_ROM(&memory.rom.cpu_m68k.p[0x2240], 0x4e71);
	}

	//if (!strcmp(name, "fatfury3")) {
	//	WRITE_WORD_ROM(memory.rom.cpu_m68k.p, 0x0010);
	//}

	if (!strcmp(name, "mslugx")) {
		/* patch out protection checks */
		int i;
		Uint8 *RAM = memory.rom.cpu_m68k.p;
		for (i = 0; i < memory.rom.cpu_m68k.size; i += 2) {
			if ((READ_WORD_ROM(&RAM[i + 0]) == 0x0243)
					&& (READ_WORD_ROM(&RAM[i + 2]) == 0x0001) && /* andi.w  #$1, D3 */
			(READ_WORD_ROM(&RAM[i + 4]) == 0x6600)) { /* bne xxxx */

				WRITE_WORD_ROM(&RAM[i + 4], 0x4e71);
				WRITE_WORD_ROM(&RAM[i + 6], 0x4e71);
			}
		}

		WRITE_WORD_ROM(&RAM[0x3bdc], 0x4e71);
		WRITE_WORD_ROM(&RAM[0x3bde], 0x4e71);
		WRITE_WORD_ROM(&RAM[0x3be0], 0x4e71);
		WRITE_WORD_ROM(&RAM[0x3c0c], 0x4e71);
		WRITE_WORD_ROM(&RAM[0x3c0e], 0x4e71);
		WRITE_WORD_ROM(&RAM[0x3c10], 0x4e71);

		WRITE_WORD_ROM(&RAM[0x3c36], 0x4e71);
		WRITE_WORD_ROM(&RAM[0x3c38], 0x4e71);
	}

}

void neogeo_reset(void) {
    //	memory.vid.modulo = 1; /* TODO: Move to init_video */
	sram_lock = 0;
	sound_code = 0;
	pending_command = 0;
	result_code = 0;

	if (memory.rom.cpu_m68k.size > 0x100000)
		cpu_68k_bankswitch(0x100000);
	else
		cpu_68k_bankswitch(0);
	cpu_68k_reset();

}

void init_sound(void) {
	printf("cpu_z80_init()\n");
	cpu_z80_init();

	printf("YM2610_sh_start()\n");		
	YM2610_sh_start();

	if (arg[OPTION_SAMPLERATE]) {
		init_audio();
		pause_audio(0);
	}
}

void init_neo(void) {
#ifdef ENABLE_940T
	int z80_overclk = CF_VAL(cf_get_item_by_name("z80clock"));
#endif

	neogeo_init_save_state();

#ifdef GP2X
	gp2x_ram_ptr_reset();
#endif

	cpu_68k_init();
//	neogeo_reset();
	pd4990a_init();
//	setup_misc_patch(rom_name);

	init_sound();

	neogeo_reset();
}

static void take_screenshot(void) {
 
}

int paused = 0;

static int fc;
static int last_line;
//static int skip_this_frame = 0;
static int bench = 0;
static ULONG frame_count = 0;

static ULONG startBenchTime;
static ULONG timerTemp;
static ULONG timerVideo;
 ULONG timerSound;
static ULONG timer68k;
static ULONG timerZ80;

static inline int neo_interrupt(int skip_this_frame) {
    static int frames;

	
	pd4990a_addretrace();
	// printf("neogeo_frame_counter_speed %d\n",neogeo_frame_counter_speed);
	if (!(memory.vid.irq2control & 0x8)) {
		if (fc >= neogeo_frame_counter_speed) {
			fc = 0;
			neogeo_frame_counter++;
		}
		fc++;
	}
 
//	skip_this_frame = skip_next_frame;
//	skip_next_frame = frame_skip(0);

	if (!skip_this_frame) {
		draw_screen();
	}
	if(bench) {
		if(--bench == 0) exit(0);
	}
	return 1;
}

void dumpStats(void) {
	ULONG ms = (ULONG)((int)getMilliseconds() - (int)startBenchTime);
	ULONG round = (ms >> 1);
	ULONG leftover = ms - timerVideo - timerSound - timer68k - timerZ80;
	ULONG frames = frame_count;//arg[OPTION_BENCH] - bench;
	ULONG fps = (frames * 1000 + (ms / 2)) / ms;

	if(leftover < 0) leftover = 0;
	
	printf("%d frames completed in %d ms; %d fps\n",  frames, ms, fps);
	if(arg[OPTION_BENCH]) {
		printf("Video %d ms, %d%%\n", timerVideo, (timerVideo * 100 + round) / ms);
		printf("Sound %d ms, %d%%\n", timerSound, (timerSound * 100 + round) / ms);
		printf("CPU 68K %d ms, %d%%\n", timer68k, (timer68k * 100 + round) / ms);
		printf("CPU Z80 %d ms, %d%%\n", timerZ80, (timerZ80 * 100 + round) / ms);
		printf("Overhead %d ms, %d%%\n", leftover, (leftover * 100 + round) / ms);
	}
}

static Uint16 pending_save_state = 0, pending_load_state = 0;
static int slow_motion = 0;

extern uint8_t *lHBuffer, *rHBuffer;
extern uint8_t *lLBuffer, *rLBuffer;
extern uint8_t *_lHBuffer, *_rHBuffer;
extern uint8_t *_lLBuffer, *_rLBuffer;

static inline void state_handling(int save,int load) {
	if (save) {
		save_state(arg[OPTION_FILE], save - 1);
		reset_frame_skip();
	}
	if (load) {
		load_state(arg[OPTION_FILE], load - 1);
		reset_frame_skip();
	}
	pending_load_state = pending_save_state = 0;
}

void main_loop(void) {
	int a,i;
 
 	extern volatile uint8_t updateSound;
 	uint32_t _updateSound;
 
 	// 10 MHZ 68000
	Uint32 cpu_68k_timeslice;// = 200000;
	//Uint32 cpu_68k_timeslice_scanline;// = cpu_68k_timeslice / nb_interlace;
	//Uint32 cpu_68k_timeslice_rem;// = cpu_68k_timeslice - (cpu_68k_timeslice_scanline) * nb_interlace;

	Uint32 cpu_z80_timeslice;// = 73333;
	//Uint32 cpu_z80_timeslice_scanline;// = cpu_z80_timeslice / nb_interlace;
	//Uint32 cpu_z80_timeslice_rem;// = cpu_z80_timeslice - (cpu_z80_timeslice_scanline) * nb_interlace;

	const float m68k_ratio = arg[OPTION_M68K] / 100.0f;
	const float z80_ratio = 1.0f;//arg[OPTION_Z80] / 100.0f;

	Uint32 tm_cycle = 0;
	Uint32 display_hz = 0;
	Uint32 frameskip = 0;

	if((arg[OPTION_REGION] == CTY_USA) || (arg[OPTION_REGION] == CTY_JAPAN)) 
		display_hz = 60;
	else
		display_hz = 50;

	cpu_68k_timeslice = 9000000 / display_hz;
	cpu_68k_timeslice *= m68k_ratio; 
	//cpu_68k_timeslice_scanline = cpu_68k_timeslice;// / nb_interlace;
	//cpu_68k_timeslice_rem = cpu_68k_timeslice - (cpu_68k_timeslice_scanline) * nb_interlace;

	cpu_z80_timeslice = 4000000 / display_hz;
	cpu_z80_timeslice *= z80_ratio; 
	//cpu_z80_timeslice_scanline = cpu_z80_timeslice / nb_interlace;
	//cpu_z80_timeslice_rem = cpu_z80_timeslice - (cpu_z80_timeslice_scanline) * nb_interlace;
	
	//reset_frame_skip();
	my_timer();
	
	printf("Starting main loop, CPU at %d%%\n", arg[OPTION_M68K]);
	startBenchTime = getMilliseconds();
	//if((bench = arg[OPTION_BENCH])) nb_interlace = 8;
	atexit(dumpStats);

	while (true) {
		handle_event();

		if(!paused) {
			if(arg[OPTION_BENCH]) timerTemp = getMilliseconds();
			tm_cycle = cpu_68k_run(cpu_68k_timeslice - tm_cycle);
			if(arg[OPTION_BENCH]) timer68k += (ULONG)((int)getMilliseconds() - (int)timerTemp);
		
			my_timer();
		}
		
		if(arg[OPTION_BENCH]) timerTemp = getMilliseconds();
		
		frameskip += arg[OPTION_FRAMESKIP];
		if ((a = neo_interrupt(frameskip > 100))) cpu_68k_interrupt(a);
		if(frameskip > 100) frameskip -= 100;
		
		if(arg[OPTION_BENCH]) timerVideo += (ULONG)((int)getMilliseconds() - (int)timerTemp);
		frame_count++;

	}
}



