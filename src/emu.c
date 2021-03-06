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
#include <string.h>
#include <stdlib.h>
#include <time.h>


#include "emu.h"
#include "conf.h"
#include "memory.h"
// #include "frame_skip.h"
#include "pd4990a.h"
#include "messages.h"
#include "profiler.h"
#include "debug.h"
#include "emu.h"

#include "timer.h"
//#include "streams.h"
#include "2610intf.h"
#include "sound.h"
#include "screen.h"
#include "neocrypt.h"

//#include "menu.h"
#include "event.h"

#include <devices/timer.h>
#include <proto/exec.h>


int frame;
const int nb_interlace = 16;
int current_line;
static int arcade;

extern int irq2enable, irq2start, irq2repeat, irq2control, irq2taken;
extern int lastirq2line;
extern int irq2repeat_limit;
extern uint32_t irq2pos_value;
extern uint32_t getMicroseconds();

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
		uint8_t *RAM = memory.rom.cpu_m68k.p;
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
	debug("cpu_z80_init()\n");
	cpu_z80_init();

	debug("YM2610_sh_start()\n");		
	YM2610_sh_start();

	if (arg[OPTION_SAMPLERATE]) {
		init_audio();
		pause_audio(0);
	}
}

void init_neo(void) {
	neogeo_init_save_state();

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
static uint32_t frame_count = 0;

static double startBenchTime;
static double timerTemp, timerTemp2;
static double timerVideo;
double timerSound;
static double timer68k;
static double timerZ80;

static inline int neo_interrupt(int skip_this_frame) {
    static int frames;

	
	pd4990a_addretrace();
	// debug("neogeo_frame_counter_speed %d\n",neogeo_frame_counter_speed);
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

double tento(double value, int exp) {
	while(exp-->0) value *= 10.0;
	return value;
}

void dtostr(char *out, int precision, double value) {
	char buffer[32], *b = buffer;
	uint32_t i;

	if(value < 0) { *out++ = '-'; value = -value; }
	if(precision) {
		i = (uint32_t)(tento(value, precision) + 0.5);
		while(precision-->0) { *b++ = '0' + (i % 10); i /= 10; }
		*b++ = '.';
	} else {
		i = (uint32_t)(value + 0.5);
	}
	do *b++ = '0' + (i % 10), i /= 10; while (i);
	do *out++ = *--b; while (b > buffer);
	*out = 0;
}

void dumpStats(void) {
	double seconds = timer_get_time() - startBenchTime;
	double leftover = seconds - timerVideo - timer68k - timerZ80 - timerSound;
	uint32_t frames = frame_count;//arg[OPTION_BENCH] - bench;
	uint32_t fps = (double)frames / seconds;
	char buffer[32];

	if(leftover < 0) leftover = 0;
	
	dtostr(buffer, 3, seconds);
	debug("%d frames completed in %s s; %d fps\n",  frames, buffer, fps);
	if(arg[OPTION_BENCH]) {
		dtostr(buffer, 2, timerVideo);
		debug("Video %s s, %d%%\n", buffer, (int)((timerVideo * 100.0) / seconds));
		dtostr(buffer, 2, timerSound);
		debug("Sound %s s, %d%%\n", buffer, (int)((timerSound * 100.0) / seconds));
		dtostr(buffer, 2, timer68k);
		debug("CPU 68K %s s, %d%%\n", buffer, (int)((timer68k * 100.0) / seconds));
		dtostr(buffer, 2, timerZ80);
		debug("CPU Z80 %s s, %d%%\n", buffer, (int)((timerZ80 * 100.0) / seconds));
		dtostr(buffer, 2, leftover);
		debug("Overhead %s s, %d%%\n", buffer, (int)((leftover * 100.0) / seconds));
	}
}

static uint16_t pending_save_state = 0, pending_load_state = 0;
static int slow_motion = 0;

// extern uint8_t *lHBuffer, *rHBuffer;
// extern uint8_t *lLBuffer, *rLBuffer;
// extern uint8_t *_lHBuffer, *_rHBuffer;
// extern uint8_t *_lLBuffer, *_rLBuffer;

static inline void state_handling(int save,int load) {
	if (save) {
		save_state(arg[OPTION_FILE], save - 1);
		//reset_frame_skip();
	}
	if (load) {
		load_state(arg[OPTION_FILE], load - 1);
		//reset_frame_skip();
	}
	pending_load_state = pending_save_state = 0;
}



void main_loop(void) {
	int a,i;
 
 	//extern volatile uint8_t updateSound;
 	//uint32_t _updateSound;
 
 	// 10 MHZ 68000
	uint32_t cpu_68k_timeslice;// = 200000;
	//uint32_t cpu_68k_timeslice_scanline;// = cpu_68k_timeslice / nb_interlace;
	//uint32_t cpu_68k_timeslice_rem;// = cpu_68k_timeslice - (cpu_68k_timeslice_scanline) * nb_interlace;

	uint32_t cpu_z80_timeslice;// = 73333;
	uint32_t cpu_z80_timeslice_scanline;// = cpu_z80_timeslice / nb_interlace;
	uint32_t cpu_z80_timeslice_rem;// = cpu_z80_timeslice - (cpu_z80_timeslice_scanline) * nb_interlace;

	double m68k_ratio = arg[OPTION_M68K] / 100.0;
	double z80_ratio = 1.0;//arg[OPTION_Z80] / 100.0f;

	uint32_t tm_cycle = 0;
	uint32_t display_hz = 0;
	uint32_t frameskip = 0;

	if((arg[OPTION_REGION] == CTY_USA) || (arg[OPTION_REGION] == CTY_JAPAN)) 
		display_hz = 60;
	else
		display_hz = 50;

	cpu_68k_timeslice = 10000000 / display_hz;
	cpu_68k_timeslice *= m68k_ratio; 
	//cpu_68k_timeslice_scanline = cpu_68k_timeslice;// / nb_interlace;
	//cpu_68k_timeslice_rem = cpu_68k_timeslice - (cpu_68k_timeslice_scanline) * nb_interlace;

	cpu_z80_timeslice = 4000000 / display_hz;
	cpu_z80_timeslice *= z80_ratio; 
	cpu_z80_timeslice_scanline = cpu_z80_timeslice / nb_interlace;
	cpu_z80_timeslice_rem = cpu_z80_timeslice - (cpu_z80_timeslice_scanline) * nb_interlace;
	if(!cpu_z80_timeslice_rem) cpu_z80_timeslice_rem = cpu_z80_timeslice_scanline;

	timer_run();
	
	startBenchTime = timer_get_time();
	if(arg[OPTION_BENCH]) timerTemp = startBenchTime;
	bench = arg[OPTION_BENCH];

	atexit(dumpStats);

	debug("Starting main loop, CPU at %d%%\n", arg[OPTION_M68K]);
	debug("Display frequency is %dHz\n", display_hz);
	
	//draw_message("Starting game %s", arg[OPTION_FILE]);
	while (true) {
		handle_event();

		if(!paused) {
			tm_cycle = cpu_68k_run(cpu_68k_timeslice - tm_cycle);
			if(arg[OPTION_BENCH]) {
				timerTemp2 = timer_get_time();
				timer68k += timerTemp2 - timerTemp;
				timerTemp = timerTemp2;
			}
		
			handle_event();

			for(i=0; i<nb_interlace; i++) {
				cpu_z80_run(cpu_z80_timeslice_scanline);
//				handle_event();
				timer_run();
			}
			if(arg[OPTION_BENCH]) {
				timerTemp2 = timer_get_time();
				timerZ80 += timerTemp2 - timerTemp;
				timerTemp = timerTemp2;
			}
		}
		
		handle_event();

		if(arg[OPTION_FRAMESKIP]) frameskip = !frameskip;
		if ((a = neo_interrupt(frameskip))) cpu_68k_interrupt(a);
		if(arg[OPTION_BENCH]) {
			timerTemp2 = timer_get_time();
			timerVideo += timerTemp2 - timerTemp;
			timerTemp = timerTemp2;
		}
		frame_count++;

	}
}



