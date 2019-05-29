#ifndef _CONF_H_
#define _CONF_H_

typedef enum {
	OPTION_FILE,
	OPTION_FRAMESKIP,
	OPTION_AUTOFRAMESKIP,
	OPTION_BENCH,
	OPTION_INTERLEAVED,
	OPTION_PAL,
	OPTION_NTSC,
	OPTION_SHOWFPS,
	OPTION_SAMPLERATE,
	OPTION_ENABLEFM,
	OPTION_LOADLASTSTATE,
	OPTION_REGION,
	OPTION_SYSTEM,
	OPTION_BITRATE,
	OPTION_ROMPATH,
	OPTION_BIOSPATH,
	OPTION_AGA,
	OPTION_VSYNC,
	OPTION_M68K,
	// OPTION_Z80,
	// OPTION_NOFPS,
	// OPTION_DOWNSAMPLE,
	// OPTION_HIFI,
	// OPTION_OVERSCAN,
	OPTION_MAX
} ARG_t;

extern int arg[OPTION_MAX + 1];

#endif
