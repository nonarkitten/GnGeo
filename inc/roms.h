/* Roms/Ram driver interface */
#ifndef H_ROMS
#define H_ROMS

 
#include <stdbool.h>

#define REGION_AUDIO_CPU_BIOS        0
#define REGION_AUDIO_CPU_CARTRIDGE   1
#define REGION_AUDIO_CPU_ENCRYPTED   2
#define REGION_AUDIO_DATA_1          3
#define REGION_AUDIO_DATA_2          4
#define REGION_FIXED_LAYER_BIOS      5
#define REGION_FIXED_LAYER_CARTRIDGE 6
#define REGION_MAIN_CPU_BIOS         7
#define REGION_MAIN_CPU_CARTRIDGE    8
#define REGION_SPRITES               9
#define REGION_SPR_USAGE             10
#define REGION_GAME_FIX_USAGE        11

#define HAS_CUSTOM_CPU_BIOS 0x1
#define HAS_CUSTOM_AUDIO_BIOS 0x2
#define HAS_CUSTOM_SFIX_BIOS 0x4

typedef struct ROM_DEF{
	char name[32];
	char parent[32];
	char longname[128];
	uint32_t year;
	uint32_t romsize[10];
	uint32_t nb_romfile;
	struct romfile{
		char filename[32];
		uint8_t region;
		uint32_t src;
		uint32_t dest;
		uint32_t size;
		uint32_t crc;
	}rom[32];
}ROM_DEF;

typedef struct GAME_INFO {
	char *name;
	char *longname;
	int year;
	uint32_t flags;
}GAME_INFO;

typedef struct ROM_REGION {
	uint8_t* p;
	uint32_t size;
}ROM_REGION;


typedef struct GAME_ROMS {
	GAME_INFO info;
	ROM_REGION cpu_m68k;
	ROM_REGION cpu_z80;
	ROM_REGION tiles;
	ROM_REGION game_sfix;
	ROM_REGION bios_sfix;
	ROM_REGION bios_audio;
	ROM_REGION zoom_table;
	ROM_REGION bios_m68k;
	ROM_REGION adpcma;
	ROM_REGION adpcmb;
	ROM_REGION spr_usage;
	ROM_REGION gfix_usage;  /* Game fix char usage */
	ROM_REGION bfix_usage;  /* Bios fix char usage */
	ROM_REGION cpu_z80c; /* Crypted z80 program rom */
}GAME_ROMS;



int dr_load_roms(GAME_ROMS *r,char *rom_path,char *name);
void dr_free_roms(GAME_ROMS *r);
int dr_save_gno(GAME_ROMS *r,char *filename);
int dr_load_game(char *zip);
ROM_DEF *dr_check_zip(char *filename);
char *dr_gno_romname(char *filename);
int dr_open_gno(char *filename);

#endif
