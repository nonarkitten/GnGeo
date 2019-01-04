#include <exec/types.h>
#include <libraries/dos.h>
#include <workbench/workbench.h>
#include <workbench/startup.h>

#include <exec/types.h>
#include <exec/libraries.h>
#include <libraries/asl.h>
 
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/asl.h>

#include <clib/alib_protos.h>
#include <clib/exec_protos.h>
#include <clib/dos_protos.h>
#include <clib/icon_protos.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "conf.h"
#include "emu.h"

// declare CLI options template string with these modifiers:
const char * argTemplate = 
	 "FILES/M,"				// ROM to load
	 "FRAMESKIP/N,"		// Set the frame-skip from 0 to 9
	 "AUTOFRAMESKIP/S," 	// Enables automatic frame-skip
	 "BENCH/N," 			// Renders N frames and outputs stats
	 "INTERLEAVED/S," 	// Uses scan line interleaving to improve performance
	 "PAL/S," 			// Enables/Forces PAL mode on NTSC machines
	 "NTSC/S," 			// Enables/Forces NTSC mode on PAL machines
	 						// These should be mutually exclusive
	 "SHOWFPS/S,"			// Show frame rate on screen
	 "SAMPLERATE/N," 		// Sets audio sample rate
							//	0 - Audio disabled
							//	9260 - Low quality
							//	18520 - Mid quality
							//	27780 - High quality
	 "ENABLEFM/S,"		// Enables FM audio (otherwise digital only)
	 "LOADLASTSTATE/N,"	// Automatically load last saved state (if present)
	 "REGION/N,"			// Country (region lock code)
	 "SYSTEMTYPE/N,"		// Arcade, home or unibios
	 "BITRATE/N,"			// Audio bits/sample, 8 or 16
	 "ROMPATH,"			// Path to ROMS
	 "BIOSPATH,"			// Path to BIOS
	 "AGA/S,"				// Enable AGA mode
	 "VSYNC/S"			// Enable VSYNC
;

static char game_name[32] = {0};
static char roms_path[32] = "roms";
static char bios_path[32] = "bios";

int arg[OPTION_MAX] = { 0 };

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
	if(s) arg[index] = MatchToolValue(s, "TRUE");
}

#define MYLEFTEDGE 0
#define MYTOPEDGE  0
#define MYWIDTH    320
#define MYHEIGHT   400
 
struct TagItem frtags[] = {
    ASL_Hail,       (ULONG)"The RKM file requester",
    ASL_Height,     MYHEIGHT,
    ASL_Width,      MYWIDTH,
    ASL_LeftEdge,   MYLEFTEDGE,
    ASL_TopEdge,    MYTOPEDGE,
	ASL_FuncFlags, 	FILF_PATGAD,
    ASL_Pattern,    (ULONG)"#?.zip",
    ASL_OKText,     (ULONG)"Okay",
    ASL_CancelText, (ULONG)"Cancel",
    ASL_File,       (ULONG)"",
    ASL_Dir,        (ULONG)roms_path,
    ASL_ExtFlags1,	FIL1F_NOFILES|FIL1F_MATCHDIRS,
    TAG_DONE
};

BOOL FromWb;

