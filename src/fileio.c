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
//#include "SDL.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include <zlib.h>
#include <stdbool.h>

#include "unzip.h"
#include "memory.h"
#include "video.h"
#include "emu.h"
#include "conf.h"
#include "fileio.h"
#include "neocrypt.h"
#include "screen.h"
#include "sound.h"
// #include "transpack.h"
//#include "menu.h"
// #include "frame_skip.h"

#if defined (__AMIGA__)
#define ROOTPATH "WinDH_C:CrossCompiler/gngeo/release/"
#else
#define ROOTPATH ""
#endif

//uint8_t *current_buf;
//char *rom_file;

void sdl_set_title(char *name);

void chomp(char *str) {
	int i = 0;
	if (str) {
		while (str[i] != 0) {
			debug(" %d ", str[i]);
			i++;
		}
		debug(" \n");
		if (str[i - 1] == 0x0A || str[i - 1] == 0x0D) str[i - 1] = 0;
		if (str[i - 2] == 0x0A || str[i - 2] == 0x0D) str[i - 2] = 0;

	}
}


char *file_basename(char *filename) {
	char *t;
	t = strrchr(filename, '/');
	if (t) return t + 1;
	return filename;
}

/* check if dir_name exist. Create it if not */
int check_dir(char *dir_name) {
	DIR *d;

	if (!(d = opendir(dir_name)) && (errno == ENOENT)) {
#ifdef WIN32
		mkdir(dir_name);
#else
		mkdir(dir_name, 0755);
#endif
		return false;
	}
	return true;
}

/* return a char* to $HOME/.gngeo/ 
   DO NOT free it!
 */
#ifdef EMBEDDED_FS

char *get_gngeo_dir(void) {
	static char *filename = ROOTPATH"";
	return filename;
}
#else

char *get_gngeo_dir(void) {
	static char *filename = NULL;
#if defined (__AMIGA__)
	int len = strlen("data/") + 1;
#else
	int len = strlen(getenv("HOME")) + strlen("/.gngeo/") + 1;
#endif
	if (!filename) {
		filename = (char*)malloc(len * sizeof (char));
		CHECK_ALLOC(filename);
#if defined (__AMIGA__)
		sprintf(filename, "data/");
#else
		sprintf(filename, "%s/.gngeo/", getenv("HOME"));
#endif
	}
	check_dir(filename);
	//debug("get_gngeo_dir %s\n",filename);
	return filename;
}
#endif

void open_nvram(char *name) {
	char *filename;
	size_t totread = 0;
	uint32_t checksum = 0;
	int i;
#ifdef EMBEDDED_FS
	const char *gngeo_dir = ROOTPATH"save/";
#elif defined(__AMIGA__)
	const char *gngeo_dir = "save/";
#else
	const char *gngeo_dir = get_gngeo_dir();
#endif
	FILE *f;
	int len = strlen(name) + strlen(gngeo_dir) + 4; /* ".nv\0" => 4 */

	filename = (char *) alloca(len);
	sprintf(filename, "%s%s.nv", gngeo_dir, name);

	if ((f = fopen(filename, "rb")) == 0) {
		debug("Error pening nvram %s\n", filename);
		return;
	} else {
		totread = fread(memory.sram, 1, 0x10000, f);
		for(i=0; i<65536; i++) checksum = (checksum << 1) ^ memory.sram[i];
		debug("Opened nvram %s, read %d bytes; checksum: %08X\n", filename, totread, checksum);
		fclose(f);
	}
}

