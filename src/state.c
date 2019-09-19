#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
 
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "memory.h"
#include "state.h"
#include "fileio.h"
#include "screen.h"
#include "sound.h"
#include "emu.h"
//#include "streams.h"

static int m68k_flag=0x2;

static int z80_flag=0x8;

#ifdef WORDS_BIGENDIAN
static int endian_flag=0x10;
#else
static int endian_flag=0x0;
#endif

#if defined (__AMIGA__)
#define ROOTPATH "data/"
#else
#define ROOTPATH ""
#endif

#if !defined(HAVE_LIBZ) || !defined (HAVE_MMAP)
#define gzopen fopen
#define gzread(f,data,size) fread(data,size,1,f)
#define gzwrite(f,data,size) fwrite(data,size,1,f)
#define gzclose fclose
#define gzFile FILE
#define gzeof feof
#define gzseek fseek

#endif

static ST_REG *reglist;
static ST_MODULE st_mod[ST_MODULE_END];
static Rect buf_rect    =	{24, 16, 304, 224};
static Rect screen_rect =	{ 0,  0, 304, 224};
//SDL_Surface *state_img_tmp;

void cpu_68k_mkstate(gzFile *gzf,int mode);
void cpu_z80_mkstate(gzFile *gzf,int mode);
void ym2610_mkstate(gzFile *gzf,int mode);

void create_state_register(ST_MODULE_TYPE module,const char *reg_name,
			   Uint8 num,void *data,int size,ST_DATA_TYPE type) {
    ST_REG *t=(ST_REG*)calloc(1,sizeof(ST_REG));
    t->next=st_mod[module].reglist;
    st_mod[module].reglist=t;
    t->reg_name=strdup(reg_name);
    t->data=data;
    t->size=size;
    t->type=type;
    t->num=num;
}

void set_pre_save_function(ST_MODULE_TYPE module,void (*func)(void)) {
    st_mod[module].pre_save_state=func;
}

void set_post_load_function(ST_MODULE_TYPE module,void (*func)(void)) {
    st_mod[module].post_load_state=func;
}

static void *find_data_by_name(ST_MODULE_TYPE module,Uint8 num,char *name) {
    ST_REG *t=st_mod[module].reglist;
    while(t) {
	if ((!strcmp(name,t->reg_name)) && (t->num==num)) {
	    /*
	     *len=t->size;
	     *type=t->type;
	     */
	    return t->data;
	}
	t=t->next;
    }
    return NULL;
}

static int sizeof_st_type(ST_DATA_TYPE type) {
    switch (type) {
    case REG_UINT8:
    case REG_INT8:
	return 1;
    case REG_UINT16:
    case REG_INT16:
	return 2;
    case REG_UINT32:
    case REG_INT32:
	return 4;
    }
    return 0; /* never go here */
}

void swap_buf16_if_need(Uint8 src_endian,Uint16* buf,Uint32 size)
{
    int i;
#ifdef WORDS_BIGENDIAN
    Uint8  my_endian=1;
#else
    Uint8  my_endian=0;
#endif
    if (my_endian!=src_endian) {
	for (i=0;i<size;i++)
	    SwapSHORT(buf[i]);
    }
}

void swap_buf32_if_need(Uint8 src_endian,Uint32* buf,Uint32 size)
{
    int i;
#ifdef WORDS_BIGENDIAN
    Uint8  my_endian=1;
#else
    Uint8  my_endian=0;
#endif
    if (my_endian!=src_endian) {
	for (i=0;i<size;i++)
	    buf[i]=SwapLONG(buf[i]);
    }
}

Uint32 how_many_slot(char *game) {
	char *st_name;
	FILE *f;
//    char *st_name_len;
#ifdef EMBEDDED_FS
	char *gngeo_dir=ROOTPATH"save/";
#else
	char *gngeo_dir=get_gngeo_dir();
#endif
	Uint32 slot=0;
	st_name=(char*)alloca(strlen(gngeo_dir)+strlen(game)+5);
	while (1) {
		sprintf(st_name,"%s%s.%03d",gngeo_dir,game,slot);
		if (st_name && (f=fopen(st_name,"rb"))) {
			fclose(f);
			slot++;
		} else
		    return slot;
	}
}

static gzFile *open_state(char *game,int slot,int mode) {
	char *st_name;
//    char *st_name_len;
#ifdef EMBEDDED_FS
	char *gngeo_dir=ROOTPATH"save/";
#else
	char *gngeo_dir=get_gngeo_dir();
#endif
	char string[20];
	char *m=(mode==STWRITE?"wb":"rb");
	gzFile *gzf;
	int  flags;
	Uint32 rate;

    st_name=(char*)alloca(strlen(gngeo_dir)+strlen(game)+5);
    sprintf(st_name,"%s%s.%03d",gngeo_dir,game,slot);

	if ((gzf = gzopen(st_name, m)) == NULL) {
		debug("%s not found\n", st_name);
		return NULL;
    }

	if(mode==STREAD) {

		memset(string, 0, 20);
		gzread(gzf, string, 6);

		if (strcmp(string, "GNGST2")) {
			debug("%s is not a valid gngeo st file\n", st_name);
			gzclose(gzf);
			return NULL;
		}

		gzread(gzf, &flags, sizeof (int));

		if (flags != (m68k_flag | z80_flag | endian_flag)) {
			debug("This save state comme from a different endian architecture.\n"
					"This is not currently supported :(\n");
			gzclose(gzf);
			return NULL;
		}
	} else {
		int flags=m68k_flag | z80_flag | endian_flag;
		gzwrite(gzf, "GNGST2", 6);
		gzwrite(gzf, &flags, sizeof(int));
	}
	return gzf;
}

int mkstate_data(FILE *gzf,void *data,int size,int mode) {
	if (mode==STREAD)
		return gzread(gzf,data,size);
	return gzwrite(gzf,data,size);
}

 

static void neogeo_mkstate(gzFile *gzf,int mode) {
 
}

int save_state(char *game,int slot) {
 
	return 1;
}
int load_state(char *game,int slot) {
 
	return 1;
}



/* neogeo state register */ 
static Uint8 st_current_pal,st_current_fix;

static void neogeo_pre_save_state(void) {

    //st_current_pal=(current_pal==memory.pal1?0:1);
    //st_current_fix=(current_fix==memory.rom.bios_sfix.p?0:1);
    //debug("%d %d\n",st_current_pal,st_current_fix);
    
}

static void neogeo_post_load_state(void) {
 
 
}

void clear_state_reg(void) {
     
}

void neogeo_init_save_state(void) {
   
    
}





