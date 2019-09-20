#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>

//#include "SDL.h"
 
#include "emu.h"
#include "roms.h"
#include "resfile.h"
#include "unzip.h"
//#include "stb_zlib.h"
#include "conf.h"
//#include "stb_image.h"


void zread_char(ZFILE *gz, char *c, int len) {
	int rc;
	rc = gn_unzip_fread(gz, (uint8_t*)c, len);
	//debug("HS  %s %d\n",c,rc);
}
void zread_uint8_t(ZFILE *gz, uint8_t *c) {
	int rc;
	rc = gn_unzip_fread(gz, c, 1);
	//debug("H8  %02x %d\n",*c,rc);
}
void zread_uint32_tle(ZFILE *gz, uint32_t *c) {
	int rc;
	rc = gn_unzip_fread(gz, (uint8_t*)c, sizeof(uint32_t));
#ifdef WORDS_BIGENDIAN
	*c= __builtin_bswap32(*c);
#endif
	//debug("H32  %08x %d\n",*c,rc);
}

/*
 * Load a rom definition file from gngeo.dat (rom/name.drv)
 * return ROM_DEF*, NULL on error
 */
const char *gngeo_dat = "data/gngeo_data.zip";
ROM_DEF *res_load_drv(char *name) {
	//char *gngeo_dat;// = CF_STR(cf_get_item_by_name("datafile"));
	ROM_DEF *drv;
	char drvfname[32];
	PKZIP *pz;
	ZFILE *z;
	int i;

	drv = calloc(sizeof(ROM_DEF), 1);

	/* Open the rom driver def */
	
	//debug("%s\n",gngeo_dat);
	
	pz = gn_open_zip(gngeo_dat);
	if (pz == NULL) {
		fprintf(stderr, "Can't open the %s\n", gngeo_dat);
		return NULL;
	}
	sprintf(drvfname, "rom/%s.drv", name);
    debug("Driver = %s\n",drvfname);
    
	if ((z=gn_unzip_fopen(pz,drvfname,0x0)) == NULL) {
		fprintf(stderr, "Can't open rom driver for %s\n", name);
		return NULL;
	}
    //debug(" done Driver = %s\n",drvfname);
	//Fill the driver struct
	zread_char(z, drv->name, 32);
	zread_char(z, drv->parent, 32);
	zread_char(z, drv->longname, 128);
	zread_uint32_tle(z, &drv->year);
	
	for (i = 0; i < 10; i++)
		zread_uint32_tle(z, &drv->romsize[i]);
		
	zread_uint32_tle(z, &drv->nb_romfile);
	
	for (i = 0; i < drv->nb_romfile; i++) {
		zread_char(z, drv->rom[i].filename, 32);
		zread_uint8_t(z, &drv->rom[i].region);
		zread_uint32_tle(z, &drv->rom[i].src);
		zread_uint32_tle(z, &drv->rom[i].dest);
		zread_uint32_tle(z, &drv->rom[i].size);
		zread_uint32_tle(z, &drv->rom[i].crc);
	}
	gn_unzip_fclose(z);
	gn_close_zip(pz);
	return drv;
}



/*
 * Load a stb image from gngeo.dat
 * return a SDL_Surface, NULL on error
 * supported format: bmp, tga, jpeg, png, psd
 * 24&32bpp only
 */
void *res_load_data(char *name) {
	PKZIP *pz;
	uint8_t * buffer;
	uint32_t size;

	pz = gn_open_zip(gngeo_dat); //gn_open_zip(CF_STR(cf_get_item_by_name("datafile")));
	if (!pz)
		return NULL;
	buffer = gn_unzip_file_malloc(pz, name, 0x0, &size);
	gn_close_zip(pz);
	return buffer;
}