void ParseArguments(int argc, char *argv[]) {
	struct DiskObject *dobj;
	struct WBStartup *WBenchMsg;
	struct Library *AslBase;
	struct WBArg *wbarg;
    LONG  wLen;
    LONG olddir = -1;
    SHORT i;

	FromWb = (argc==0) ? TRUE : FALSE;
	
    //AslBase = OpenLibrary("asl.library", 50);
    //IAsl = (struct AslIFace*)GetInterface(AslBase, "main", 1, NULL);
	arg[OPTION_AUTOFRAMESKIP] = 0;
	arg[OPTION_INTERLEAVED] = 0;
	arg[OPTION_PAL] = 0;
	arg[OPTION_NTSC] = 0;
	arg[OPTION_SHOWFPS] = 1;
	arg[OPTION_ENABLEFM] = 1;
	arg[OPTION_LOADLASTSTATE] = 0;
	arg[OPTION_VSYNC] = 0;
	
    if(!FromWb) {
		// CLI start
		struct RDArgs *rdargs;
		const char *arg_name;		
		
		if(!(rdargs = ReadArgs(argTemplate,arg,NULL))) {
			printf("Incorrect values received by ReadArgs\n");
			exit(-1);
		}
			
		if(arg_name = ((char**)arg[OPTION_FILE])[0]) strcpy(game_name, arg_name);
		if(arg[OPTION_BIOSPATH]) strcpy(bios_path, (char*)arg[OPTION_BIOSPATH]);
		if(arg[OPTION_ROMPATH]) strcpy(roms_path, (char*)arg[OPTION_ROMPATH]);
				
		/* ReadArgs puts pointers to LONG here, so fold them back */		
		#define FIXUP(X,DEF) arg[X] = (arg[X]) ? (*(ULONG*)arg[X]) : (DEF)
		FIXUP(OPTION_FRAMESKIP,0);
		FIXUP(OPTION_BENCH,0);
		FIXUP(OPTION_SAMPLERATE,18520);
		FIXUP(OPTION_REGION,(int)CTY_USA);
		FIXUP(OPTION_SYSTEM,(int)SYS_UNIBIOS);
		FIXUP(OPTION_BITRATE,8);
		#undef FIXUP
		
		FreeArgs(rdargs);

	} else {
        WBenchMsg = (struct WBStartup *)argv;
        
        /* We actually only care about the first argument -- us */
        wbarg=WBenchMsg->sm_ArgList; 

		/* if there's a directory lock for this wbarg, CD there */
		if((wbarg->wa_Lock)&&(*wbarg->wa_Name))
			olddir = CurrentDir(wbarg->wa_Lock);

		if((*wbarg->wa_Name) && (dobj=GetDiskObject(wbarg->wa_Name))) {
			char **toolarray = (char **)dobj->do_ToolTypes;
			char *string;
			
	 		setArgStr(toolarray,  "ROM", 			game_name);
	 		setArgStr(toolarray,  "ROMPATH", 		roms_path);
	 		setArgStr(toolarray,  "BIOSPATH", 		bios_path);

	 		setArgInt(toolarray,  "FRAMESKIP", 		OPTION_FRAMESKIP, 1);
	 		setArgInt(toolarray,  "BENCH", 			OPTION_BENCH, 0);
	 		setArgInt(toolarray,  "SAMPLERATE", 	OPTION_SAMPLERATE, 18520);
	 		setArgInt(toolarray,  "BITRATE", 	    OPTION_BITRATE, 8);
	 		
	 		setArgBool(toolarray, "AUTOFRAMESKIP", 	OPTION_AUTOFRAMESKIP);
	 		setArgBool(toolarray, "INTERLEAVED", 	OPTION_INTERLEAVED);
	 		setArgBool(toolarray, "PAL", 			OPTION_PAL);
	 		setArgBool(toolarray, "NTSC", 			OPTION_NTSC);
	 		setArgBool(toolarray, "SHOWFPS", 		OPTION_SHOWFPS);
	 		setArgBool(toolarray, "ENABLEFM", 		OPTION_ENABLEFM);
	 		setArgBool(toolarray, "LOADLASTSTATE", 	OPTION_LOADLASTSTATE);
	 		setArgBool(toolarray, "VSYNC", 			OPTION_VSYNC);
			
			if(MatchToolValue(FindToolType(toolarray, "REGION"), "europe"))
				arg[OPTION_REGION] = CTY_EUROPE;
			else if(MatchToolValue(FindToolType(toolarray, "REGION"), "usa"))
				arg[OPTION_REGION] = CTY_USA;
			else if(MatchToolValue(FindToolType(toolarray, "REGION"), "asia"))
				arg[OPTION_REGION] = CTY_ASIA;
			else 
				arg[OPTION_REGION] = CTY_JAPAN;
			
			if(MatchToolValue(FindToolType(toolarray, "SYSTEM"), "arcade"))
				arg[OPTION_REGION] = SYS_ARCADE;
			else if(MatchToolValue(FindToolType(toolarray, "SYSTEM"), "home"))
				arg[OPTION_REGION] = SYS_HOME;
			else 
				arg[OPTION_REGION] = SYS_UNIBIOS;
	 						 		
			FreeDiskObject(dobj);
	 	}
	 		
		if(olddir != -1)  CurrentDir(olddir); /* CD back where we were */
		
    }

	arg[OPTION_FILE] = (int)game_name;
	arg[OPTION_BIOSPATH] = (int)bios_path;
	arg[OPTION_ROMPATH] = (int)roms_path;

	if((arg[OPTION_FILE] == NULL) || (game_name[0] == 0)) {
		struct FileRequester *fr;
		if (AslBase = OpenLibrary("asl.library", 0)) {
			if (fr = (struct FileRequester *)AllocAslRequest(ASL_FileRequest, frtags)) {
				if (AslRequest(fr, NULL)) {
					char *gn = &fr->rf_Dir[1 + strlen(arg[OPTION_ROMPATH])];
					int gnlen = strstr(gn, ".zip") - gn;
					if(gnlen > 0 && gnlen < 31) {
						memcpy(game_name, gn, gnlen);
						game_name[gnlen] = 0;
						printf("Game: %s\n", game_name);
					}
					
				}
				FreeAslRequest(fr);
			}
			else exit(1); //printf("User Cancelled\n");
			CloseLibrary(AslBase);
		}
	}
        
    	 if(arg[OPTION_FRAMESKIP] < 0) arg[OPTION_FRAMESKIP] = 0;
    else if(arg[OPTION_FRAMESKIP] > 9) arg[OPTION_FRAMESKIP] = 9;
    
    // Both set or both unset, autodetect host system
    if((arg[OPTION_NTSC]) == (arg[OPTION_PAL])) 
    	arg[OPTION_PAL] = HostPAL, arg[OPTION_NTSC] = !HostPAL;

	// Bitrate may be 8 or 16 bits only    
    if((arg[OPTION_BITRATE] != 8)
    && (arg[OPTION_BITRATE] != 16)) arg[OPTION_BITRATE] = 8;
    
    // Any non-standard sample rate will disable digital playback
    if((arg[OPTION_SAMPLERATE] != 9260)
    && (arg[OPTION_SAMPLERATE] != 18520)
    && (arg[OPTION_SAMPLERATE] != 27780)) arg[OPTION_SAMPLERATE] = 0;
}







