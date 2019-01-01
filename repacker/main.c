#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdbool.h>
#include <time.h>

#include <exec/exec.h>
#include <exec/execbase.h>
#include <devices/timer.h>
#include <dos/dos.h>
#include <dos/dosasl.h>
#include <workbench/startup.h>

#include <sys/stat.h>

#include <zlib.h>
//#define ZLIB_IN_CHUNK 128*1024
#include <unistd.h>
#include <sys/mman.h>
#include <assert.h>

#include "lz4w.h"

extern struct DosLibrary *DOSBase;

typedef struct {
	unsigned int file_sig : 32;
	
	unsigned int version : 16;
	unsigned int gp_flags : 16;
	unsigned int comp_method : 16;
	unsigned int file_mod_time : 16;
	unsigned int file_mod_date : 16;
	
	unsigned int file_crc32 : 32;
	unsigned int compressed_size : 32;
	unsigned int uncompressed_size : 32;	
	
	unsigned int filename_length : 16;
	unsigned int extra_field_length : 16;
	// filename
	// extra field
} ZipFILE;

const uint8_t ZipFILE_scatter[] = {
	3,2,1,0,
	
	5,4,
	7,6,
	9,8,
	11,10,
	13,12,
	
	17,16,15,14,
	21,20,19,18,
	25,24,23,22,
	
	27,26,
	29,28
};

static void Scatter(uint8_t *data, uint8_t *scatter, int len) {
	int i;
	uint8_t t;
	
	for(i=0; i<len; i++) if(scatter[i] > i) { t = data[i]; data[i] = data[scatter[i]]; data[scatter[i]] = t; } 
}

struct Library *TimerBase;

static ULONG E_Freq;

static ULONG getTicks(){
	static struct EClockVal startTime;
	static int init = 0;
	
	struct EClockVal endTime;
	
	if(!init) {
		E_Freq = ReadEClock( &startTime );	
		printf("Tick rate %d\n", E_Freq);
		init = 1;
		return 0;
		
	} else {
		ULONG diff;
		ReadEClock( &endTime );
		diff = (int)endTime.ev_lo - (int)startTime.ev_lo;
		startTime.ev_lo = endTime.ev_lo;
		startTime.ev_hi = endTime.ev_hi;
		return diff;
	}
}

typedef int (*zipCallback)(FILE*, ZipFILE*, const char *);

static int IsFile(char *filename) {
	struct FileInfoBlock fib;
	BPTR fh = Open(filename, MODE_OLDFILE);
	LONG r = 0;
	if(fh) {
		if(ExamineFH(fh, &fib)) {
			r = fib.fib_DirEntryType < 0;
		}
		Close(fh);
	}
	return r;
}

static __inline unsigned int __bswap_32 (unsigned int __bsx) {
  __asm__ __volatile__ (
  	"ror%.w %#8, %0;"
	"swap %0;"
	"ror%.w %#8, %0"
	: "+d" (__bsx));
  return __bsx;
}

static __inline unsigned short __bswap_16 (unsigned short __bsx) {
  __asm__ __volatile__ ( "ror%.w %#8, %0;" : "+d" (__bsx) );
  return __bsx;
}

static int total_recmp, total_comp, total_uncmp;


static int search_central_dir(char *zipFileName, FILE *zip, zipCallback callback) {
	ZipFILE zipFile;
    static char fileNameBuffer[256];
    static char romFolderTemplate[256];
    static char fileName[256];
    
	unsigned long sig_file = 0x504b0304;
	unsigned long sig_cdir = 0x504b0102;
	char *n;
	BPTR dir;
	//int n = 1;

	// original ROMS are flat-packed with no folder info
	snprintf( romFolderTemplate, sizeof(romFolderTemplate), "%s", zipFileName );
	*(n = strchr( romFolderTemplate, '.' )) = 0;
	dir = CreateDir(romFolderTemplate);
	sprintf( n, "/%%s" );
	
	total_recmp = total_comp = total_uncmp = 0;
	
	do {
		printf("Memory avail: %d.\n", AvailMem(MEMF_PUBLIC));
		fread(&zipFile, sizeof(ZipFILE), 1, zip);
		if(zipFile.file_sig == sig_cdir) {
			printf("Completed successfully.\n");
			printf("Uncompressed size:    %d.\n", total_uncmp);
			printf("Compressed ZIP size:  %d (%d%%).\n", total_comp, (100 * total_comp) / total_uncmp);
			printf("Compressed LZHA size: %d (%d%%).\n", total_recmp, (100 * total_recmp) / total_uncmp);
			UnLock(dir);
			return 1;
		} else if(zipFile.file_sig != sig_file) {
			break;
		}
		
		Scatter(&zipFile, ZipFILE_scatter, sizeof(ZipFILE_scatter));
		fread(fileNameBuffer, zipFile.filename_length, 1, zip);
		fileNameBuffer[zipFile.filename_length] = 0;
		fseek(zip, zipFile.extra_field_length, SEEK_CUR);
		snprintf(fileName, sizeof(fileName), romFolderTemplate, fileNameBuffer);
	} while(callback(zip, &zipFile, fileName));

	//Rename(romNameBuffer, zipFileName);
error:
	printf("Corrupt zip (%08X)\n", zipFile.file_sig);
	return 0;
}

