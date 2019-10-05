#include "amiga.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "conf.h"
#include "emu.h"

// declare CLI options template string with these modifiers:
const char * argTemplate = 
	 "FILE,"				// ROM to load
	 "FRAMESKIP/S,"		// Enable frame-skip
	 "BENCH/N/K," 			// Renders N frames and outputs stats
	 "INTERLEAVED/S," 	// Uses scan line interleaving to improve performance
	 "PAL/S," 			// Enables/Forces PAL mode on NTSC machines
	 "NTSC/S," 			// Enables/Forces NTSC mode on PAL machines
	 						// These should be mutually exclusive
	 "SHOWFPS/S,"			// Show frame rate on screen
	 "SAMPLERATE/N/K," 		// Sets audio sample rate
							//	0 - Audio disabled
							//	9260 - Low quality
							//	18520 - Mid quality
							//	27780 - High quality
	 "ENABLEFM/S,"		// Enables FM audio (otherwise digital only)
	 "LOADLASTSTATE/S,"	// Automatically load last saved state (if present)
	 "REGION/N,"			// Country (region lock code)
	 "SYSTEMTYPE/N,"		// Arcade, home or unibios
	 "BITRATE/N,"			// Audio bits/sample, 8 or 16
	 "ROMPATH/K,"			// Path to ROMS
	 "BIOSPATH/K,"			// Path to BIOS
	 "AGA/S,"				// Enable AGA mode
	 "VSYNC/S,"			// Enable VSYNC

	 "M68K/N/K,"  			// Specify relative speed of CPU
	 "NOAMMX/S,"				// disable AMMX
	 "P1JOY/S," // Enable P1 gamepad
	 "P2JOY/S," // Enable P2 gamepad
	 "P13BUTTON/S," // Set P1 to 3-button
	 "P23BUTTON/S," // Set P1 to 3-button
	 "DEBUG/S"
;

static char game_name[32] = {0};
static char roms_path[32] = "roms";
static char bios_path[32] = "bios";

int arg[OPTION_MAX + 1] = { 0 };

static setArgInt(char **toolarray, char *argName, int index, int def) {
	char *s = (char*)FindToolType(toolarray,argName);
	arg[index] = (s) ? atoi(s) : (def);
}

static setArgStr(char **toolarray, char *argName, char *out) {
	char *s = (char*)FindToolType(toolarray,argName);
	if(s) strcpy( out, s );
}

static setArgBool(char **toolarray, char *argName, int index) {
	char *s = (char*)FindToolType(toolarray,argName);
	if(s) arg[index] = 1; //MatchToolValue(s, "true");
}

#define MYLEFTEDGE 0
#define MYTOPEDGE  0
#define MYWIDTH    320
#define MYHEIGHT   400
 
struct TagItem frtags[] = {
    ASL_Hail,       (uint32_t)"The RKM file requester",
    ASL_Height,     MYHEIGHT,
    ASL_Width,      MYWIDTH,
    ASL_LeftEdge,   MYLEFTEDGE,
    ASL_TopEdge,    MYTOPEDGE,
	ASL_FuncFlags, 	FILF_PATGAD,
    ASL_Pattern,    (uint32_t)"#?.zip",
    ASL_OKText,     (uint32_t)"Okay",
    ASL_CancelText, (uint32_t)"Cancel",
    ASL_File,       (uint32_t)"",
    ASL_Dir,        (uint32_t)roms_path,
    ASL_ExtFlags1,	FIL1F_NOFILES|FIL1F_MATCHDIRS,
    TAG_DONE
};

bool FromWb;

static void fixup_readargs(void) {
	#define FIXUP(X) if(arg[X]) arg[X] = *(uint32_t*)arg[X];
	FIXUP(OPTION_BENCH);
	FIXUP(OPTION_SAMPLERATE);
	FIXUP(OPTION_REGION);
	FIXUP(OPTION_SYSTEM);
	FIXUP(OPTION_BITRATE);
	FIXUP(OPTION_M68K);
	#undef FIXUP

	if(arg[OPTION_FILE]) strcpy(game_name, arg[OPTION_FILE]);
	if(arg[OPTION_BIOSPATH]) strcpy(bios_path, arg[OPTION_BIOSPATH]);
	if(arg[OPTION_ROMPATH]) strcpy(roms_path, arg[OPTION_ROMPATH]);
}