/* TODO: multiple memcard */
void open_memcard(char *name) {
//	char *filename;
	size_t totread = 0;
	uint32_t checksum = 0;
	int i;
// #ifdef EMBEDDED_FS
// 	const char *gngeo_dir = ROOTPATH"save/";
// #elif defined(__AMIGA__)
	const char *gngeo_dir = "save/";
// #else
// 	const char *gngeo_dir = get_gngeo_dir();
// #endif
	FILE *f;
//	int len = strlen("memcard") + strlen(gngeo_dir) + 1; /* ".nv\0" => 4 */

//	filename = (char *) alloca(len);
//	sprintf(filename, "%s%s", gngeo_dir, "memcard");

	if ((f = fopen("save/memcard", "rb")) == 0) {
		debug("Error reading memcard.");
	} else {
		totread = fread(memory.memcard, 1, 0x800, f);
		for(i=0; i<0x800; i++) checksum = (checksum << 1) ^ memory.memcard[i];
		debug("Opened memcard and read %d bytes, checksum %08X.\n", totread, checksum);
		fclose(f);
	}
}

void save_nvram(char *name) {
	char filename[256];
	uint32_t checksum = 0;
// #ifdef EMBEDDED_FS
// 	const char *gngeo_dir = ROOTPATH"save/";
// #elif defined(__AMIGA__)
//	const char *gngeo_dir = strdup("save/");
// #else
// 	const char *gngeo_dir = get_gngeo_dir();
// #endif
	FILE *f;
	//int len = strlen(name) + strlen(gngeo_dir) + 4; /* ".nv\0" => 4 */
	//strlen(name) + strlen(getenv("HOME")) + strlen("/.gngeo/") + 4;
	int i;

	sprintf(filename, "save/%s.nv", name);
	//debug("Save nvram %s\n",filename);
	// for (i = 0xffff; i >= 0; i--) {
	// 	if (memory.sram[i] != 0)
	// 		break;
	// }

	//filename = (char *) alloca(len);
	//sprintf(filename, "%s%s.nv", gngeo_dir, name);

	if ((f = fopen(filename, "wb")) != NULL) {
		int totlen = fwrite(memory.sram, 1, 0x10000, f);
		for(i=0; i<65536; i++) checksum = (checksum << 1) ^ memory.sram[i];
		debug("Saved nvram %s; checksum: %08X\n", filename, checksum);
		fclose(f);
	} else {
		debug("Failed to save %s.\n", filename);
	}
}

void save_memcard(char *name) {
	uint32_t checksum = 0;
	int i;
//	char filename[256];
// #ifdef EMBEDDED_FS
// 	const char *gngeo_dir = ROOTPATH"save/";
// #elif defined(__AMIGA__)
//	const char *gngeo_dir = strdup("save/");
// #else
// 	const char *gngeo_dir = get_gngeo_dir();
// #endif
	FILE *f;
//	int len = strlen("memcard") + strlen(gngeo_dir) + 1; /* ".nv\0" => 4 */

//	filename = (char *) alloca(len);
//	sprintf(filename, "%s", gngeo_dir, "memcard");

	if ((f = fopen("save/memcard", "wb")) != NULL) {
		fwrite(memory.memcard, 1, 0x800, f);
		for(i=0; i<0x800; i++) checksum = (checksum << 1) ^ memory.memcard[i];
		debug("Saved memcard, checksum %08X.\n", checksum);
		fclose(f);
	}
}

int close_game(void) {
	save_nvram(arg[OPTION_FILE]);
	save_memcard(arg[OPTION_FILE]);
	dr_free_roms(&memory.rom);
	return true;
}

int init_game(char *rom_name) {
	screen_reinit();

	if (strstr(rom_name, ".gno") != NULL) {
		dr_open_gno(rom_name);

	} else {

		//open_rom(rom_name);
		if (dr_load_game(rom_name) == false) {
			//debug("Can't load %s\n", rom_name);
			return false;
		}
	}
	open_nvram(arg[OPTION_FILE]);
	open_memcard(arg[OPTION_FILE]);

	//sdl_set_title(arg[OPTION_FILE]);

	init_neo();
	setup_misc_patch(arg[OPTION_FILE]);

	fix_usage = memory.fix_board_usage;
	current_pal = memory.vid.pal_neo[0];
	current_fix = memory.rom.bios_sfix.p;
	current_pc_pal = (uint16_t *) memory.vid.pal_host[0];

	memory.vid.currentpal=0;
	memory.vid.currentfix=0;


	return true;
}
