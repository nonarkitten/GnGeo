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

#ifndef _EMU_H_
#define _EMU_H_

#include <config.h>
#include "conf.h"
#include <string.h>

#define debug(...) do { if(arg[OPTION_DEBUG]) printf(__VA_ARGS__); } while(0)
#define error(...) do { printf(__VA_ARGS__); exit(-1); } while(0)

#define Uintptr long

extern int HostCpuClock;
extern int HostPAL;
extern int paused;

typedef struct {
   uint32_t x;
   uint32_t y;
   uint32_t w;
   uint32_t h;
} Rect;

typedef enum SYSTEM {
    SYS_ARCADE=0,
    SYS_HOME,
    SYS_UNIBIOS,
    SYS_MAX
} SYSTEM;

typedef enum COUNTRY {
    CTY_JAPAN=0,
    CTY_EUROPE,
    CTY_USA,
    CTY_ASIA,
    CTY_MAX
} COUNTRY;

//static inline short SwapSHORT(short val) {
//	asm volatile
//	(
//		"ror.w	#8,%0"
//
//		: "=d" (val)
//		: "0" (val)
//		);
//	return val;
//}
//
//static inline long SwapLONG(long val) {
//	asm volatile
//	(
//		"ror.w	#8,%0 \n\t"
//		"swap	%0 \n\t"
//		"ror.w	#8,%0"
//
//		: "=d" (val)
//		: "0" (val)
//		);
//
//	return val;
//}

// struct {
//     char *game;
//     uint16_t x_start;
//     uint16_t y_start;
//     uint16_t res_x;
//     uint16_t res_y;
//     uint16_t sample_rate;
//     uint16_t test_switch;
// 
//     uint8_t sound;
//     uint8_t vsync;
//     uint8_t snd_st_reg_create;
//     uint8_t do_message;
//     uint8_t nb_joy;
//     uint8_t raster;
//     uint8_t debug;
//     uint8_t rom_type;
//     uint8_t special_bios;
//     uint8_t extra_xor;
//     uint8_t pal;
//     uint8_t accurate940;
//     SYSTEM system;
//     COUNTRY country;
// 
//     uint8_t autoframeskip;
//     uint8_t show_fps;
//     uint8_t sleep_idle;
//     uint8_t screen320;
// 
//     char message[128];
//     char fps[4];
// 
//     int *p1_key;
//     int *p2_key;
// 
//    // SDL_Joystick **joy;
//     int *p1_joy;
//     int *p2_joy;
// 
//     int *p1_hotkey0, *p1_hotkey1, *p1_hotkey2, *p1_hotkey3;
//     int *p2_hotkey0, *p2_hotkey1, *p2_hotkey2, *p2_hotkey3;
// 
//     int p1_hotkey[4];
//     int p2_hotkey[4];
// } conf;

enum {
    HOTKEY_MASK_A = 0x1,
    HOTKEY_MASK_B = 0x2,
    HOTKEY_MASK_C = 0x4,
    HOTKEY_MASK_D = 0x8,
};

enum {
    BUT_A = 0,
    BUT_B,
    BUT_C,
    BUT_D,
    BUT_START,
    BUT_COIN,
    KB_UP,
    KB_DOWN,
    KB_LEFT,
    KB_RIGHT,
    BUT_HOTKEY0,
    BUT_HOTKEY1,
    BUT_HOTKEY2,
    BUT_HOTKEY3
};
enum {
    AXE_X = 6,
    AXE_Y,
    AXE_X_DIR,
    AXE_Y_DIR
};

//config conf;

//uint8_t key[SDLK_LAST];

uint8_t key[100];

uint8_t *joy_button[2];
int32_t *joy_axe[2];
uint32_t joy_numaxes[2];

void debug_loop(void);
void main_loop(void);
void init_neo(void);
void cpu_68k_dpg_step(void);
void setup_misc_patch(char *name);
void neogeo_reset(void);



#ifdef ENABLE_PROFILER
#define PROFILER_START profiler_start
#define PROFILER_STOP profiler_stop

#else
#define PROFILER_START(a)
#define PROFILER_STOP(a)
#endif

/* LOG generation */
#define GNGEO_LOG(...)
#define DEBUG_LOG debug
//#define GNGEO_LOG printf
 

#endif