void ParseArguments(int argc, char *argv[]) {
	struct DiskObject *dobj;
	struct WBStartup *WBenchMsg;
	struct Library *AslBase;
	struct WBArg *wbarg;
	struct RDArgs *read_args, *free_args;
	static uint32_t defaults[7] = {
		1, 
		100,
		18520,
		(int)CTY_USA,
		(int)SYS_UNIBIOS,
		16,
		0
	};
    LONG  wLen;
    LONG olddir = -1;
    SHORT i;
	BPTR config;
	TEXT* buffer;
	int length = 0;

	FromWb = (argc==0) ? TRUE : FALSE;
	defaults[6] = FromWb;
	
    //AslBase = OpenLibrary("asl.library", 50);
    //IAsl = (struct AslIFace*)GetInterface(AslBase, "main", 1, NULL);

	if(!(buffer = AllocVec(4096, MEMF_PUBLIC)))
		error("Out of memory.\n");

	arg[OPTION_SHOWFPS] = &defaults[0];
	arg[OPTION_M68K] = &defaults[1];
	arg[OPTION_SAMPLERATE] = &defaults[2];
	arg[OPTION_REGION] = &defaults[3];
	arg[OPTION_SYSTEM] = &defaults[4];
	arg[OPTION_BITRATE] = &defaults[5];
	arg[OPTION_DEBUG] = &defaults[6];

	// read default arguments
	if((config = Open("gngeo.config", MODE_OLDFILE))) {
		length = Read(config, buffer, 1024);
		Close(config);
	}

    if(!FromWb) {
		// concatenate command line args
		for(i=1; i<argc; i++) length += sprintf(buffer + length, " %s", argv[i]);
		arg[OPTION_DEBUG] = 1;

	} else {
		arg[OPTION_DEBUG] = 0;
        WBenchMsg = (struct WBStartup *)argv;
        /* We actually only care about the first argument -- us */
        wbarg=WBenchMsg->sm_ArgList;
		/* if there's a directory lock for this wbarg, CD there */
		if((wbarg->wa_Lock)&&(*wbarg->wa_Name))
			olddir = CurrentDir(wbarg->wa_Lock);
		// concatenate tooltypes
		if((*wbarg->wa_Name) && (dobj=GetDiskObject(wbarg->wa_Name))) {
			char *tool, **toolarray = dobj->do_ToolTypes;
			while(tool = *toolarray++) length += sprintf(buffer + length, " %s", tool);
		}	 		
		if(olddir != -1)  CurrentDir(olddir); /* CD back where we were */
    }

	for(i=0; i<length; i++) {
		if(buffer[i] && (buffer[i] < ' ')) buffer[i] = ' ';
	}

	buffer[length++] = '\n'; buffer[length] = 0;
	debug("Parsing args:\n%s\n", buffer);
	if(!(read_args = AllocDosObject(DOS_RDARGS,NULL))) {
		FreeVec(buffer);
		error("Out of memory\n");

	} else {
		read_args->RDA_Source.CS_Buffer = buffer;
		read_args->RDA_Source.CS_Length = length;

		if(!(free_args = ReadArgs(argTemplate, arg, read_args))) {
			debug("Incorrect values received by ReadArgs\n");

		} else {
			fixup_readargs();
			FreeArgs(free_args);
			arg[OPTION_FILE] = (int)game_name;
			arg[OPTION_BIOSPATH] = (int)bios_path;
			arg[OPTION_ROMPATH] = (int)roms_path;
		}
		FreeDosObject(DOS_RDARGS,read_args);
	}
	FreeVec(buffer);

	if(!game_name[0]) {
		struct FileRequester *fr;
		if (AslBase = OpenLibrary("asl.library", 0)) {
			if (fr = (struct FileRequester *)AllocAslRequest(ASL_FileRequest, frtags)) {
				if (AslRequest(fr, NULL)) {
					char *gn = &fr->rf_Dir[1 + strlen(arg[OPTION_ROMPATH])];
					int gnlen = strstr(gn, ".zip") - gn;
					if(gnlen > 0 && gnlen < 31) {
						memcpy(game_name, gn, gnlen);
						game_name[gnlen] = 0;
					}
					
				}
				FreeAslRequest(fr);
			}
			CloseLibrary(AslBase);
		}
	}
	if(!game_name[0]) {
		debug("No ROM was selected.\n");
		exit(1);
	}
	//debug("Game: %s\n", game_name);


		 if(arg[OPTION_M68K] <  25) arg[OPTION_M68K] =  25;
	else if(arg[OPTION_M68K] > 150) arg[OPTION_M68K] = 150;
        
    // Both set or both unset, autodetect host system
    if((arg[OPTION_NTSC]) == (arg[OPTION_PAL])) 
    	arg[OPTION_PAL] = HostPAL, arg[OPTION_NTSC] = !HostPAL;

	// Bitrate may be 8 or 16 bits only    
	if(arg[OPTION_BITRATE] > 12) arg[OPTION_BITRATE] = 16;
	else arg[OPTION_BITRATE] = 8;
    
    // Any non-standard sample rate will disable digital playback
		 if(arg[OPTION_SAMPLERATE] > 22222) arg[OPTION_SAMPLERATE] = 27780;
	else if(arg[OPTION_SAMPLERATE] > 12346) arg[OPTION_SAMPLERATE] = 18520;
	else if(arg[OPTION_SAMPLERATE] >  4500) arg[OPTION_SAMPLERATE] =  9260;
	else                                    arg[OPTION_SAMPLERATE] =     0;
	
	if(arg[OPTION_NOAMMX]) AC68080 = 0;
}







