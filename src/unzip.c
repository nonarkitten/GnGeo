/*
 * unzip.c
 * Basic unzip interface
 *
 *  Created on: 1 janv. 2010
 *      Author: Mathieu Peponas
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "lz4w.h"

#include <errno.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "unzip.h"

#include <devices/timer.h>
#include <proto/exec.h>

//pkzip = path
ZFILE *gn_unzip_fopen(PKZIP *zf, char *filename, uint32_t file_crc) {
	unsigned char *mem = NULL, *mem2 = NULL;
	struct FileInfoBlock *fib = NULL;
	struct ZFILE *zfile = NULL;
	const char name[256];
	ULONG size = 0;
	BPTR fh = NULL;
	
//	if(!(name = AllocVec(strlen(zf) + strlen(filename) + 2, MEMF_FAST)))
//		return NULL;
		
	sprintf( name, "%s/%s", (char*)zf, filename );
	printf("Decompressing '%s'\n", name);

	if(!(fh = Open(name, MODE_OLDFILE)))
		return NULL;

	if((fib = (struct FileInfoBlock *)AllocDosObject(DOS_FIB, NULL))) {
		ExamineFH(fh, fib);
		size = fib->fib_Size;
		FreeDosObject(DOS_FIB, fib);
	} else return NULL;
		
	//printf("Attempting to allocate %d bytes\n", size);
	if(!(mem = AllocVec(size, MEMF_FAST)))
		return NULL;
		
	//printf("Reading file... ");
	Read(fh, mem, size);
	Close(fh);
	
	size = lz4w_length(mem);
	//printf("Attempting to allocate %d more bytes\n", size);
	if((mem2 = AllocVec(size, MEMF_FAST))) {
		if((zfile = AllocVec(sizeof(struct ZFILE), MEMF_FAST))) {
			lz4w_unpack(mem, mem2);
			zfile->mem = mem2;
			zfile->len = size;
			zfile->pos = 0;
		} else {
			FreeVec(mem2);
		}
	}

	FreeVec(mem);
	return zfile;
}

void gn_unzip_fclose(ZFILE *z) {
	FreeVec(z->mem);
	FreeVec(z);
}

int gn_unzip_fread(ZFILE *z, uint8_t *data, unsigned int size) {
	if(size > (z->len - z->pos)) size = (z->len - z->pos);
	CopyMem( &z->mem[z->pos], data, size );
	z->pos += size;
	return size;
}

uint8_t *gn_unzip_file_malloc(PKZIP *zf, char *filename, uint32_t file_crc, unsigned int *outlen) {
	ZFILE *z = gn_unzip_fopen(zf, filename, file_crc);
	uint8_t *data = z->mem;
	*outlen = z->len;
	FreeVec(z);
	return data;
}

// 
PKZIP *gn_open_zip(char *path) {
	ULONG len = 1 + strlen(path);
	void *mem = AllocVec(len, MEMF_FAST);
	CopyMem(path, mem, len);
	// maybe check if path is valid?
	return (PKZIP*)mem;
}

void gn_close_zip(PKZIP *zf) { FreeVec(zf); }