static int processZip(FILE* zip, ZipFILE* zf, const char *filename) {
	unsigned char *out = AllocVec(zf->uncompressed_size, MEMF_CLEAR);
	unsigned char *lz4w;
    int ret = Z_OK;
    int resize = 0;
    FILE* file;
	int n = 1;
    
    total_uncmp += zf->uncompressed_size;
    total_comp += zf->compressed_size;
    
    if(zf->comp_method == 8) {
		/* allocate inflate state */
		unsigned char *in = AllocVec(zf->compressed_size, MEMF_CLEAR);
	    z_stream strm = { 0 };

	    int percent = 0;
	    if(zf->uncompressed_size) percent = (100 * zf->compressed_size) / zf->uncompressed_size;
		printf("Recompressing '%s', %d:%d (%d%%)...", filename, 
			zf->compressed_size, zf->uncompressed_size, percent);
		
		fread(in, zf->compressed_size, 1, zip);

		strm.avail_out = zf->uncompressed_size;
		strm.next_out = out;

		strm.avail_in = zf->compressed_size;
		strm.next_in = in;

		inflateInit2(&strm, -MAX_WBITS);
		inflate(&strm, Z_NO_FLUSH);

		inflateEnd(&strm);
		FreeVec(in);

    } else if(zf->comp_method == 0) {
		printf("Compressing '%s', %d ...", filename, zf->compressed_size);
    	fread(out, zf->compressed_size, 1, zip);
    	
    } else return 0;

	lz4w = lz4w_compress(out, zf->uncompressed_size, 1, &resize);
	if(lz4w) printf(" to %d bytes (%d%%).\n", resize, (100 * resize) / zf->uncompressed_size);		
	
	if((file = fopen(filename, "wb"))) {
		fwrite(lz4w, resize, 1, file);
		total_recmp += resize;
		close(file);
	} else {
		printf("Error writing file.\n");
	}
	
	FreeVec(lz4w);
	FreeVec(out);
	
	return (Z_OK == ret);
}

int main(int argc,char **argv) {
	STRPTR pattern[512];
	FILE* file;
	struct AnchorPath *anchorpath;
	
	anchorpath = (struct AnchorPath*)AllocVec(sizeof(struct AnchorPath) + 512, MEMF_CLEAR);
	
	if(argc == 1) snprintf( pattern, 512, "#?.zip" );
	else snprintf( pattern, 512, "%s#?.zip", argv[1] );
	
	printf("Searching for '%s'\n", pattern);
	if (DOSBase = (struct DosLibrary *) OpenLibrary("dos.library", 37)) {

		anchorpath->ap_Strlen = 512;
		anchorpath->ap_BreakBits = SIGBREAKF_CTRL_C;

		if (0 == MatchFirst(pattern, anchorpath)) {
			do{ 
				STRPTR path = anchorpath->ap_Buf;
				printf("Found zip archive '%s'.\n", path);
				if(IsFile(path)) {
					if((file = fopen(path, "rb"))) {
						// real ZIP file, let's decompress it
						int ret = search_central_dir(path, file, processZip);
						close(file);
						if(ret) {
							char buffer[256];
							snprintf( buffer, sizeof(buffer), "%s.old", path );
							printf("Renaming %s to %s.\n", path, buffer);
							Rename(path, buffer);
							*strchr(buffer, '.') = 0;
							printf("Renaming %s to %s.\n", buffer, path);
							Rename(buffer, path);
						}
					}
				}
			} while (0 == MatchNext(anchorpath));
		}
		MatchEnd(anchorpath);
	}
	FreeVec(anchorpath);
	
	printf("Done.\n");
}