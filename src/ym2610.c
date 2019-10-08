/***************************************************************************

 ym2610.c

 Software emulation for YAMAHA YM-2610 sound generator

 Copyright (C) 2001, 2002, 2003 Jarek Burczynski (bujar at mame dot net)
 Copyright (C) 1998 Tatsuyuki Satoh, MultiArcadeMachineEmulator development

 Version 1.4 (final beta)

 Comming from NJ pspmvs emu comming from Mame

 ***************************************************************************/

/*--------------------------------------------------------------------------
 
 History:

 03-08-2003 Jarek Burczynski:
 - fixed YM2608 initial values (after the reset)
 - fixed flag and irqmask handling (YM2608)
 - fixed BUFRDY flag handling (YM2608)

 14-06-2003 Jarek Burczynski:
 - implemented all of the YM2608 status register flags
 - implemented support for external memory read/write via YM2608
 - implemented support for deltat memory limit register in YM2608 emulation

 22-05-2003 Jarek Burczynski:
 - fixed LFO PM calculations (copy&paste bugfix)

 08-05-2003 Jarek Burczynski:
 - fixed SSG support

 22-04-2003 Jarek Burczynski:
 - implemented 100% correct LFO generator (verified on real YM2610 and YM2608)

 15-04-2003 Jarek Burczynski:
 - added support for YM2608's register 0x110 - status mask

 01-12-2002 Jarek Burczynski:
 - fixed register addressing in YM2608, YM2610, YM2610B chips. (verified on real YM2608)
 The addressing patch used for early Neo-Geo games can be removed now.

 26-11-2002 Jarek Burczynski, Nicola Salmoria:
 - recreated YM2608 ADPCM ROM using data from real YM2608's output which leads to:
 - added emulation of YM2608 drums.
 - output of YM2608 is two times lower now - same as YM2610 (verified on real YM2608)

 16-08-2002 Jarek Burczynski:
 - binary exact Envelope Generator (verified on real YM2203);
 identical to YM2151
 - corrected 'off by one' error in feedback calculations (when feedback is off)
 - corrected connection (algorithm) calculation (verified on real YM2203 and YM2610)

 18-12-2001 Jarek Burczynski:
 - added SSG-EG support (verified on real chip)

 12-08-2001 Jarek Burczynski:
 - corrected sin_tab and tl_tab data (verified on real chip)
 - corrected feedback calculations (verified on real chip)
 - corrected phase generator calculations (verified on real chip)
 - corrected envelope generator calculations (verified on real chip)
 - corrected volume level.
 - changed YM2610Update() function:
 this was needed to calculate YM2610 FM channels output correctly.
 (Each FM channel is calculated as in other chips, but the output of the
 channel gets shifted right by one *before* sending to accumulator.
 That was impossible to do with previous implementation).

 23-07-2001 Jarek Burczynski, Nicola Salmoria:
 - corrected ADPCM type A algorithm and tables (verified on real chip)

 11-06-2001 Jarek Burczynski:
 - corrected end of sample bug in OPNB_ADPCM_CALC_CH.
 Real YM2610 checks for equality between current and end addresses
 (only 20 LSB bits).

 08-12-1998 hiro-shi:
 - rename ADPCMA -> ADPCMB, ADPCMB -> DELTAT
 - move ROM limit check.(CALC_CH? -> 2610Write1/2)
 - test program (ADPCMB_TEST)
 - move ADPCM A/B end check.
 - ADPCMB repeat flag(no check)
 - change ADPCM volume rate (8->16) (32->48).

 09-12-1998 hiro-shi:
 - change ADPCM volume. (8->16, 48->64)
 - replace ym2610 ch0/3 (YM-2610B)
 - init cur_chip (restart bug fix)
 - change ADPCM_SHIFT (10->8) missing bank change 0x4000-0xffff.
 - add ADPCM_SHIFT_MASK
 - change ADPCMA_DECODE_MIN/MAX.

 ----------------------------------------------------------------------------

 comment of hiro-shi(Hiromitsu Shioya):
 YM2610 = OPN-B
 YM2610 : PSG:3ch FM:4ch ADPCM(18.5KHz):6ch DeltaT ADPCM:1ch

 --------------------------------------------------------------------------*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>

#include "mvs.h"
#include "state.h"
#include "2610intf.h"
#include "ym2610.h"

#ifndef PI
#define PI 3.14159265358979323846
#endif

/* select timer system internal or external */
#define FM_INTERNAL_TIMER 0

/* --- speedup optimize --- */
/* busy flag enulation , The definition of FM_GET_TIME_NOW() is necessary. */
#define FM_BUSY_FLAG_SUPPORT 1

/*------------------------------------------------------------------------*/

#define FREQ_SH			16  /* 16.16 fixed point (frequency calculations) */
#define EG_SH			16  /* 16.16 fixed point (envelope generator timing) */
#define LFO_SH			24  /*  8.24 fixed point (LFO calculations)       */
#define TIMER_SH		16  /* 16.16 fixed point (timers calculations)    */

#define FREQ_MASK		((1<<FREQ_SH)-1)

#define ENV_BITS		10
#define ENV_LEN			(1<<ENV_BITS)
#define ENV_STEP		(128.0/ENV_LEN)

#define MAX_ATT_INDEX	(ENV_LEN-1) /* 1023 */
#define MIN_ATT_INDEX	(0)			/* 0 */

#define EG_ATT			4
#define EG_DEC			3
#define EG_SUS			2
#define EG_REL			1
#define EG_OFF			0

#define SIN_BITS		10
#define SIN_LEN			(1<<SIN_BITS)
#define SIN_MASK		(SIN_LEN-1)

#define TL_RES_LEN		(256) /* 8 bits addressing (real chip) */

#define FINAL_SH	(0)
#define MAXOUT		(+32767)
#define MINOUT		(-32768)

/*  TL_TAB_LEN is calculated as:
 *   13 - sinus amplitude bits     (Y axis)
 *   2  - sinus sign bit           (Y axis)
 *   TL_RES_LEN - sinus resolution (X axis)
 */
#define TL_TAB_LEN (13*2*TL_RES_LEN)
//static signed int ALIGN_DATA tl_tab[TL_TAB_LEN];

#define ENV_QUIET		(TL_TAB_LEN>>3)

/* sin waveform table in 'decibel' scale */
//static unsigned int ALIGN_DATA sin_tab[SIN_LEN];

/* sustain level table (3dB per step) */
/* bit0, bit1, bit2, bit3, bit4, bit5, bit6 */
/* 1,    2,    4,    8,    16,   32,   64   (value)*/
/* 0.75, 1.5,  3,    6,    12,   24,   48   (dB)*/

/* 0 - 15: 0, 3, 6, 9,12,15,18,21,24,27,30,33,36,39,42,93 (dB)*/
#define SC(db) (u32) (db * (4.0/ENV_STEP) )
static const u32 ALIGN_DATA sl_table[16] = { SC(0), SC(1), SC(2), SC(3 ),
		SC(4 ), SC(5 ), SC(6 ), SC(7), SC(8), SC(9), SC(10), SC(11),
		SC(12), SC(13), SC(14), SC(31) };
#undef SC

#define RATE_STEPS (8)
static const u8 ALIGN_DATA eg_inc[19 * RATE_STEPS] = {

/*cycle:0 1  2 3  4 5  6 7*/

/* 0 */0, 1, 0, 1, 0, 1, 0, 1, /* rates 00..11 0 (increment by 0 or 1) */
/* 1 */0, 1, 0, 1, 1, 1, 0, 1, /* rates 00..11 1 */
/* 2 */0, 1, 1, 1, 0, 1, 1, 1, /* rates 00..11 2 */
/* 3 */0, 1, 1, 1, 1, 1, 1, 1, /* rates 00..11 3 */

/* 4 */1, 1, 1, 1, 1, 1, 1, 1, /* rate 12 0 (increment by 1) */
/* 5 */1, 1, 1, 2, 1, 1, 1, 2, /* rate 12 1 */
/* 6 */1, 2, 1, 2, 1, 2, 1, 2, /* rate 12 2 */
/* 7 */1, 2, 2, 2, 1, 2, 2, 2, /* rate 12 3 */

/* 8 */2, 2, 2, 2, 2, 2, 2, 2, /* rate 13 0 (increment by 2) */
/* 9 */2, 2, 2, 4, 2, 2, 2, 4, /* rate 13 1 */
/*10 */2, 4, 2, 4, 2, 4, 2, 4, /* rate 13 2 */
/*11 */2, 4, 4, 4, 2, 4, 4, 4, /* rate 13 3 */

/*12 */4, 4, 4, 4, 4, 4, 4, 4, /* rate 14 0 (increment by 4) */
/*13 */4, 4, 4, 8, 4, 4, 4, 8, /* rate 14 1 */
/*14 */4, 8, 4, 8, 4, 8, 4, 8, /* rate 14 2 */
/*15 */4, 8, 8, 8, 4, 8, 8, 8, /* rate 14 3 */

/*16 */8, 8, 8, 8, 8, 8, 8, 8, /* rates 15 0, 15 1, 15 2, 15 3 (increment by 8) */
/*17 */16, 16, 16, 16, 16, 16, 16, 16, /* rates 15 2, 15 3 for attack */
/*18 */0, 0, 0, 0, 0, 0, 0, 0, /* infinity rates for attack and decay(s) */
};

#define O(a) (a*RATE_STEPS)

/*note that there is no O(17) in this table - it's directly in the code */
static const u8 ALIGN_DATA eg_rate_select[32+64+32]={	/* Envelope Generator rates (32 + 64 rates + 32 RKS) */
/* 32 infinite time rates */O(18), O(18), O(18), O(18), O(18), O(18), O(18),
		O(18), O(18), O(18), O(18), O(18), O(18), O(18), O(18), O(18),
		O(18), O(18), O(18), O(18), O(18), O(18), O(18), O(18), O(18),
		O(18), O(18), O(18), O(18), O(18), O(18), O(18),

		/* rates 00-11 */O(0), O(1), O(2), O(3), O(0), O(1), O(2), O(3),
		O(0), O(1), O(2), O(3), O(0), O(1), O(2), O(3), O(0),
		O(1), O(2), O(3), O(0), O(1), O(2), O(3), O(0), O(1),
		O(2), O(3), O(0), O(1), O(2), O(3), O(0), O(1), O(2),
		O(3), O(0), O(1), O(2), O(3), O(0), O(1), O(2), O(3),
		O(0), O(1), O(2), O(3),

		/* rate 12 */O(4), O(5), O(6), O(7),

		/* rate 13 */O(8), O(9), O(10), O(11),

		/* rate 14 */O(12), O(13), O(14), O(15),

		/* rate 15 */O(16), O(16), O(16), O(16),

		/* 32 dummy rates (same as 15 3) */O(16), O(16), O(16), O(16), O(16),
		O(16), O(16), O(16), O(16), O(16), O(16), O(16), O(16), O(16),
		O(16), O(16), O(16), O(16), O(16), O(16), O(16), O(16), O(16),
		O(16), O(16), O(16), O(16), O(16), O(16), O(16), O(16), O(16)

};
#undef O

/*rate  0,    1,    2,   3,   4,   5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15*/
/*shift 11,   10,   9,   8,   7,   6,  5,  4,  3,  2, 1,  0,  0,  0,  0,  0 */
/*mask  2047, 1023, 511, 255, 127, 63, 31, 15, 7,  3, 1,  0,  0,  0,  0,  0 */

#define O(a) (a*1)
static const u8 ALIGN_DATA eg_rate_shift[32+64+32]={	/* Envelope Generator counter shifts (32 + 64 rates + 32 RKS) */
/* 32 infinite time rates */O(0), O(0), O(0), O(0), O(0), O(0), O(0), O(0),
		O(0), O(0), O(0), O(0), O(0), O(0), O(0), O(0), O(0), O(0), O(0),
		O(0), O(0), O(0), O(0), O(0), O(0), O(0), O(0), O(0), O(0), O(0),
		O(0), O(0),

		/* rates 00-11 */O(11), O(11), O(11), O(11), O(10), O(10), O(10), O(10),
		O(9), O(9), O(9), O(9), O(8), O(8), O(8), O(8), O(7),
		O(7), O(7), O(7), O(6), O(6), O(6), O(6), O(5), O(5),
		O(5), O(5), O(4), O(4), O(4), O(4), O(3), O(3), O(3),
		O(3), O(2), O(2), O(2), O(2), O(1), O(1), O(1), O(1),
		O(0), O(0), O(0), O(0),

		/* rate 12 */O(0), O(0), O(0), O(0),

		/* rate 13 */O(0), O(0), O(0), O(0),

		/* rate 14 */O(0), O(0), O(0), O(0),

		/* rate 15 */O(0), O(0), O(0), O(0),

		/* 32 dummy rates (same as 15 3) */O(0), O(0), O(0), O(0), O(0),
		O(0), O(0), O(0), O(0), O(0), O(0), O(0), O(0), O(0),
		O(0), O(0), O(0), O(0), O(0), O(0), O(0), O(0), O(0),
		O(0), O(0), O(0), O(0), O(0), O(0), O(0), O(0), O(0)

};
#undef O

static const u8 ALIGN_DATA dt_tab[4 * 32] = {
/* this is YM2151 and YM2612 phase increment data (in 10.10 fixed point format)*/
/* FD=0 */
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0,
		/* FD=1 */
		0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 3, 3, 3, 4, 4, 4, 5,
		5, 6, 6, 7, 8, 8, 8, 8,
		/* FD=2 */
		1, 1, 1, 1, 2, 2, 2, 2, 2, 3, 3, 3, 4, 4, 4, 5, 5, 6, 6, 7, 8, 8, 9, 10,
		11, 12, 13, 14, 16, 16, 16, 16,
		/* FD=3 */
		2, 2, 2, 2, 2, 3, 3, 3, 4, 4, 4, 5, 5, 6, 6, 7, 8, 8, 9, 10, 11, 12, 13,
		14, 16, 17, 19, 20, 22, 22, 22, 22 };

/* OPN key frequency number -> key code follow table */
/* fnum higher 4bit -> keycode lower 2bit */
static const u8 ALIGN_DATA opn_fktable[16] = { 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 3,
		3, 3, 3, 3, 3 };

/* 8 LFO speed parameters */
/* each value represents number of samples that one LFO level will last for */
static const u32 ALIGN_DATA lfo_samples_per_step[8] = { 108, 77, 71, 67, 62, 44,
		8, 5 };

/*There are 4 different LFO AM depths available, they are:
 0 dB, 1.4 dB, 5.9 dB, 11.8 dB
 Here is how it is generated (in EG steps):

 11.8 dB = 0, 2, 4, 6, 8, 10,12,14,16...126,126,124,122,120,118,....4,2,0
 5.9 dB = 0, 1, 2, 3, 4, 5, 6, 7, 8....63, 63, 62, 61, 60, 59,.....2,1,0
 1.4 dB = 0, 0, 0, 0, 1, 1, 1, 1, 2,...15, 15, 15, 15, 14, 14,.....0,0,0

 (1.4 dB is loosing precision as you can see)

 It's implemented as generator from 0..126 with step 2 then a shift
 right N times, where N is:
 8 for 0 dB
 3 for 1.4 dB
 1 for 5.9 dB
 0 for 11.8 dB
 */
static const u8 lfo_ams_depth_shift[4] = { 8, 3, 1, 0 };

/*There are 8 different LFO PM depths available, they are:
 0, 3.4, 6.7, 10, 14, 20, 40, 80 (cents)

 Modulation level at each depth depends on F-NUMBER bits: 4,5,6,7,8,9,10
 (bits 8,9,10 = FNUM MSB from OCT/FNUM register)

 Here we store only first quarter (positive one) of full waveform.
 Full table (lfo_pm_table) containing all 128 waveforms is build
 at run (init) time.

 One value in table below represents 4 (four) basic LFO steps
 (1 PM step = 4 AM steps).

 For example:
 at LFO SPEED=0 (which is 108 samples per basic LFO step)
 one value from "lfo_pm_output" table lasts for 432 consecutive
 samples (4*108=432) and one full LFO waveform cycle lasts for 13824
 samples (32*432=13824; 32 because we store only a quarter of whole
 waveform in the table below)
 */
static const u8 ALIGN_DATA lfo_pm_output[7 * 8][8] = { /* 7 bits meaningful (of F-NUMBER), 8 LFO output levels per one depth (out of 32), 8 LFO depths */
/* FNUM BIT 4: 000 0001xxxx */
/* DEPTH 0 */{ 0, 0, 0, 0, 0, 0, 0, 0 },
/* DEPTH 1 */{ 0, 0, 0, 0, 0, 0, 0, 0 },
/* DEPTH 2 */{ 0, 0, 0, 0, 0, 0, 0, 0 },
/* DEPTH 3 */{ 0, 0, 0, 0, 0, 0, 0, 0 },
/* DEPTH 4 */{ 0, 0, 0, 0, 0, 0, 0, 0 },
/* DEPTH 5 */{ 0, 0, 0, 0, 0, 0, 0, 0 },
/* DEPTH 6 */{ 0, 0, 0, 0, 0, 0, 0, 0 },
/* DEPTH 7 */{ 0, 0, 0, 0, 1, 1, 1, 1 },

/* FNUM BIT 5: 000 0010xxxx */
/* DEPTH 0 */{ 0, 0, 0, 0, 0, 0, 0, 0 },
/* DEPTH 1 */{ 0, 0, 0, 0, 0, 0, 0, 0 },
/* DEPTH 2 */{ 0, 0, 0, 0, 0, 0, 0, 0 },
/* DEPTH 3 */{ 0, 0, 0, 0, 0, 0, 0, 0 },
/* DEPTH 4 */{ 0, 0, 0, 0, 0, 0, 0, 0 },
/* DEPTH 5 */{ 0, 0, 0, 0, 0, 0, 0, 0 },
/* DEPTH 6 */{ 0, 0, 0, 0, 1, 1, 1, 1 },
/* DEPTH 7 */{ 0, 0, 1, 1, 2, 2, 2, 3 },

/* FNUM BIT 6: 000 0100xxxx */
/* DEPTH 0 */{ 0, 0, 0, 0, 0, 0, 0, 0 },
/* DEPTH 1 */{ 0, 0, 0, 0, 0, 0, 0, 0 },
/* DEPTH 2 */{ 0, 0, 0, 0, 0, 0, 0, 0 },
/* DEPTH 3 */{ 0, 0, 0, 0, 0, 0, 0, 0 },
/* DEPTH 4 */{ 0, 0, 0, 0, 0, 0, 0, 1 },
/* DEPTH 5 */{ 0, 0, 0, 0, 1, 1, 1, 1 },
/* DEPTH 6 */{ 0, 0, 1, 1, 2, 2, 2, 3 },
/* DEPTH 7 */{ 0, 0, 2, 3, 4, 4, 5, 6 },

/* FNUM BIT 7: 000 1000xxxx */
/* DEPTH 0 */{ 0, 0, 0, 0, 0, 0, 0, 0 },
/* DEPTH 1 */{ 0, 0, 0, 0, 0, 0, 0, 0 },
/* DEPTH 2 */{ 0, 0, 0, 0, 0, 0, 1, 1 },
/* DEPTH 3 */{ 0, 0, 0, 0, 1, 1, 1, 1 },
/* DEPTH 4 */{ 0, 0, 0, 1, 1, 1, 1, 2 },
/* DEPTH 5 */{ 0, 0, 1, 1, 2, 2, 2, 3 },
/* DEPTH 6 */{ 0, 0, 2, 3, 4, 4, 5, 6 },
/* DEPTH 7 */{ 0, 0, 4, 6, 8, 8, 0xa, 0xc },

/* FNUM BIT 8: 001 0000xxxx */
/* DEPTH 0 */{ 0, 0, 0, 0, 0, 0, 0, 0 },
/* DEPTH 1 */{ 0, 0, 0, 0, 1, 1, 1, 1 },
/* DEPTH 2 */{ 0, 0, 0, 1, 1, 1, 2, 2 },
/* DEPTH 3 */{ 0, 0, 1, 1, 2, 2, 3, 3 },
/* DEPTH 4 */{ 0, 0, 1, 2, 2, 2, 3, 4 },
/* DEPTH 5 */{ 0, 0, 2, 3, 4, 4, 5, 6 },
/* DEPTH 6 */{ 0, 0, 4, 6, 8, 8, 0xa, 0xc },
/* DEPTH 7 */{ 0, 0, 8, 0xc, 0x10, 0x10, 0x14, 0x18 },

/* FNUM BIT 9: 010 0000xxxx */
/* DEPTH 0 */{ 0, 0, 0, 0, 0, 0, 0, 0 },
/* DEPTH 1 */{ 0, 0, 0, 0, 2, 2, 2, 2 },
/* DEPTH 2 */{ 0, 0, 0, 2, 2, 2, 4, 4 },
/* DEPTH 3 */{ 0, 0, 2, 2, 4, 4, 6, 6 },
/* DEPTH 4 */{ 0, 0, 2, 4, 4, 4, 6, 8 },
/* DEPTH 5 */{ 0, 0, 4, 6, 8, 8, 0xa, 0xc },
/* DEPTH 6 */{ 0, 0, 8, 0xc, 0x10, 0x10, 0x14, 0x18 },
/* DEPTH 7 */{ 0, 0, 0x10, 0x18, 0x20, 0x20, 0x28, 0x30 },

/* FNUM BIT10: 100 0000xxxx */
/* DEPTH 0 */{ 0, 0, 0, 0, 0, 0, 0, 0 },
/* DEPTH 1 */{ 0, 0, 0, 0, 4, 4, 4, 4 },
/* DEPTH 2 */{ 0, 0, 0, 4, 4, 4, 8, 8 },
/* DEPTH 3 */{ 0, 0, 4, 4, 8, 8, 0xc, 0xc },
/* DEPTH 4 */{ 0, 0, 4, 8, 8, 8, 0xc, 0x10 },
/* DEPTH 5 */{ 0, 0, 8, 0xc, 0x10, 0x10, 0x14, 0x18 },
/* DEPTH 6 */{ 0, 0, 0x10, 0x18, 0x20, 0x20, 0x28, 0x30 },
/* DEPTH 7 */{ 0, 0, 0x20, 0x30, 0x40, 0x40, 0x50, 0x60 },

};

#include "conf.h"
int enablefm = 0;
int enable16 = 0;

/* all 128 LFO PM waveforms */
static s32 ALIGN_DATA lfo_pm_table[128 * 8 * 32]; /* 128 combinations of 7 bits meaningful (of F-NUMBER), 8 LFO depths, 32 LFO output levels per one depth */

/*----------------------------------
 for SSG emulator
 -----------------------------------*/

#define SSG_MAX_OUTPUT 0x7fff
#define SSG_STEP 0x8000

/* SSG register ID */
#define SSG_AFINE		(0)
#define SSG_ACOARSE		(1)
#define SSG_BFINE		(2)
#define SSG_BCOARSE		(3)
#define SSG_CFINE		(4)
#define SSG_CCOARSE		(5)
#define SSG_NOISEPER	(6)
#define SSG_ENABLE		(7)
#define SSG_AVOL		(8)
#define SSG_BVOL		(9)
#define SSG_CVOL		(10)
#define SSG_EFINE		(11)
#define SSG_ECOARSE		(12)
#define SSG_ESHAPE		(13)

#define SSG_PORTA		(14)
#define SSG_PORTB		(15)

/* register number to channel number , slot offset */
#define OPN_CHAN(N) (N&3)
#define OPN_SLOT(N) ((N>>2)&3)

/* slot number */
#define SLOT1 0
#define SLOT2 2
#define SLOT3 1
#define SLOT4 3

/* bit0 = Right enable , bit1 = Left enable */
#define OUTD_RIGHT  1
#define OUTD_LEFT   2
#define OUTD_CENTER 3

/* struct describing a single operator (SLOT) */
typedef struct {
	s32 *DT; /* detune          :dt_tab[DT] */
	u8 KSR; /* key scale rate  :3-KSR */
	u32 ar; /* attack rate  */
	u32 d1r; /* decay rate   */
	u32 d2r; /* sustain rate */
	u32 rr; /* release rate */
	u8 ksr; /* key scale rate  :kcode>>(3-KSR) */
	u32 mul; /* multiple        :ML_TABLE[ML] */
	/* Phase Generator */u32 phase; /* phase counter */
	u32 Incr; /* phase step */
	/* Envelope Generator */u8 state; /* phase type */
	u32 tl; /* total level: TL << 3 */
	s32 volume; /* envelope counter */
	u32 sl; /* sustain level:sl_table[SL] */
	u32 vol_out; /* current output from EG circuit (without AM from LFO) */
	u8 eg_sh_ar; /*  (attack state) */
	u8 eg_sel_ar; /*  (attack state) */
	u8 eg_sh_d1r; /*  (decay state) */
	u8 eg_sel_d1r; /*  (decay state) */
	u8 eg_sh_d2r; /*  (sustain state) */
	u8 eg_sel_d2r; /*  (sustain state) */
	u8 eg_sh_rr; /*  (release state) */
	u8 eg_sel_rr; /*  (release state) */
	u8 ssg; /* SSG-EG waveform */
	u8 ssgn; /* SSG-EG negated output */
	u32 key; /* 0=last key was KEY OFF, 1=KEY ON */
	/* LFO */u32 AMmask; /* AM enable flag */

} FM_SLOT;

typedef struct {
	FM_SLOT SLOT[4]; /* four SLOTs (operators) */
	u8 ALGO; /* algorithm */
	u8 FB; /* feedback shift */
	u8 ams; /* channel AMS */
	u8 kcode; /* key code:                        */
	s32 op1_out[2]; /* op1 output for feedback */
	s32 *connect1; /* SLOT1 output pointer */
	s32 *connect3; /* SLOT3 output pointer */
	s32 *connect2; /* SLOT2 output pointer */
	s32 *connect4; /* SLOT4 output pointer */
	s32 *mem_connect;/* where to put the delayed sample (MEM) */
	s32 mem_value; /* delayed sample (MEM) value */
	s32 pms; /* channel PMS */
	u32 fc; /* fnum,blk:adjusted to sample rate */
	u32 block_fnum; /* current blk/fnum value for this slot (can be different betweeen slots of one channel in 3slot mode) */
	s32 pan[3];
} FM_CH;

typedef struct {
	int clock; /* master clock  (Hz)   */
	int rate; /* sampling rate (Hz)   */
	double freqbase; /* frequency base       */
	double TimerBase; /* Timer base time      */
#if FM_BUSY_FLAG_SUPPORT
	double BusyExpire; /* ExpireTime of Busy clear */
#endif
	u8 address; /* address register     */
	u8 irq; /* interrupt level      */
	u8 irqmask; /* irq mask             */
	u8 status; /* status flag          */
	u32 mode; /* mode  CSM / 3SLOT    */
	u8 prescaler_sel;/* prescaler selector */
	u8 fn_h; /* freq latch           */
	s32 TA; /* timer a              */
	s32 TAC; /* timer a counter      */
	u8 TB; /* timer b              */
	s32 TBC; /* timer b counter      */
	/* local time tables */s32 dt_tab[8][32];/* DeTune table       */
	/* Extention Timer and IRQ handler */
	FM_TIMERHANDLER Timer_Handler;
	FM_IRQHANDLER IRQ_Handler;
} FM_ST;

/* OPN 3slot struct */
typedef struct {
	u32 fc[3]; /* fnum3,blk3: calculated */
	u8 fn_h; /* freq3 latch */
	u8 kcode[3]; /* key code */
	u32 block_fnum[3]; /* current fnum value for this slot (can be different betweeen slots of one channel in 3slot mode) */
} FM_3SLOT;

/* OPN/A/B common state */
typedef struct {
	FM_ST ST; /* general state */
	FM_3SLOT SL3; /* 3 slot mode state */
	//FM_CH P_CH[6]; /* pointer of CH */
	//unsigned int pan[6 * 2]; /* fm channels output masks (0xffffffff = enable) */
	u32 eg_cnt; /* global envelope generator counter */
	u32 eg_timer; /* global envelope generator counter works at frequency = chipclock/64/3 */
	u32 eg_timer_add; /* step of eg_timer */
	u32 eg_timer_overflow;/* envelope generator timer overlfows every 3 samples (on real chip) */
	/* there are 2048 FNUMs that can be generated using FNUM/BLK registers
	 but LFO works with one more bit of a precision so we really need 4096 elements */u32 fn_table[4096]; /* fnumber->increment counter */
	/* LFO */u32 lfo_cnt;
	u32 lfo_inc;
	u32 lfo_freq[8]; /* LFO FREQ table */
} FM_OPN;

/* SSG struct */
static struct SSG_t {
	int lastEnable;
	u32 step;
	int period[3];
	int PeriodN;
	int PeriodE;
	int count[3];
	int CountN;
	int CountE;
	u32 vol[3];
	u32 VolE;
	u8 envelope[3];
	u8 output[3];
	u8 OutputN;
	s8 count_env;
	u8 hold;
	u8 alternate;
	u8 attack;
	u8 holding;
	int RNG;
	u32 vol_table[32];
	s32 pan[3];
} SSG;

/* ADPCM type A channel struct */
typedef struct {
	u8 flag; /* port state				*/
	u8 flagMask; /* arrived flag mask		*/
	u8 now_data; /* current ROM data			*/
	u32 now_addr; /* current ROM address		*/
	u32 now_step;
	u32 step;
	u32 start; /* sample data start address*/
	u32 end; /* sample data end address	*/
	u8 IL; /* Instrument Level			*/
	//s32 adpcma_acc; /* accumulator				*/
	//s32 adpcma_step; /* step						*/
	s32 adpcma_out; /* (speedup) hiro-shi!!		*/
	s8 vol_mul; /* volume in "0.75dB" steps	*/
	u8 vol_shift; /* volume in "-6dB" steps	*/
	s32 pan[3];
	//s32 *pan; /* &out_adpcma[OPN_xxxx] 	*/
} ADPCMA;

/* ADPCM type B struct */
typedef struct adpcmb_state {
	//s32 *pan; /* pan : &output_pointer[pan]   */
	double freqbase;
	int output_range;
	u32 now_addr; /* current address      */
	u32 now_step; /* currect step         */
	u32 step; /* step                 */
	u32 start; /* start address        */
	u32 limit; /* limit address        */
	u32 end; /* end address          */
	u32 delta; /* delta scale          */
	s32 volume; /* current volume       */
	s32 acc; /* shift Measurement value*/
	s32 adpcmd; /* next Forecast        */
	s32 adpcml; /* current value        */
	s32 prev_acc; /* leveling value       */
	u8 now_data; /* current rom data     */
	u8 CPU_data; /* current data from reg 08 */
	u8 portstate; /* port status          */
	u8 control2; /* control reg: SAMPLE, DA/AD, RAM TYPE (x8bit / x1bit), ROM/RAM */
	u8 portshift; /* address bits shift-left:
	 ** 8 for YM2610,
	 ** 5 for Y8950 and YM2608 */
	u8 DRAMportshift; /* address bits shift-right:
	 ** 0 for ROM and x8bit DRAMs,
	 ** 3 for x1 DRAMs */
	u8 memread; /* needed for reading/writing external memory */
	/* note that different chips have these flags on different
	 ** bits of the status register
	 */
	u8 status_change_EOS_bit; /* 1 on End Of Sample (record/playback/cycle time of AD/DA converting has passed)*/
	u8 status_change_BRDY_bit; /* 1 after recording 2 datas (2x4bits) or after reading/writing 1 data */
	/* neither Y8950 nor YM2608 can generate IRQ when PCMBSY bit changes, so instead of above,
	 ** the statusflag gets ORed with PCM_BSY (below) (on each read of statusflag of Y8950 and YM2608)
	 */
	u8 PCM_BSY; /* 1 when ADPCM is playing; Y8950/YM2608 only */
	s32 pan[3];
} ADPCMB;

/* here's the virtual YM2610 */
static struct ym2610_t {
	u8 regs[512]; /* registers            */
	FM_OPN OPN; /* OPN state            */
	FM_CH CH[6]; /* channel state        */
	u8 addr_A1; /* address line A1      */
	/* ADPCM-A unit */u8 adpcmaTL; /* adpcmA total level   */
	ADPCMA adpcma[6]; /* adpcm channels       */
	u8 adpcm_arrivedEndAddress;

	/* ADPCM-B unit */
	ADPCMB adpcmb; /* Delta-T ADPCM unit   */

}ALIGN_DATA YM2610;

/* current chip state */
static s32 m2, c1, c2; /* Phase Modulation input for operators 2,3,4 */
static s32 mem; /* one sample delay memory */

static s32 ALIGN_DATA out_fm[8]; /* outputs of working channels */
static s32 out_ssg; /* channel output CHENTER only for SSG */
static s32 ALIGN_DATA out_adpcma[4]; /* channel output NONE,LEFT,RIGHT or CENTER for YM2608/YM2610 ADPCM */
static s32 ALIGN_DATA out_delta[4]; /* channel output NONE,LEFT,RIGHT or CENTER for YM2608/YM2610 DELTAT*/

static u32 LFO_AM; /* runtime LFO calculations helper */
static s32 LFO_PM; /* runtime LFO calculations helper */

/* log output level */
#define LOG_ERR  3      /* ERROR       */
#define LOG_WAR  2      /* WARNING     */
#define LOG_INF  1      /* INFORMATION */
#define LOG_LEVEL LOG_INF

#define LOG(n,x) if((n)>=LOG_LEVEL ) logerror x

/*********************************************************************************************/

/* status set and IRQ handling */
static inline void FM_STATUS_SET(int flag) {
	/* set status flag */
	YM2610.OPN.ST.status |= flag;
	if (!(YM2610.OPN.ST.irq) && (YM2610.OPN.ST.status & YM2610.OPN.ST.irqmask)) {
		YM2610.OPN.ST.irq = 1;
		/* callback user interrupt handler (IRQ is OFF to ON) */
		if(YM2610.OPN.ST.IRQ_Handler) YM2610.OPN.ST.IRQ_Handler(1);
	}
}

/* status reset and IRQ handling */
static inline void FM_STATUS_RESET(int flag) {
	/* reset status flag */
	YM2610.OPN.ST.status &= ~flag;
	if ((YM2610.OPN.ST.irq) && !(YM2610.OPN.ST.status & YM2610.OPN.ST.irqmask)) {
		YM2610.OPN.ST.irq = 0;
		/* callback user interrupt handler (IRQ is ON to OFF) */
		if(YM2610.OPN.ST.IRQ_Handler) YM2610.OPN.ST.IRQ_Handler(0);
	}
}

/* IRQ mask set */
static inline void FM_IRQMASK_SET(int flag) {
	YM2610.OPN.ST.irqmask = flag;
	/* IRQ handling check */
	FM_STATUS_SET(0);
	FM_STATUS_RESET(0);
}

/* OPN Mode Register Write */
static inline void set_timers(int v) {
	/* b7 = CSM MODE */
	/* b6 = 3 slot mode */
	/* b5 = reset b */
	/* b4 = reset a */
	/* b3 = timer enable b */
	/* b2 = timer enable a */
	/* b1 = load b */
	/* b0 = load a */
	YM2610.OPN.ST.mode = v;

	/* reset Timer b flag */
	if (v & 0x20) FM_STATUS_RESET(0x02);
	/* reset Timer a flag */
	if (v & 0x10) FM_STATUS_RESET(0x01);
	/* load b */
	if (v & 0x02) {
		if (YM2610.OPN.ST.TBC == 0) {
			YM2610.OPN.ST.TBC = (256 - YM2610.OPN.ST.TB) << 4;
			/* External timer handler */
#if FM_INTERNAL_TIMER==0
			if(YM2610.OPN.ST.Timer_Handler)
				YM2610.OPN.ST.Timer_Handler(1, YM2610.OPN.ST.TBC, YM2610.OPN.ST.TimerBase);
#endif
		}
	} else { /* stop timer b */
		if (YM2610.OPN.ST.TBC != 0) {
			YM2610.OPN.ST.TBC = 0;
#if FM_INTERNAL_TIMER==0
			if(YM2610.OPN.ST.Timer_Handler)
				YM2610.OPN.ST.Timer_Handler(1, 0, YM2610.OPN.ST.TimerBase);
#endif
		}
	}
	/* load a */
	if (v & 0x01) {
		if (YM2610.OPN.ST.TAC == 0) {
			YM2610.OPN.ST.TAC = (1024 - YM2610.OPN.ST.TA);
			/* External timer handler */
#if FM_INTERNAL_TIMER==0
			if(YM2610.OPN.ST.Timer_Handler)
				YM2610.OPN.ST.Timer_Handler(0, YM2610.OPN.ST.TAC, YM2610.OPN.ST.TimerBase);
#endif
		}
	} else { /* stop timer a */
		if (YM2610.OPN.ST.TAC != 0) {
			YM2610.OPN.ST.TAC = 0;
#if FM_INTERNAL_TIMER==0
			if(YM2610.OPN.ST.Timer_Handler)
				YM2610.OPN.ST.Timer_Handler(0, 0, YM2610.OPN.ST.TimerBase);
#endif
		}
	}
}

/* Timer A Overflow */
static inline void TimerAOver() {
	/* set status (if enabled) */
	if (YM2610.OPN.ST.mode & 0x04) FM_STATUS_SET(0x01);
	/* clear or reload the counter */
	YM2610.OPN.ST.TAC = (1024 - YM2610.OPN.ST.TA);
#if FM_INTERNAL_TIMER==0
	if(YM2610.OPN.ST.Timer_Handler)
		YM2610.OPN.ST.Timer_Handler(0, YM2610.OPN.ST.TAC, YM2610.OPN.ST.TimerBase);
#endif
}
/* Timer B Overflow */
static inline void TimerBOver() {
	/* set status (if enabled) */
	if (YM2610.OPN.ST.mode & 0x08) FM_STATUS_SET(0x02);
	/* clear or reload the counter */
	YM2610.OPN.ST.TBC = (256 - YM2610.OPN.ST.TB) << 4;
#if FM_INTERNAL_TIMER==0
	if(YM2610.OPN.ST.Timer_Handler)
		YM2610.OPN.ST.Timer_Handler(1, YM2610.OPN.ST.TBC, YM2610.OPN.ST.TimerBase);
#endif
}

#if FM_INTERNAL_TIMER
/* ----- internal timer mode , update timer */

/* ---------- calculate timer A ---------- */
#define INTERNAL_TIMER_A(CSM_CH) { \
		if(ST.TAC /*&&  (.Timer_Handler==0) */)		\
			/*if((.TAC -= (int)(ST.freqbase*4096)) <= 0 )*/	\
			if((ST.TAC -= (4096000 / ST.rate) <= 0 ) {	\
				TimerAOver();						\
				/* CSM mode total level latch and auto key on */	\
				if(ST.mode & 0x80 )	CSMKeyControll(CSM_CH );			\
			}											\
	}
/* ---------- calculate timer B ---------- */
#define INTERNAL_TIMER_B(step) {					\
		if(ST.TBC /*&& (.Timer_Handler==0) */)				\
			/*if((.TBC -= (int)(.freqbase*4096*step)) <= 0 )*/	\
			if((ST.TBC -= ((4096000 / ST.rate) * step) <= 0 ) TimerBOver( ); \
	}
#else /* FM_INTERNAL_TIMER */
/* external timer mode */
#define INTERNAL_TIMER_A(CSM_CH)
#define INTERNAL_TIMER_B(step)
#endif /* FM_INTERNAL_TIMER */

#if FM_BUSY_FLAG_SUPPORT
static inline u8 FM_STATUS_FLAG() {
	if (YM2610.OPN.ST.BusyExpire) {
		if ((YM2610.OPN.ST.BusyExpire - FM_GET_TIME_NOW()) > 0)
			return YM2610.OPN.ST.status | 0x80; /* with busy */
		/* expire */
		YM2610.OPN.ST.BusyExpire = 0;
	}
	return YM2610.OPN.ST.status;
}
static inline void FM_BUSY_SET(int busyclock) {
	YM2610.OPN.ST.BusyExpire = FM_GET_TIME_NOW() + (YM2610.OPN.ST.TimerBase * busyclock);
}
#define FM_BUSY_CLEAR() (YM2610.OPN.ST.BusyExpire = 0)
#else
#define FM_STATUS_FLAG() (YM2610.OPN.ST.status)
#define FM_BUSY_SET(bclock) {}
#define FM_BUSY_CLEAR() {}
#endif

static inline void FM_KEYON(FM_CH *CH, int s) {
	FM_SLOT *SLOT = &CH->SLOT[s];
	if (!SLOT->key) {
		SLOT->key = 1;
		SLOT->phase = 0; /* restart Phase Generator */
		SLOT->state = EG_ATT; /* phase -> Attack */
	}
}

static inline void FM_KEYOFF(FM_CH *CH, int s) {
	FM_SLOT *SLOT = &CH->SLOT[s];
	if (SLOT->key) {
		SLOT->key = 0;
		if (SLOT->state > EG_REL)
			SLOT->state = EG_REL;/* phase -> Release */
	}
}

/* set algorithm connection */
static void setup_connection(FM_CH *CH, int ch) {
	s32 *carrier = &out_fm[ch];

	s32 **om1 = &CH->connect1;
	s32 **om2 = &CH->connect3;
	s32 **oc1 = &CH->connect2;

	s32 **memc = &CH->mem_connect;

	switch (CH->ALGO) {
	case 0:
		/* M1---C1---MEM---M2---C2---OUT */
		*om1 = &c1;
		*oc1 = &mem;
		*om2 = &c2;
		*memc = &m2;
		break;
	case 1:
		/* M1------+-MEM---M2---C2---OUT */
		/*      C1-+                     */
		*om1 = &mem;
		*oc1 = &mem;
		*om2 = &c2;
		*memc = &m2;
		break;
	case 2:
		/* M1-----------------+-C2---OUT */
		/*      C1---MEM---M2-+          */
		*om1 = &c2;
		*oc1 = &mem;
		*om2 = &c2;
		*memc = &m2;
		break;
	case 3:
		/* M1---C1---MEM------+-C2---OUT */
		/*                 M2-+          */
		*om1 = &c1;
		*oc1 = &mem;
		*om2 = &c2;
		*memc = &c2;
		break;
	case 4:
		/* M1---C1-+-OUT */
		/* M2---C2-+     */
		/* MEM: not used */
		*om1 = &c1;
		*oc1 = carrier;
		*om2 = &c2;
		*memc = &mem; /* store it anywhere where it will not be used */
		break;
	case 5:
		/*    +----C1----+     */
		/* M1-+-MEM---M2-+-OUT */
		/*    +----C2----+     */
		*om1 = 0; /* special mark */
		*oc1 = carrier;
		*om2 = carrier;
		*memc = &m2;
		break;
	case 6:
		/* M1---C1-+     */
		/*      M2-+-OUT */
		/*      C2-+     */
		/* MEM: not used */
		*om1 = &c1;
		*oc1 = carrier;
		*om2 = carrier;
		*memc = &mem; /* store it anywhere where it will not be used */
		break;
	case 7:
		/* M1-+     */
		/* C1-+-OUT */
		/* M2-+     */
		/* C2-+     */
		/* MEM: not used*/
		*om1 = carrier;
		*oc1 = carrier;
		*om2 = carrier;
		*memc = &mem; /* store it anywhere where it will not be used */
		break;
	}

	CH->connect4 = carrier;
}

/* set detune & multiple */static inline void set_det_mul(FM_CH *CH,
		FM_SLOT *SLOT, int v) {
	SLOT->mul = (v & 0x0f) ? (v & 0x0f) * 2 : 1;
	SLOT->DT = YM2610.OPN.ST.dt_tab[(v >> 4) & 7];
	CH->SLOT[SLOT1].Incr = -1;
}

/* set total level */static inline void set_tl(FM_CH *CH, FM_SLOT *SLOT, int v) {
	SLOT->tl = (v & 0x7f) << (ENV_BITS - 7); /* 7bit TL */
}

/* set attack rate & key scale  */
static inline void set_ar_ksr(FM_CH *CH, FM_SLOT *SLOT, int v) {
	u8 old_KSR = SLOT->KSR;

	SLOT->ar = (v & 0x1f) ? 32 + ((v & 0x1f) << 1) : 0;

	SLOT->KSR = 3 - (v >> 6);
	if (SLOT->KSR != old_KSR) {
		CH->SLOT[SLOT1].Incr = -1;
	} else {
		/* refresh Attack rate */
		if ((SLOT->ar + SLOT->ksr) < 32 + 62) {
			SLOT->eg_sh_ar = eg_rate_shift[SLOT->ar + SLOT->ksr];
			SLOT->eg_sel_ar = eg_rate_select[SLOT->ar + SLOT->ksr];
		} else {
			SLOT->eg_sh_ar = 0;
			SLOT->eg_sel_ar = 17 * RATE_STEPS;
		}
	}
}

/* set decay rate */static inline void set_dr(FM_SLOT *SLOT, int v) {
	SLOT->d1r = (v & 0x1f) ? 32 + ((v & 0x1f) << 1) : 0;

	SLOT->eg_sh_d1r = eg_rate_shift[SLOT->d1r + SLOT->ksr];
	SLOT->eg_sel_d1r = eg_rate_select[SLOT->d1r + SLOT->ksr];

}

/* set sustain rate */static inline void set_sr(FM_SLOT *SLOT, int v) {
	SLOT->d2r = (v & 0x1f) ? 32 + ((v & 0x1f) << 1) : 0;

	SLOT->eg_sh_d2r = eg_rate_shift[SLOT->d2r + SLOT->ksr];
	SLOT->eg_sel_d2r = eg_rate_select[SLOT->d2r + SLOT->ksr];
}

/* set release rate */static inline void set_sl_rr(FM_SLOT *SLOT, int v) {
	SLOT->sl = sl_table[v >> 4];

	SLOT->rr = 34 + ((v & 0x0f) << 2);

	SLOT->eg_sh_rr = eg_rate_shift[SLOT->rr + SLOT->ksr];
	SLOT->eg_sel_rr = eg_rate_select[SLOT->rr + SLOT->ksr];
}

static inline int op_calc(unsigned phase, unsigned env, int pm) {
	int a1 = (int)(phase >> 16) + pm, d1 = a1 & 511;
	int d0 = (d1 * (512 - d1)) >> (4 + (env >> 6));
	return (a1 & 512) ? -d0 : d0;
// 	short d1 = ((((int)(phase >> 16) + pm) + 512) & 1023) - 512;
// 	return (d1 * (((d1 < 0) ? d1 : -d1) + 512)) >> (4 + (env >> 6));
}

static inline int op_calc1(unsigned phase, unsigned env, int pm) {
	int a1 = ((int)phase + pm) >> 16, d1 = a1 & 511;
	int d0 = (d1 * (512 - d1)) >> (4 + (env >> 6));
	return (a1 & 512) ? -d0 : d0;
// 	short d1 = (((((int)phase + pm) >> 16) + 512) & 1023) - 512;
// 	return (d1 * (((d1 < 0) ? d1 : -d1) + 512)) >> (4 + (env >> 6));
}

/* advance LFO to next sample */
static inline void advance_lfo(void) {
	u8 pos;
	u8 prev_pos;

	if (YM2610.OPN.lfo_inc) /* LFO enabled ? */
	{
		prev_pos = YM2610.OPN.lfo_cnt >> LFO_SH & 127;

		YM2610.OPN.lfo_cnt += YM2610.OPN.lfo_inc;

		pos = (YM2610.OPN.lfo_cnt >> LFO_SH) & 127;

		/* update AM when LFO output changes */

		/*if (prev_pos != pos)*/
		/* actually I can't optimize is this way without rewritting chan_calc()
		 to use chip->lfo_am instead of global lfo_am */
		{

			/* triangle */
			/* AM: 0 to 126 step +2, 126 to 0 step -2 */
			if (pos < 64)
				LFO_AM = (pos & 63) * 2;
			else
				LFO_AM = 126 - ((pos & 63) * 2);
		}

		/* PM works with 4 times slower clock */
		prev_pos >>= 2;
		pos >>= 2;
		/* update PM when LFO output changes */
		/*if (prev_pos != pos)*//* can't use global lfo_pm for this optimization, must be chip->lfo_pm instead*/
		{
			LFO_PM = pos;
		}

	} else {
		LFO_AM = 0;
		LFO_PM = 0;
	}
}

static inline void advance_eg_channel(FM_SLOT *SLOT) {
	unsigned int out;
	unsigned int swap_flag = 0;
	unsigned int i = 4; /* four operators per channel */

	do {	switch (SLOT->state) {
		case EG_ATT: /* attack phase */
			if (!(YM2610.OPN.eg_cnt & ((1 << SLOT->eg_sh_ar) - 1))) {
				SLOT->volume += (~SLOT->volume
						* (eg_inc[SLOT->eg_sel_ar
								+ ((YM2610.OPN.eg_cnt >> SLOT->eg_sh_ar) & 7)])) >> 4;
				if (SLOT->volume <= MIN_ATT_INDEX) {
					SLOT->volume = MIN_ATT_INDEX;
					SLOT->state = EG_DEC;
				}
			}
			break;

		case EG_DEC: /* decay phase */
			if (SLOT->ssg & 0x08) /* SSG EG type envelope selected */
			{
				if (!(YM2610.OPN.eg_cnt & ((1 << SLOT->eg_sh_d1r) - 1))) {
					SLOT->volume += (eg_inc[SLOT->eg_sel_d1r
							+ ((YM2610.OPN.eg_cnt >> SLOT->eg_sh_d1r) & 7)] << 2);

					if (SLOT->volume >= SLOT->sl)
						SLOT->state = EG_SUS;
				}
			} else {
				if (!(YM2610.OPN.eg_cnt & ((1 << SLOT->eg_sh_d1r) - 1))) {
					SLOT->volume += eg_inc[SLOT->eg_sel_d1r
							+ ((YM2610.OPN.eg_cnt >> SLOT->eg_sh_d1r) & 7)];

					if (SLOT->volume >= SLOT->sl)
						SLOT->state = EG_SUS;
				}
			}
			break;

		case EG_SUS: /* sustain phase */
			if (SLOT->ssg & 0x08) /* SSG EG type envelope selected */
			{
				if (!(YM2610.OPN.eg_cnt & ((1 << SLOT->eg_sh_d2r) - 1))) {
					SLOT->volume += (eg_inc[SLOT->eg_sel_d2r
							+ ((YM2610.OPN.eg_cnt >> SLOT->eg_sh_d2r) & 7)] << 2);

					if (SLOT->volume >= MAX_ATT_INDEX) {
						SLOT->volume = MAX_ATT_INDEX;

						if (SLOT->ssg & 0x01) /* bit 0 = hold */
						{
							if (SLOT->ssgn & 1) /* have we swapped once ??? */
							{
								/* yes, so do nothing, just hold current level */
							} else
								swap_flag = (SLOT->ssg & 0x02) | 1; /* bit 1 = alternate */

						} else {
							/* same as KEY-ON operation */

							/* restart of the Phase Generator should be here,
							 only if AR is not maximum ??? */
							/*SLOT->phase = 0;*/

							/* phase -> Attack */
							SLOT->state = EG_ATT;

							swap_flag = (SLOT->ssg & 0x02); /* bit 1 = alternate */
						}
					}
				}
			} else {
				if (!(YM2610.OPN.eg_cnt & ((1 << SLOT->eg_sh_d2r) - 1))) {
					SLOT->volume += eg_inc[SLOT->eg_sel_d2r
							+ ((YM2610.OPN.eg_cnt >> SLOT->eg_sh_d2r) & 7)];

					if (SLOT->volume >= MAX_ATT_INDEX) {
						SLOT->volume = MAX_ATT_INDEX;
						/* do not change SLOT->state (verified on real chip) */
					}
				}

			}
			break;

		case EG_REL: /* release phase */
			if (!(YM2610.OPN.eg_cnt & ((1 << SLOT->eg_sh_rr) - 1))) {
				SLOT->volume += eg_inc[SLOT->eg_sel_rr
						+ ((YM2610.OPN.eg_cnt >> SLOT->eg_sh_rr) & 7)];

				if (SLOT->volume >= MAX_ATT_INDEX) {
					SLOT->volume = MAX_ATT_INDEX;
					SLOT->state = EG_OFF;
				}
			}
			break;

		}

		out = SLOT->tl + ((u32) SLOT->volume);

		if ((SLOT->ssg & 0x08) && (SLOT->ssgn & 2)) /* negate output (changes come from alternate bit, init comes from attack bit) */
			out ^= ((1 << ENV_BITS) - 1); /* 1023 */

		/* we need to store the result here because we are going to change ssgn
		 in next instruction */
		SLOT->vol_out = out;

		SLOT->ssgn ^= swap_flag;

		SLOT++;
	} while (--i);

}

#define volume_calc(OP) ((OP)->vol_out + (AM & (OP)->AMmask))

static inline void chan_calc(FM_CH *CH) {
	unsigned int eg_out;

	u32 AM = LFO_AM >> CH->ams;

	m2 = c1 = c2 = mem = 0;

	*CH->mem_connect = CH->mem_value; /* restore delayed sample (MEM) value to m2 or c2 */

	eg_out = volume_calc(&CH->SLOT[SLOT1]);
	{
		s32 out = CH->op1_out[0] + CH->op1_out[1];
		CH->op1_out[0] = CH->op1_out[1];

		if (!CH->connect1) {
			/* algorithm 5  */
			mem = c1 = c2 = CH->op1_out[0];
		} else {
			/* other algorithms */
			*CH->connect1 += CH->op1_out[0];
		}

		CH->op1_out[1] = 0;
		if (eg_out < ENV_QUIET) /* SLOT 1 */
		{
			if (!CH->FB)
				out = 0;

			CH->op1_out[1] = op_calc1(CH->SLOT[SLOT1].phase, eg_out,
					(out << CH->FB));
		}
	}

	eg_out = volume_calc(&CH->SLOT[SLOT3]);
	if (eg_out < ENV_QUIET) /* SLOT 3 */
		*CH->connect3 += op_calc(CH->SLOT[SLOT3].phase, eg_out, m2);

	eg_out = volume_calc(&CH->SLOT[SLOT2]);
	if (eg_out < ENV_QUIET) /* SLOT 2 */
		*CH->connect2 += op_calc(CH->SLOT[SLOT2].phase, eg_out, c1);

	eg_out = volume_calc(&CH->SLOT[SLOT4]);
	if (eg_out < ENV_QUIET) /* SLOT 4 */
		*CH->connect4 += op_calc(CH->SLOT[SLOT4].phase, eg_out, c2);

	/* store current MEM */
	CH->mem_value = mem;

	/* update phase counters AFTER output calculations */
	if (CH->pms) {

		/* add support for 3 slot mode */

		u32 block_fnum = CH->block_fnum;

		u32 fnum_lfo = ((block_fnum & 0x7f0) >> 4) * 32 * 8;
		s32 lfo_fn_table_index_offset =
				lfo_pm_table[fnum_lfo + CH->pms + LFO_PM];

		if (lfo_fn_table_index_offset) /* LFO phase modulation active */
		{
			u8 blk;
			u32 fn;
			int kc, fc;

			block_fnum = block_fnum * 2 + lfo_fn_table_index_offset;

			blk = (block_fnum & 0x7000) >> 12;
			fn = block_fnum & 0xfff;

			/* keyscale code */
			kc = (blk << 2) | opn_fktable[fn >> 8];
			/* phase increment counter */
			fc = YM2610.OPN.fn_table[fn] >> (7 - blk);

			CH->SLOT[SLOT1].phase += ((fc + CH->SLOT[SLOT1].DT[kc])
					* CH->SLOT[SLOT1].mul) >> 1;
			CH->SLOT[SLOT2].phase += ((fc + CH->SLOT[SLOT2].DT[kc])
					* CH->SLOT[SLOT2].mul) >> 1;
			CH->SLOT[SLOT3].phase += ((fc + CH->SLOT[SLOT3].DT[kc])
					* CH->SLOT[SLOT3].mul) >> 1;
			CH->SLOT[SLOT4].phase += ((fc + CH->SLOT[SLOT4].DT[kc])
					* CH->SLOT[SLOT4].mul) >> 1;
		} else /* LFO phase modulation  = zero */
		{
			CH->SLOT[SLOT1].phase += CH->SLOT[SLOT1].Incr;
			CH->SLOT[SLOT2].phase += CH->SLOT[SLOT2].Incr;
			CH->SLOT[SLOT3].phase += CH->SLOT[SLOT3].Incr;
			CH->SLOT[SLOT4].phase += CH->SLOT[SLOT4].Incr;
		}
	} else /* no LFO phase modulation */
	{
		CH->SLOT[SLOT1].phase += CH->SLOT[SLOT1].Incr;
		CH->SLOT[SLOT2].phase += CH->SLOT[SLOT2].Incr;
		CH->SLOT[SLOT3].phase += CH->SLOT[SLOT3].Incr;
		CH->SLOT[SLOT4].phase += CH->SLOT[SLOT4].Incr;
	}
}

/* update phase increment and envelope generator */
static inline void refresh_fc_eg_slot(FM_SLOT *SLOT, int fc, int kc) {
	int ksr;

	/* (frequency) phase increment counter */
	SLOT->Incr = ((fc + SLOT->DT[kc]) * SLOT->mul) >> 1;

	ksr = kc >> SLOT->KSR;
	if (SLOT->ksr != ksr) {
		SLOT->ksr = ksr;

		/* calculate envelope generator rates */
		if ((SLOT->ar + SLOT->ksr) < 32 + 62) {
			SLOT->eg_sh_ar = eg_rate_shift[SLOT->ar + SLOT->ksr];
			SLOT->eg_sel_ar = eg_rate_select[SLOT->ar + SLOT->ksr];
		} else {
			SLOT->eg_sh_ar = 0;
			SLOT->eg_sel_ar = 17 * RATE_STEPS;
		}

		SLOT->eg_sh_d1r = eg_rate_shift[SLOT->d1r + SLOT->ksr];
		SLOT->eg_sel_d1r = eg_rate_select[SLOT->d1r + SLOT->ksr];

		SLOT->eg_sh_d2r = eg_rate_shift[SLOT->d2r + SLOT->ksr];
		SLOT->eg_sel_d2r = eg_rate_select[SLOT->d2r + SLOT->ksr];

		SLOT->eg_sh_rr = eg_rate_shift[SLOT->rr + SLOT->ksr];
		SLOT->eg_sel_rr = eg_rate_select[SLOT->rr + SLOT->ksr];
	}
}

/* update phase increment counters */
static inline void refresh_fc_eg_chan(FM_CH *CH) {
	if (CH->SLOT[SLOT1].Incr == -1) {
		int fc = CH->fc;
		int kc = CH->kcode;
		refresh_fc_eg_slot(&CH->SLOT[SLOT1], fc, kc);
		refresh_fc_eg_slot(&CH->SLOT[SLOT2], fc, kc);
		refresh_fc_eg_slot(&CH->SLOT[SLOT3], fc, kc);
		refresh_fc_eg_slot(&CH->SLOT[SLOT4], fc, kc);
	}
}

/* initialize time tables */
static void init_timetables(const u8 *dttable) {
	int i, d;
	double rate;

#if 0
	printf("FM.C: samplerate=%8i chip clock=%8i  freqbase=%f  \n",
			YM2610.OPN.ST.rate, YM2610.OPN.ST.clock, YM2610.OPN.ST.freqbase );
#endif

	/* DeTune table */
	for (d = 0; d <= 3; d++) {
		for (i = 0; i <= 31; i++) {
			rate = ((double) dttable[d * 32 + i]) * SIN_LEN * YM2610.OPN.ST.freqbase
					* (1 << FREQ_SH) / ((double) (1 << 20));
			YM2610.OPN.ST.dt_tab[d][i] = (s32) rate;
			YM2610.OPN.ST.dt_tab[d + 4][i] = -YM2610.OPN.ST.dt_tab[d][i];
#if 0
			logerror("FM.C: DT [%2i %2i] = %8x  \n", d, i, YM2610.OPN.ST.dt_tab[d][i] );
#endif
		}
	}

}

static void reset_channels(FM_CH *CH, int num) {
	int c, s;

	YM2610.OPN.ST.mode = 0; /* normal mode */
	YM2610.OPN.ST.TA = 0;
	YM2610.OPN.ST.TAC = 0;
	YM2610.OPN.ST.TB = 0;
	YM2610.OPN.ST.TBC = 0;

	for (c = 0; c < num; c++) {
		CH[c].fc = 0;
		CH[c].pan[0] = CH[c].pan[1] = 0;
		CH[c].pan[2] = ~0;
		
		for (s = 0; s < 4; s++) {
			CH[c].SLOT[s].ssg = 0;
			CH[c].SLOT[s].ssgn = 0;
			CH[c].SLOT[s].state = EG_OFF;
			CH[c].SLOT[s].volume = MAX_ATT_INDEX;
			CH[c].SLOT[s].vol_out = MAX_ATT_INDEX;
		}
	}
}

/* initialize generic tables */
static void OPNInitTable(void) {
	signed int i, x;
	signed int n;
	double o, m;



	/*logerror("FM.C: ENV_QUIET= %08x\n",ENV_QUIET );*/

	/* build LFO PM modulation table */
	for (i = 0; i < 8; i++) /* 8 PM depths */
	{
		u8 fnum;
		for (fnum = 0; fnum < 128; fnum++) /* 7 bits meaningful of F-NUMBER */
		{
			u8 value;
			u8 step;
			u32 offset_depth = i;
			u32 offset_fnum_bit;
			u32 bit_tmp;

			for (step = 0; step < 8; step++) {
				value = 0;
				for (bit_tmp = 0; bit_tmp < 7; bit_tmp++) /* 7 bits */
				{
					if (fnum & (1 << bit_tmp)) /* only if bit "bit_tmp" is set */
					{
						offset_fnum_bit = bit_tmp * 8;
						value +=
								lfo_pm_output[offset_fnum_bit + offset_depth][step];
					}
				}
				lfo_pm_table[(fnum * 32 * 8) + (i * 32) + step + 0] = value;
				lfo_pm_table[(fnum * 32 * 8) + (i * 32) + (step ^ 7) + 8] =
						value;
				lfo_pm_table[(fnum * 32 * 8) + (i * 32) + step + 16] = -value;
				lfo_pm_table[(fnum * 32 * 8) + (i * 32) + (step ^ 7) + 24] =
						-value;
			}
		}
	}
}

/* CSM Key Controll */
static inline void CSMKeyControll(FM_CH *CH) {
	/* this is wrong, atm */

	/* all key on */
	FM_KEYON(CH, SLOT1);
	FM_KEYON(CH, SLOT2);
	FM_KEYON(CH, SLOT3);
	FM_KEYON(CH, SLOT4);
}

/* prescaler set (and make time tables) */
static void OPNSetPres(int pres, int TimerPres, int SSGpres) {
	int i;

	/* frequency base */
	YM2610.OPN.ST.freqbase =
			(YM2610.OPN.ST.rate) ? ((double) YM2610.OPN.ST.clock / YM2610.OPN.ST.rate) / pres : 0;

	YM2610.OPN.ST.freqbase *= 2.0;
#if 0
	YM2610.OPN.ST.rate = (double)YM2610.OPN.ST.clock / pres;
	YM2610.OPN.ST.freqbase = 1.0f;
#endif

	YM2610.OPN.eg_timer_add = (1 << EG_SH) * YM2610.OPN.ST.freqbase;
	YM2610.OPN.eg_timer_overflow = (3) * (1 << EG_SH);

	/* Timer base time */
	YM2610.OPN.ST.TimerBase = 1.0f / ((double) YM2610.OPN.ST.clock / (double) TimerPres);

	/* SSG part  prescaler set */
	if (SSGpres)
		SSG.step = ((double) SSG_STEP * YM2610.OPN.ST.rate * 8.0)
				/ (YM2610.OPN.ST.clock * 2 / SSGpres);

	/* make time tables */
	init_timetables(dt_tab);

	/* there are 2048 FNUMs that can be generated using FNUM/BLK registers
	 but LFO works with one more bit of a precision so we really need 4096 elements */
	/* calculate fnumber -> increment counter table */
	for (i = 0; i < 4096; i++) {
		/* freq table for octave 7 */
		/* OPN phase increment counter = 20bit */
		YM2610.OPN.fn_table[i] = (u32) ((double) i * 32 * YM2610.OPN.ST.freqbase
				* (1 << (FREQ_SH - 10))); /* -10 because chip works with 10.10 fixed point, while we use 16.16 */
#if 0
		logerror("FM.C: fn_table[%4i] = %08x (dec=%8i)\n",
				i, YM2610.OPN.fn_table[i]>>6,YM2610.OPN.fn_table[i]>>6 );
#endif
	}

	/* LFO freq. table */
	for (i = 0; i < 8; i++) {
		/* Amplitude modulation: 64 output levels (triangle waveform); 1 level lasts for one of "lfo_samples_per_step" samples */
		/* Phase modulation: one entry from lfo_pm_output lasts for one of 4 * "lfo_samples_per_step" samples  */
		YM2610.OPN.lfo_freq[i] = (1.0 / lfo_samples_per_step[i]) * (1 << LFO_SH)
				* YM2610.OPN.ST.freqbase;
#if 0
		logerror("FM.C: lfo_freq[%i] = %08x (dec=%8i)\n",
				i, YM2610.OPN.lfo_freq[i],YM2610.OPN.lfo_freq[i] );
#endif
	}
}

/* write a OPN mode register 0x20-0x2f */
static void OPNWriteMode(int r, int v) {
	u8 c;
	FM_CH *CH;

	switch (r) {
	case 0x21: /* Test */
		break;
	case 0x22: /* LFO FREQ (YM2608/YM2610/YM2610B/YM2612) */
		if (v & 0x08) /* LFO enabled ? */
		{
			YM2610.OPN.lfo_inc = YM2610.OPN.lfo_freq[v & 7];
		} else {
			YM2610.OPN.lfo_inc = 0;
		}
		break;
	case 0x24: /* timer A High 8*/
		YM2610.OPN.ST.TA = (YM2610.OPN.ST.TA & 0x03) | (((int) v) << 2);
		break;
	case 0x25: /* timer A Low 2*/
		YM2610.OPN.ST.TA = (YM2610.OPN.ST.TA & 0x3fc) | (v & 3);
		break;
	case 0x26: /* timer B */
		YM2610.OPN.ST.TB = v;
		break;
	case 0x27: /* mode, timer control */
		set_timers(v);
		break;
	case 0x28: /* key on / off */
		c = v & 0x03;
		if (c == 3) break;
		if (v & 0x04) c += 3;
			
		CH = &YM2610.CH[c];
		
		if (v & 0x10) FM_KEYON(CH, SLOT1);
		else FM_KEYOFF(CH, SLOT1);
		
		if (v & 0x20) FM_KEYON(CH, SLOT2);
		else FM_KEYOFF(CH, SLOT2);
		
		if (v & 0x40) FM_KEYON(CH, SLOT3);
		else FM_KEYOFF(CH, SLOT3);
		
		if (v & 0x80) FM_KEYON(CH, SLOT4);
		else FM_KEYOFF(CH, SLOT4);
		break;
	}
}

/* write a OPN register (0x30-0xff) */
static void OPNWriteReg(int r, int v) {
	FM_CH *CH;
	FM_SLOT *SLOT;

	u8 c = OPN_CHAN(r);

	if (c == 3)
		return; /* 0xX3,0xX7,0xXB,0xXF */

	if (r >= 0x100)
		c += 3;

	CH = &YM2610.CH[c];

	SLOT = &(CH->SLOT[OPN_SLOT(r)]);

	switch (r & 0xf0) {
	case 0x30: /* DET , MUL */
		set_det_mul(CH, SLOT, v);
		break;

	case 0x40: /* TL */
		set_tl(CH, SLOT, v);
		break;

	case 0x50: /* KS, AR */
		set_ar_ksr(CH, SLOT, v);
		break;

	case 0x60: /* bit7 = AM ENABLE, DR */
		set_dr(SLOT, v);
		SLOT->AMmask = (v & 0x80) ? ~0 : 0;
		break;

	case 0x70: /*     SR */
		set_sr(SLOT, v);
		break;

	case 0x80: /* SL, RR */
		set_sl_rr(SLOT, v);
		break;

	case 0x90: /* SSG-EG */

		SLOT->ssg = v & 0x0f;
		SLOT->ssgn = (v & 0x04) >> 1; /* bit 1 in ssgn = attack */

		/* SSG-EG envelope shapes :

		 E AtAlH
		 1 0 0 0  \\\\

        1 0 0 1  \___

		 1 0 1 0  \/\/
		 ___
		 1 0 1 1
		 1 1 0 0  ////
		 ___
		 1 1 0 1  /

		 1 1 1 0  /\/
		 1 1 1 1  /___


		 E = SSG-EG enable


		 The shapes are generated using Attack, Decay and Sustain phases.

		 Each single character in the diagrams above represents this whole
		 sequence:

		 - when KEY-ON = 1, normal Attack phase is generated (*without* any
		 difference when compared to normal mode),

		 - later, when envelope level reaches minimum level (max volume),
		 the EG switches to Decay phase (which works with bigger steps
		 when compared to normal mode - see below),

		 - later when envelope level passes the SL level,
		 the EG swithes to Sustain phase (which works with bigger steps
		 when compared to normal mode - see below),

		 - finally when envelope level reaches maximum level (min volume),
		 the EG switches to Attack phase again (depends on actual waveform).

		 Important is that when switch to Attack phase occurs, the phase counter
		 of that operator will be zeroed-out (as in normal KEY-ON) but not always.
		 (I havent found the rule for that - perhaps only when the output level is low)

		 The difference (when compared to normal Envelope Generator mode) is
		 that the resolution in Decay and Sustain phases is 4 times lower;
		 this results in only 256 steps instead of normal 1024.
		 In other words:
		 when SSG-EG is disabled, the step inside of the EG is one,
		 when SSG-EG is enabled, the step is four (in Decay and Sustain phases).

		 Times between the level changes are the same in both modes.


		 Important:
		 Decay 1 Level (so called SL) is compared to actual SSG-EG output, so
		 it is the same in both SSG and no-SSG modes, with this exception:

		 when the SSG-EG is enabled and is generating raising levels
		 (when the EG output is inverted) the SL will be found at wrong level !!!
		 For example, when SL=02:
		 0 -6 = -6dB in non-inverted EG output
		 96-6 = -90dB in inverted EG output
		 Which means that EG compares its level to SL as usual, and that the
		 output is simply inverted afterall.


		 The Yamaha's manuals say that AR should be set to 0x1f (max speed).
		 That is not necessary, but then EG will be generating Attack phase.

		 */

		break;

	case 0xa0:
		switch(OPN_SLOT(r)) {
			case 0: /* 0xa0-0xa2 : FNUM1 */
			{
				u32 fn = (((u32)((YM2610.OPN.ST.fn_h)&7))<<8) + v;
				u8 blk = YM2610.OPN.ST.fn_h>>3;
				/* keyscale code */
				CH->kcode = (blk<<2) | opn_fktable[fn >> 7];
				/* phase increment counter */
				CH->fc = YM2610.OPN.fn_table[fn*2]>>(7-blk);

				/* store fnum in clear form for LFO PM calculations */
				CH->block_fnum = (blk<<11) | fn;

				CH->SLOT[SLOT1].Incr=-1;
			}
			break;
			case 1: /* 0xa4-0xa6 : FNUM2,BLK */
			YM2610.OPN.ST.fn_h = v&0x3f;
			break;
			case 2: /* 0xa8-0xaa : 3CH FNUM1 */
			if(r < 0x100)
			{
				u32 fn = (((u32)(YM2610.OPN.SL3.fn_h&7))<<8) + v;
				u8 blk = YM2610.OPN.SL3.fn_h>>3;
				/* keyscale code */
				YM2610.OPN.SL3.kcode[c]= (blk<<2) | opn_fktable[fn >> 7];
				/* phase increment counter */
				YM2610.OPN.SL3.fc[c] = YM2610.OPN.fn_table[fn*2]>>(7-blk);
				YM2610.OPN.SL3.block_fnum[c] = fn;
				YM2610.CH[2].SLOT[SLOT1].Incr=-1;
			}
			break;
			case 3: /* 0xac-0xae : 3CH FNUM2,BLK */
			if(r < 0x100)
			YM2610.OPN.SL3.fn_h = v&0x3f;
			break;
		}
		break;

		case 0xb0:
		switch(OPN_SLOT(r) ) {
			case 0: /* 0xb0-0xb2 : FB,ALGO */
			{
				int feedback = (v>>3)&7;
				CH->ALGO = v&7;
				CH->FB = feedback ? feedback+6 : 0;
				setup_connection(CH, c );
			}
			break;
			case 1: /* 0xb4-0xb6 : L , R , AMS , PMS (YM2612/YM2610B/YM2610/YM2608) */
			{
				/* b0-2 PMS */
				CH->pms = (v & 7) * 32; /* CH->pms = PM depth * 32 (index in lfo_pm_table) */

				/* b4-5 AMS */
				CH->ams = lfo_ams_depth_shift[(v>>4) & 0x03];

				/* PAN :  b7 = L, b6 = R */
				//CH->lr = v & 0xC0;
				if((v & 0xC0) == 0xC0) {
					CH->pan[0] = 0;
					CH->pan[1] = 0;
					CH->pan[2] = ~0;
				} else {				
					CH->pan[0] = (v & 0x80) ? ~0 : 0;
					CH->pan[1] = (v & 0x40) ? ~0 : 0;
					CH->pan[2] = 0;
				}				
			
				//YM2610.OPN.pan[ c*2 ] = (v & 0x80) ? ~0 : 0;
				//YM2610.OPN.pan[ c*2+1 ] = (v & 0x40) ? ~0 : 0;

			}
			break;
		}
		break;
	}
}

/*********************************************************************************************/

/* SSG */

static void SSGWriteReg(int r, int v) {
	int old;

	YM2610.regs[r] = v;

	switch (r) {
	case 0x00:
	case 0x02:
	case 0x04: /* Channel A/B/C Fine Tune */
	case 0x01:
	case 0x03:
	case 0x05: /* Channel A/B/C Coarse */
	{
		int ch = r >> 1;

		r &= ~1;
		YM2610.regs[r + 1] &= 0x0f;
		old = SSG.period[ch];
		SSG.period[ch] = (YM2610.regs[r] + 256 * YM2610.regs[r + 1]) * SSG.step;
		if (SSG.period[ch] == 0)
			SSG.period[ch] = SSG.step;
		SSG.count[ch] += SSG.period[ch] - old;
		if (SSG.count[ch] <= 0)
			SSG.count[ch] = 1;
	}
		break;

	case 0x06: /* Noise percent */
		YM2610.regs[SSG_NOISEPER] &= 0x1f;
		old = SSG.PeriodN;
		SSG.PeriodN = YM2610.regs[SSG_NOISEPER] * SSG.step;
		if (SSG.PeriodN == 0)
			SSG.PeriodN = SSG.step;
		SSG.CountN += SSG.PeriodN - old;
		if (SSG.CountN <= 0)
			SSG.CountN = 1;
		break;

	case 0x07: /* Enable */
		SSG.lastEnable = YM2610.regs[SSG_ENABLE];
		break;

	case 0x08:
	case 0x09:
	case 0x0a: /* Channel A/B/C Volume */
	{
		int ch = r & 3;

		YM2610.regs[r] &= 0x1f;
		SSG.envelope[ch] = YM2610.regs[r] & 0x10;
		SSG.vol[ch] =
				SSG.envelope[ch] ?
						SSG.VolE :
						SSG.vol_table[
								YM2610.regs[r] ? YM2610.regs[r] * 2 + 1 : 0];
	}
		break;

	case SSG_EFINE: // Envelope Fine
	case SSG_ECOARSE: // Envelope Coarse
		old = SSG.PeriodE;
		SSG.PeriodE = (YM2610.regs[SSG_EFINE] + 256 * YM2610.regs[SSG_ECOARSE])
				* SSG.step;
		if (SSG.PeriodE == 0)
			SSG.PeriodE = SSG.step / 2;
		SSG.CountE += SSG.PeriodE - old;
		if (SSG.CountE <= 0)
			SSG.CountE = 1;
		break;

	case SSG_ESHAPE: // Envelope Shapes
		YM2610.regs[SSG_ESHAPE] &= 0x0f;
		SSG.attack = (YM2610.regs[SSG_ESHAPE] & 0x04) ? 0x1f : 0x00;
		if ((YM2610.regs[SSG_ESHAPE] & 0x08) == 0) {
			/* if Continue = 0, map the shape to the equivalent one which has Continue = 1 */
			SSG.hold = 1;
			SSG.alternate = SSG.attack;
		} else {
			SSG.hold = YM2610.regs[SSG_ESHAPE] & 0x01;
			SSG.alternate = YM2610.regs[SSG_ESHAPE] & 0x02;
		}
		SSG.CountE = SSG.PeriodE;
		SSG.count_env = 0x1f;
		SSG.holding = 0;
		SSG.VolE = SSG.vol_table[SSG.count_env ^ SSG.attack];
		if (SSG.envelope[0])
			SSG.vol[0] = SSG.VolE;
		if (SSG.envelope[1])
			SSG.vol[1] = SSG.VolE;
		if (SSG.envelope[2])
			SSG.vol[2] = SSG.VolE;
		break;

	case SSG_PORTA: // Port A
	case SSG_PORTB: // Port B
		break;
	}
}

static int SSG_calc_count(int length) {
	int i;

	/* calc SSG count */
	for (i = 0; i < 3; i++) {
		if (YM2610.regs[SSG_ENABLE] & (0x01 << i)) {
			if (SSG.count[i] <= length * SSG_STEP)
				SSG.count[i] += length * SSG_STEP;
			SSG.output[i] = 1;
		} else if (YM2610.regs[0x08 + i] == 0) {
			if (SSG.count[i] <= length * SSG_STEP)
				SSG.count[i] += length * SSG_STEP;
		}
	}

	/* for the noise channel we must not touch OutputN - it's also not necessary */
	/* since we use outn. */
	if ((YM2610.regs[SSG_ENABLE] & 0x38) == 0x38) /* all off */
	{
		if (SSG.CountN <= length * SSG_STEP)
			SSG.CountN += length * SSG_STEP;
	}

	return (SSG.OutputN | YM2610.regs[SSG_ENABLE]);
}

static int SSG_CALC(int outn) {
	int ch;
	int vol[3];
	int left;

	/* vola, volb and volc keep track of how long each square wave stays */
	/* in the 1 position during the sample period. */
	vol[0] = vol[1] = vol[2] = 0;

	left = SSG_STEP;

	do {
		int nextevent;

		nextevent = (SSG.CountN < left) ? SSG.CountN : left;

		for (ch = 0; ch < 3; ch++) {
			if (outn & (0x08 << ch)) {
				if (SSG.output[ch])
					vol[ch] += SSG.count[ch];
				SSG.count[ch] -= nextevent;

				while (SSG.count[ch] <= 0) {
					SSG.count[ch] += SSG.period[ch];
					if (SSG.count[ch] > 0) {
						SSG.output[ch] ^= 1;
						if (SSG.output[ch])
							vol[ch] += SSG.period[ch];
						break;
					}
					SSG.count[ch] += SSG.period[ch];
					vol[ch] += SSG.period[ch];
				}
				if (SSG.output[ch])
					vol[ch] -= SSG.count[ch];
			} else {
				SSG.count[ch] -= nextevent;
				while (SSG.count[ch] <= 0) {
					SSG.count[ch] += SSG.period[ch];
					if (SSG.count[ch] > 0) {
						SSG.output[ch] ^= 1;
						break;
					}
					SSG.count[ch] += SSG.period[ch];
				}
			}
		}

		SSG.CountN -= nextevent;
		if (SSG.CountN <= 0) {
			/* Is noise output going to change? */
			if ((SSG.RNG + 1) & 2) /* (bit0^bit1)? */
			{
				SSG.OutputN = ~SSG.OutputN;
				outn = (SSG.OutputN | YM2610.regs[SSG_ENABLE]);
			}

			if (SSG.RNG & 1)
				SSG.RNG ^= 0x24000;
			SSG.RNG >>= 1;
			SSG.CountN += SSG.PeriodN;
		}

		left -= nextevent;
	} while (left > 0);

	/* update envelope */
	if (SSG.holding == 0) {
		SSG.CountE -= SSG_STEP;
		if (SSG.CountE <= 0) {
			do {
				SSG.count_env--;
				SSG.CountE += SSG.PeriodE;
			} while (SSG.CountE <= 0);

			/* check envelope current position */
			if (SSG.count_env < 0) {
				if (SSG.hold) {
					if (SSG.alternate)
						SSG.attack ^= 0x1f;
					SSG.holding = 1;
					SSG.count_env = 0;
				} else {
					/* if count_env has looped an odd number of times (usually 1), */
					/* invert the output. */
					if (SSG.alternate && (SSG.count_env & 0x20))
						SSG.attack ^= 0x1f;

					SSG.count_env &= 0x1f;
				}
			}

			SSG.VolE = SSG.vol_table[SSG.count_env ^ SSG.attack];
			/* reload volume */
			if (SSG.envelope[0])
				SSG.vol[0] = SSG.VolE;
			if (SSG.envelope[1])
				SSG.vol[1] = SSG.VolE;
			if (SSG.envelope[2])
				SSG.vol[2] = SSG.VolE;
		}
	}

	out_ssg = (((vol[0] * SSG.vol[0]) + (vol[1] * SSG.vol[1])
			+ (vol[2] * SSG.vol[2])) / SSG_STEP) / 3;

	return outn;
}

static void SSG_init_table(void) {
	int i;
	double out;

	/* calculate the volume->voltage conversion table */
	/* The AY-3-8910 has 16 levels, in a logarithmic scale (3dB per step) */
	/* The YM2149 still has 16 levels for the tone generators, but 32 for */
	/* the envelope generator (1.5dB per step). */
	out = SSG_MAX_OUTPUT;
	for (i = 31; i > 0; i--) {
		SSG.vol_table[i] = out + 0.5f; /* round to nearest */

		out /= 1.188502227f; /* = 10 ^ (1.5/20) = 1.5dB */
	}
	SSG.vol_table[0] = 0;
}

static void SSG_reset(void) {
	int i;

	SSG.RNG = 1;
	SSG.output[0] = 0;
	SSG.output[1] = 0;
	SSG.output[2] = 0;
	SSG.OutputN = 0xff;
	SSG.lastEnable = -1;
	for (i = 0; i < SSG_PORTA; i++) {
		YM2610.regs[i] = 0x00;
		SSGWriteReg(i, 0x00);
	}
}

static void SSG_write(int r, int v) {
	SSGWriteReg(r, v);
}

/*********************************************************************************************/

/**** YM2610 ADPCM-A defines ****/
#define ADPCM_SHIFT    (8)      /* 16 frequency step rate   */
#define ADPCMA_ADDRESS_SHIFT 8   /* adpcm A address shift */

static u8 *pcmbufA;
static u32 pcmsizeA;

/* Algorithm and tables verified on real YM2610 */

/* usual ADPCM table (16 * 1.1^N) */
static int steps[49] = { 16, 17, 19, 21, 23, 25, 28, 31, 34, 37, 41, 45, 50, 55,
		60, 66, 73, 80, 88, 97, 107, 118, 130, 143, 157, 173, 190, 209, 230,
		253, 279, 307, 337, 371, 408, 449, 494, 544, 598, 658, 724, 796, 876,
		963, 1060, 1166, 1282, 1411, 1552 };

/* different from the usual ADPCM table */
static int step_inc[16] = { 
	-1 * 16, -1 * 16, -1 * 16, -1 * 16, 2 * 16, 5 * 16, 7 * 16, 9 * 16,	
	-1 * 16, -1 * 16, -1 * 16, -1 * 16, 2 * 16, 5 * 16, 7 * 16, 9 * 16	
};

/* speedup purposes only */
#define JEDI_PADDING 0
static int16_t jedi_table[50 * 16] = { 0 };

static void OPNB_ADPCMA_init_table(void) {
	int step, nib;
	
	// 0 to 48 are the base samples
	for (step = 0; step < 49; step++) {
		/* loop over all nibbles and compute the difference */
		for (nib = 0; nib < 16; nib++) {
			int value = (2 * (nib & 0x07) + 1) * steps[step] / 8;
			jedi_table[step * 16 + nib] = (nib & 0x08) ? -value : value;
		}
	}	
	for (nib = 0; nib < 16; nib++) {
		jedi_table[49 * 16 + nib] = jedi_table[48 * 16 + nib];
	}
}

/* ADPCM A (Non control type) : calculate one channel output */
// static inline void OPNB_ADPCMA_calc_chan(int c) {
// 	/* end check */
// 	/* 11-06-2001 JB: corrected comparison. Was > instead of == */
// 	/* YM2610 checks lower 20 bits only, the 4 MSB bits are sample bank */
// 	/* Here we use 1<<21 to compensate for nibble calculations */
// 	register int32_t acc, decstep, a, b;
// 	register uint8_t data0, data1;
// 
// 	data0 = pcmbufA[YM2610.adpcma[c].now_addr++];
// 	data1 = data0; data0 >>= 4; data1 &= 0xF;
// 
// 	decstep = YM2610.adpcma[c].adpcma_step;
// 	acc = YM2610.adpcma[c].adpcma_acc;
// 	
// 	a = jedi_table[ decstep + data0 ];
// 	b = step_inc[ data0 ];
// 	decstep += b;
// 	acc += a;
// 
// 	if (decstep > 768) decstep = 768; else if (decstep < 0) decstep = 0;
// 	if (acc & 0x800) acc |= ~0xfff; else acc &= 0xfff;
// 
// 	a = jedi_table[ decstep + data1 ];
// 	b = step_inc[ data1 ];
// 	decstep += b;
// 	acc += a;
// 
// 	if (acc & 0x800) acc |= ~0xfff; else acc &= 0xfff;
// 		
// 	YM2610.adpcma[c].adpcma_acc = acc;
// 	acc *= YM2610.adpcma[c].vol_mul;
// 	YM2610.adpcma[c].adpcma_step = decstep;	
// 	acc >>= YM2610.adpcma[c].vol_shift;
// 	
// 	if (decstep > 768) decstep = 768; else if (decstep < 0) decstep = 0;
// 
// 	YM2610.adpcma[c].adpcma_out = acc;
// }

static uint8_t patchA_map[8192] = { 0 };
static int16_t *pcmbufA_ = NULL;

static int bufferA_patched(int c) {
	int patch_idx = YM2610.adpcma[c].start >> 11;
	uint8_t patch_mask = 1 << ((YM2610.adpcma[c].start >> 8) & 7);

	if(patchA_map[patch_idx] & patch_mask) return 1;
	patchA_map[patch_idx] |= patch_mask;
	return 0;
}

typedef union { struct { int8_t h; uint8_t l; } part; int16_t s; } int16_bytes_t;
typedef union { struct { int8_t a; uint8_t b; int8_t c; uint8_t d; } part; int32_t s; } int32_bytes_t;
typedef union { struct { int16_t a; int16_t b; } part; int32_t s; } int32_words_t;

#define DO_ADPCM(OUT,SAMPLE) { \
	acc += jedi_table[ decstep + SAMPLE ]; \
	decstep += step_inc[ SAMPLE ]; \
	/* ensure buffer is in-range */ \
	if(decstep > 768) decstep = 768; \
	else if(decstep < 0) decstep = 0; \
    /* sign-extend 12 to 16 bits */ \
    /* acc = ((int16_t)(((uint16_t)acc) << 4)) >> 4; */\
	if(acc & 0x800) acc |= 0xF000; else acc &= 0x7FFF; \
    /* write out our 16-bit value to memory */ \
    OUT = acc; }

static void decode_bufferA(uint32_t *pi, int32_t *po, uint32_t length) {
    // fast extract nibbles using bitfields
    union { 
        struct { 
            uint32_t a:4; 
            uint32_t b:4; 
            uint32_t c:4; 
            uint32_t d:4;
            uint32_t e:4; 
            uint32_t f:4; 
            uint32_t g:4; 
            uint32_t h:4; 
        } part; 
        uint32_t s; 
    } sample;
    int32_words_t out;
    int16_t acc = 0;
    int16_t decstep = 0;
	int i;

    // doing eight samples per read; length should be multiples of 256
	//length += 7; 
    length >>= 2; 
	for(i=0; i<length; i++) {
        // read eight samples (one nibble each)
        sample.s = *pi++; //out.s = 0;

        DO_ADPCM(out.part.a,sample.part.a);
        DO_ADPCM(out.part.b,sample.part.b);
        //*po++ = out.s;

        DO_ADPCM(out.part.a,sample.part.c);
        DO_ADPCM(out.part.b,sample.part.d);
        *po++ = out.s;

        DO_ADPCM(out.part.a,sample.part.e);
        DO_ADPCM(out.part.b,sample.part.f);
        //*po++ = out.s;

        DO_ADPCM(out.part.a,sample.part.g);
        DO_ADPCM(out.part.b,sample.part.h);
        *po++ = out.s;  

	}
}

static void patch_bufferA(int c) {
	register uint8_t *pi; 
	register int16_t *po;
	register uint32_t length;
	
	pi = &pcmbufA[YM2610.adpcma[c].start];
	po = &pcmbufA_[YM2610.adpcma[c].start];
	length = 1 + YM2610.adpcma[c].end - YM2610.adpcma[c].start;
	decode_bufferA(pi, po, length);
}

/* ADPCM type A Write */
static inline void OPNB_ADPCMA_write(int r, int v) {
	ADPCMA *adpcma = YM2610.adpcma;
	u8 c = r & 0x07;

	YM2610.regs[r] = v & 0xff; /* stock data */

	switch (r) {
	case 0x100: /* DM,--,C5,C4,C3,C2,C1,C0 */
		if (!(v & 0x80)) {
			/* KEY ON */
			for (c = 0; c < 6; c++) {
				if ((v >> c) & 1) {
					int enable = 1;
					/**** start adpcm ****/
					adpcma[c].now_addr = adpcma[c].start;
					adpcma[c].adpcma_out = 0;

					if (pcmbufA == NULL) {
						/* Check ROM Mapped */
						enable = 0;
					} else {
						if (adpcma[c].end >= pcmsizeA) {
							/* Check End in Range */
							/* adpcma[c].end = pcmsizeA - 1; *//* JB: DO NOT uncomment this, otherwise you will break the comparison in the ADPCM_CALC_CHA() */
						}
						if (adpcma[c].start >= pcmsizeA) /* Check Start in Range */
						{
							enable = 0;
						}
					}
					
 					if(enable && !bufferA_patched(c)) {
 						patch_bufferA(c);
 					}
					adpcma[c].flag = enable;
				} // if ((v >> c) & 1)
		
			}
		} else {
			/* KEY OFF */
			for (c = 0; c < 6; c++)
				if ((v >> c) & 1)
					adpcma[c].flag = 0;
		}
		break;

	case 0x101: /* B0-5 = TL */
		YM2610.adpcmaTL = (v & 0x3f) ^ 0x3f;
		for (c = 0; c < 6; c++) {
			int volume = YM2610.adpcmaTL + adpcma[c].IL;
			/* This is correct, 63 = quiet */
			if (volume >= 63) {
				adpcma[c].vol_mul   = 0;
				adpcma[c].vol_shift = 0;
			} else {
				adpcma[c].vol_mul   = 15 - (volume & 7);	/* so called 0.75 dB */
				adpcma[c].vol_shift =  1 + (volume >> 3);	/* Yamaha engineers used the approximation: each -6 dB is close to divide by two (shift right) */
			}
		}
		break;

	default:
		c = r & 0x07;
		if (c >= 0x06)
			return;
		switch (r & 0x138) {
		case 0x108: /* B7=L,B6=R,B4-0=IL */
		{
			int volume;
			if((v & 0xC0) == 0xC0) {
				adpcma[c].pan[0] = 0;
				adpcma[c].pan[1] = 0;
				adpcma[c].pan[2] = ~0;
			} else {
				adpcma[c].pan[0] = (v & 0x80) ? ~0 : 0;
				adpcma[c].pan[1] = (v & 0x40) ? ~0 : 0;
				adpcma[c].pan[2] = 0;
			}

			adpcma[c].IL = (v & 0x1f) ^ 0x1f;
			volume = YM2610.adpcmaTL + adpcma[c].IL;

			/* This is correct, 63 = quiet */
			if (volume >= 63) {
				adpcma[c].vol_mul   = 0;
				adpcma[c].vol_shift = 0;
			} else {
				adpcma[c].vol_mul   = 15 - (volume & 7);	/* so called 0.75 dB */
				adpcma[c].vol_shift =  1 + (volume >> 3);	/* Yamaha engineers used the approximation: each -6 dB is close to divide by two (shift right) */
			}
		}
			break;

		case 0x110:
		case 0x118:
			adpcma[c].start = ((YM2610.regs[0x118 + c] << 8)
					| YM2610.regs[0x110 + c]) << ADPCMA_ADDRESS_SHIFT;
			break;

		case 0x120:
		case 0x128:
			adpcma[c].end = ((YM2610.regs[0x128 + c] << 8)
					| YM2610.regs[0x120 + c]) << ADPCMA_ADDRESS_SHIFT;
			adpcma[c].end += (1 << ADPCMA_ADDRESS_SHIFT) - 1;
			break;
		}
	}
}

/*********************************************************************************************/

/* DELTA-T particle adjuster */
#define ADPCMB_DELTA_MAX (24576)
#define ADPCMB_DELTA_MIN (127)
#define ADPCMB_DELTA_DEF (127)

#define ADPCMB_DECODE_RANGE 32768
#define ADPCMB_DECODE_MIN (-(ADPCMB_DECODE_RANGE))
#define ADPCMB_DECODE_MAX ((ADPCMB_DECODE_RANGE)-1)

static u8 *pcmbufB;
static u32 pcmsizeB;

/* Forecast to next Forecast (rate = *8) */
/* 1/8 , 3/8 , 5/8 , 7/8 , 9/8 , 11/8 , 13/8 , 15/8 */
static const s32 adpcmb_decode_table1[16] = {
	 1,  3,  5,  7,  9,  11,  13,  15,
	-1, -3, -5, -7, -9, -11, -13, -15
};

/* delta to next delta (rate= *64) */
/* 0.9 , 0.9 , 0.9 , 0.9 , 1.2 , 1.6 , 2.0 , 2.4 */
static const s32 adpcmb_decode_table2[16] = {
	57, 57, 57, 57, 77, 102, 128, 153,
	57, 57, 57, 57, 77, 102, 128, 153
};

/* 0-DRAM x1, 1-ROM, 2-DRAM x8, 3-ROM (3 is bad setting - not allowed by the manual) */
static u8 dram_rightshift[4] = { 3, 0, 0, 0 };

static uint8_t patchB_map[8192] = { 0 };
static int16_t *pcmbufB_ = NULL;

static int bufferB_patched(void) {
	int patch_idx = YM2610.adpcmb.start >> 11;
	uint8_t patch_mask = 1 << ((YM2610.adpcmb.start >> 8) & 7);

	if(patchB_map[patch_idx] & patch_mask) return 1;
	patchB_map[patch_idx] |= patch_mask;
	return 0;
}

static void patch_bufferB(void) {
	register int32_t acc = 0, decstep = 0;
	register uint8_t data0, data1;
	register uint8_t *pi; 
	register int16_t *po;
	register uint32_t length;
	register int32_t a, b;
	
	pi = &pcmbufB[YM2610.adpcmb.start];
	po = &pcmbufB_[YM2610.adpcmb.start];
	length = 1 + YM2610.adpcmb.end - YM2610.adpcmb.start;

	while(length--) {
		data0 = *pi++; data1 = data0; data0 >>= 4; data1 &= 0xF;
		
		a = adpcmb_decode_table1[ data0 ];
		b = adpcmb_decode_table2[ data0 ];
		a *= decstep;
		b *= decstep;
		a /= 8;
		b /= 64;
		acc += a;
		decstep = b;

		a = adpcmb_decode_table1[ data1 ];
		b = adpcmb_decode_table2[ data1 ];
		a *= decstep;
		b *= decstep;
		a /= 8;
		b /= 64;
		acc += a;
		decstep = b;
		
		if(acc > 32767) acc = 32767;
		else if(acc < -32768) acc < -32768;
		
		if (decstep > 24576 ) decstep = 24576;
		else if(decstep < 127 ) decstep = 127;

		*po++ = acc;	
	}
}

/* DELTA-T-ADPCM write register */
static void OPNB_ADPCMB_write(ADPCMB *adpcmb, int r, int v) {
	int enable;
	if (r >= 0x20) return;

	YM2610.regs[r] = v; /* stock data */

	switch (r) {
	case 0x10:
		/*
		 START:
		 Accessing *external* memory is started when START bit (D7) is set to "1", so
		 you must set all conditions needed for recording/playback before starting.
		 If you access *CPU-managed* memory, recording/playback starts after
		 read/write of ADPCM data register $08.

		 REC:
		 0 = ADPCM synthesis (playback)
		 1 = ADPCM analysis (record)

		 MEMDATA:
		 0 = processor (*CPU-managed*) memory (means: using register $08)
		 1 = external memory (using start/end/limit registers to access memory: RAM or ROM)


		 SPOFF:
		 controls output pin that should disable the speaker while ADPCM analysis

		 RESET and REPEAT only work with external memory.


		 some examples:
		 value:   START, REC, MEMDAT, REPEAT, SPOFF, x,x,RESET   meaning:
		 C8     1      1    0       0       1      0 0 0       Analysis (recording) from AUDIO to CPU (to reg $08), sample rate in PRESCALER register
		 E8     1      1    1       0       1      0 0 0       Analysis (recording) from AUDIO to EXT.MEMORY,       sample rate in PRESCALER register
		 80     1      0    0       0       0      0 0 0       Synthesis (playing) from CPU (from reg $08) to AUDIO,sample rate in DELTA-N register
		 a0     1      0    1       0       0      0 0 0       Synthesis (playing) from EXT.MEMORY to AUDIO,        sample rate in DELTA-N register

		 60     0      1    1       0       0      0 0 0       External memory write via ADPCM data register $08
		 20     0      0    1       0       0      0 0 0       External memory read via ADPCM data register $08

		 */
		v |= 0x20; /*  YM2610 always uses external memory and doesn't even have memory flag bit. */
		adpcmb->portstate = v & (0x80 | 0x40 | 0x20 | 0x10 | 0x01); /* start, rec, memory mode, repeat flag copy, reset(bit0) */

		if (adpcmb->portstate & 0x80) { 
			/* START,REC,MEMDATA,REPEAT,SPOFF,--,--,RESET */
			enable = 1;
			/* set PCM BUSY bit */
			//adpcmb->PCM_BSY = 1;

			/* start ADPCM */
			adpcmb->now_step = 0;
			adpcmb->acc = 0;
			adpcmb->prev_acc = 0;
			adpcmb->adpcml = 0;
			adpcmb->adpcmd = ADPCMB_DELTA_DEF;
			adpcmb->now_data = 0;
		}

		adpcmb->now_addr = adpcmb->start;
		adpcmb->memread = 2; /* two dummy reads needed before accesing external memory via register $08*/

		/* if yes, then let's check if ADPCM memory is mapped and big enough */
		if (!pcmbufB) {
			adpcmb->portstate = 0x00;
			enable = 0;
			//adpcmb->PCM_BSY = 0;
		} else {
			if (adpcmb->end >= pcmsizeB) /* Check End in Range */
				adpcmb->end = pcmsizeB - 1;

			if (adpcmb->start >= pcmsizeB) { /* Check Start in Range */
				adpcmb->portstate = 0x00;
				//adpcmb->PCM_BSY = 0;
				enable = 0;
			}
		}
		
		if(enable && !bufferB_patched()) {
			patch_bufferB();
		}
		adpcmb->PCM_BSY = enable;

		if (adpcmb->portstate & 0x01) {
			adpcmb->portstate = 0x00;

			/* clear PCM BUSY bit (in status register) */
			adpcmb->PCM_BSY = 0;

			/* set BRDY flag */
			if (adpcmb->status_change_BRDY_bit)
				YM2610.adpcm_arrivedEndAddress |=
						adpcmb->status_change_BRDY_bit;
		}
		break;

	case 0x11: /* L,R,-,-,SAMPLE,DA/AD,RAMTYPE,ROM */
		v |= 0x01; /*  YM2610 always uses ROM as an external memory and doesn't tave ROM/RAM memory flag bit. */
		//adpcmb->pan = &out_delta[(v >> 6) & 0x03];
		if ((adpcmb->control2 & 3) != (v & 3)) {
			/*0-DRAM x1, 1-ROM, 2-DRAM x8, 3-ROM (3 is bad setting - not allowed by the manual) */
			if (adpcmb->DRAMportshift != dram_rightshift[v & 3]) {
				adpcmb->DRAMportshift = dram_rightshift[v & 3];

				/* final shift value depends on chip type and memory type selected:
				 8 for YM2610 (ROM only),
				 5 for ROM for Y8950 and YM2608,
				 5 for x8bit DRAMs for Y8950 and YM2608,
				 2 for x1bit DRAMs for Y8950 and YM2608.
				 */

				/* refresh addresses */
				adpcmb->start = ((YM2610.regs[0x13] << 8) | YM2610.regs[0x12])
						<< adpcmb->portshift;
				adpcmb->end = ((YM2610.regs[0x15] << 8) | YM2610.regs[0x14])
						<< adpcmb->portshift;
				adpcmb->end += (1 << adpcmb->portshift) - 1;
				adpcmb->limit = ((YM2610.regs[0x1d] << 8) | YM2610.regs[0x1c])
						<< adpcmb->portshift;
			}
		}
		//YM2610.adpcmb.pan
		if((v & 0xC0) == 0xC0) {
			adpcmb->pan[0] = 0;
			adpcmb->pan[1] = 0;
			adpcmb->pan[2] = ~0;
		} else {
			adpcmb->pan[0] = (v & 0x80) ? ~0 : 0;
			adpcmb->pan[1] = (v & 0x40) ? ~0 : 0;
			adpcmb->pan[2] = 0;
		}
		adpcmb->control2 = v;
		break;

	case 0x12: /* Start Address L */
	case 0x13: /* Start Address H */
		adpcmb->start = ((YM2610.regs[0x13] << 8) | YM2610.regs[0x12])
				<< adpcmb->portshift;
		/*logerror("DELTAT start: 02=%2x 03=%2x addr=%8x\n", YM2610.regs[0x12], YM2610.regs[0x13], adpcmb->start);*/
		break;

	case 0x14: /* Stop Address L */
	case 0x15: /* Stop Address H */
		adpcmb->end = ((YM2610.regs[0x15] << 8) | YM2610.regs[0x14])
				<< adpcmb->portshift;
		adpcmb->end += (1 << adpcmb->portshift) - 1;
		/*logerror("DELTAT end  : 04=%2x 05=%2x addr=%8x\n", YM2610.regs[0x14], YM2610.reg[0x15], adpcmb->end);*/
		break;

	case 0x19: /* DELTA-N L (ADPCM Playback Prescaler) */
	case 0x1a: /* DELTA-N H */
		adpcmb->delta = (YM2610.regs[0x1a] << 8) | YM2610.regs[0x19];
		/*logerror("DELTAT deltan:09=%2x 0a=%2x\n", YM2610.regs[0x19], YM2610.regs[0x1a]);*/
		break;

	case 0x1b: /* Output level control (volume, linear) */
	{
		//s32 oldvol = adpcmb->volume;
		adpcmb->volume = v & 0xff;//) * (adpcmb->output_range / 256) / ADPCMB_DECODE_RANGE;
		// 		v	  *		((1<<16)>>8)		>>	15;
		// thus:	v	  *		(1<<8)				>>	15;
		// thus: output_range must be (1 << (15+8)) at least
		// 		v     *		((1<<23)>>8)		>>	15;
		// 		v	  *		(1<<15)				>>	15;
		/*logerror("DELTAT vol = %2x\n", v & 0xff);*/
		//if (oldvol != 0) {
		//	adpcmb->adpcml *= adpcmb->volume;
		//	adpcmb->adpcml /= oldvol;
			//adpcmb->adpcml = (int)((double)adpcmb->adpcml / (double)oldvol * (double)adpcmb->volume);
		//}
	}
		break;
	}
}

// static inline void adpcmb_decode(uint8_t code) {
// 	acc += adpcmb_decode_table1[ code ] * decstep / 8;
// 	decstep = decstep * adpcmb_decode_table2[ code ] / 64;
// 
// 	//acc = (acc << 16) >> 16;
// }

// static inline void OPNB_ADPCMB_CALC(void) {
// 	// this is called by the main loop ONLY when we need a new sample; note that
// 	// since the Amiga samples at 27Khz we have to also down-sample 2:1
// 	register int32_t acc, decstep, a, b;
// 	register uint8_t data0, data1;
// 
// 	// Get next chunk
// 	data0 = pcmbufB[YM2610.adpcmb.now_addr++];
// 	data1 = data0; data0 >>= 4; data1 &= 0xF;
// 		
// 	decstep = YM2610.adpcmb.adpcmd;
// 	acc = YM2610.adpcmb.acc;
// 		
// 	a = adpcmb_decode_table1[ data0 ];
// 	b = adpcmb_decode_table2[ data0 ];
// 	a *= decstep;
// 	b *= decstep;
// 	a /= 8;
// 	b /= 64;
// 	acc += a;
// 	decstep = b;
// 
// 	a = adpcmb_decode_table1[ data1 ];
// 	b = adpcmb_decode_table2[ data1 ];
// 	a *= decstep;
// 	b *= decstep;
// 	a /= 8;
// 	b /= 64;
// 	acc += a;
// 	decstep = b;
// 		
// 	if(acc > 32767) acc = 32767;
// 	else if(acc < -32768) acc < -32768;
// 		
// 	if (decstep > 24576 ) decstep = 24576;
// 	else if(decstep < 127 ) decstep = 127;
// 
// 	YM2610.adpcmb.adpcmd = decstep;
// 	YM2610.adpcmb.acc = acc;
// 	
// 	YM2610.adpcmb.adpcml = (acc * YM2610.adpcmb.volume) >> 8;
// }

/*********************************************************************************************/

/* YM2610(B) */

#ifdef SOUND_TEST
static int stream_pos;
static int samples_left;
#endif
static FM_TIMERHANDLER sav_TimerHandler;
static FM_IRQHANDLER sav_IRQHandler;

void YM2610Init(int clock, int rate, void *pcmroma, int pcmsizea, void *pcmromb, int pcmsizeb, FM_TIMERHANDLER TimerHandler, FM_IRQHANDLER IRQHandler) {
	/*
	 sound->stack    = 0x10000;
	 sound->stereo   = 1;
	 #ifdef SOUND_TEST
	 if (sound_test)
	 sound->callback = YM2610Update_SoundTest;
	 else
	 #endif
	 sound->callback = YM2610Update;
	 */

	/* clear */
	memset(&YM2610, 0, sizeof(YM2610));
	memset(&SSG, 0, sizeof(SSG));

	enablefm = arg[OPTION_ENABLEFM];
	enable16 = arg[OPTION_BITRATE] == 16;

	OPNInitTable();
	SSG_init_table();
	OPNB_ADPCMA_init_table();

	/* FM */
	//YM2610.OPN.P_CH = YM2610.CH;
	YM2610.OPN.ST.clock = clock;
	YM2610.OPN.ST.rate = rate;
	
	/* Extend handler */
	YM2610.OPN.ST.Timer_Handler = TimerHandler;
	YM2610.OPN.ST.IRQ_Handler = IRQHandler;
	sav_TimerHandler = TimerHandler;
	sav_IRQHandler = IRQHandler;

	/* SSG */
	SSG.step = ((double) SSG_STEP * rate * 8) / clock;
	
	/* ADPCM-A */
	pcmbufA = (u8 *) pcmroma;
	pcmsizeA = pcmsizea;
	if(pcmbufA_) free(pcmbufA_);
	pcmbufA_ = malloc(pcmsizeA * 2);
	
	//decode_bufferA( pcmbufA, pcmbufA_, pcmsizea );
	
	/* ADPCM-B */
	pcmbufB = (u8 *) pcmromb;
	pcmsizeB = pcmsizeb;
	if(pcmbufB_) free(pcmbufB_);
	pcmbufB_ = malloc(pcmsizeB * 2);

	YM2610.adpcmb.status_change_EOS_bit = 0x80; /* status flag: set bit7 on End Of Sample */

	YM2610Reset();
}

static int max_vol[14] = { 0 };
#define SETMAX(ch,a) max_vol[ch] = (max_vol[ch]<(a)) ? a : max_vol[ch];

static uint16_t gain = 256;
static int32_t max_sample = 32767;

void YM2610ChangeSamplerate(int rate) {
	int i;

	printf("Max sample: %d\n", max_sample);

	YM2610.OPN.ST.rate = rate;
	SSG.step = ((double) SSG_STEP * rate * 8) / YM2610.OPN.ST.clock;
	OPNSetPres(6 * 24, 6 * 24, 4 * 2); /* OPN 1/6, SSG 1/4 */	
}

/* reset one of chip */
void YM2610Reset(void) {
	int i;
	// = &YM2610.OPN;

	/* Reset Prescaler */
	OPNSetPres(6 * 24, 6 * 24, 4 * 2); /* OPN 1/6, SSG 1/4 */
	/* reset SSG section */
	SSG_reset();
	/* status clear */
	FM_IRQMASK_SET(0x03);
	FM_BUSY_CLEAR();
	OPNWriteMode(0x27, 0x30); /* mode 0, timer reset */

	YM2610.OPN.eg_timer = 0;
	YM2610.OPN.eg_cnt = 0;

	FM_STATUS_RESET(0xff);

	memset(pcmbufA_, 0, pcmsizeA * 2);
	memset(pcmbufB_, 0, pcmsizeB * 2);

	reset_channels(YM2610.CH, 6);
	/* reset OPerator paramater */
	for (i = 0xb6; i >= 0xb4; i--) {
		OPNWriteReg(i, 0xc0);
		OPNWriteReg(i | 0x100, 0xc0);
	}
	for (i = 0xb2; i >= 0x30; i--) {
		OPNWriteReg(i, 0x00);
		OPNWriteReg(i | 0x100, 0x00);
	}
	for (i = 0x26; i >= 0x20; i--) {
		OPNWriteReg(i, 0x00);
	}
	/**** ADPCM work initial ****/
	for (i = 0; i < 6; i++) {
		YM2610.adpcma[i].step = (u32) ((double) (1 << ADPCM_SHIFT)
				* ((double) YM2610.OPN.ST.freqbase) / 3.0);
		YM2610.adpcma[i].now_addr = 0;
		YM2610.adpcma[i].now_step = 0;
		YM2610.adpcma[i].start = 0;
		YM2610.adpcma[i].end = 0;
		YM2610.adpcma[i].vol_mul = 0;
		YM2610.adpcma[i].pan[0] = 0;
		YM2610.adpcma[i].pan[1] = 0;
		YM2610.adpcma[i].pan[2] = ~0;
		// = &out_adpcma[OUTD_CENTER]; /* default center */
		YM2610.adpcma[i].flagMask = 1 << i;
		YM2610.adpcma[i].flag = 0;
		//YM2610.adpcma[i].adpcma_acc = 0;
		//YM2610.adpcma[i].adpcma_step = 0;
		YM2610.adpcma[i].adpcma_out = 0;
	}
	YM2610.adpcmaTL = 0x3f;

	YM2610.adpcm_arrivedEndAddress = 0;

	/* ADPCM-B unit */
	YM2610.adpcmb.freqbase = YM2610.OPN.ST.freqbase;
	YM2610.adpcmb.portshift = 8; /* allways 8bits shift */
	YM2610.adpcmb.output_range = 1 << 23;

	YM2610.adpcmb.now_addr = 0;
	YM2610.adpcmb.now_step = 0;
	YM2610.adpcmb.step = 0;
	YM2610.adpcmb.start = 0;
	YM2610.adpcmb.end = 0;
	YM2610.adpcmb.limit = ~0; /* this way YM2610 and Y8950 (both of which don't have limit address reg) will still work */
	YM2610.adpcmb.volume = 0;
	YM2610.adpcmb.pan[0] = 0;
	YM2610.adpcmb.pan[1] = 0;
	YM2610.adpcmb.pan[2] = ~0;
	YM2610.adpcmb.acc = 0;
	YM2610.adpcmb.prev_acc = 0;
	YM2610.adpcmb.adpcmd = 127;
	YM2610.adpcmb.adpcml = 0;
	YM2610.adpcmb.portstate = 0x20;
	YM2610.adpcmb.control2 = 0x01;

	/* The flag mask register disables the BRDY after the reset, however
	 ** as soon as the mask is enabled the flag needs to be set. */

	/* set BRDY bit in status register */
	if (YM2610.adpcmb.status_change_BRDY_bit)
		YM2610.adpcm_arrivedEndAddress |= YM2610.adpcmb.status_change_BRDY_bit;

#ifdef SOUND_TEST
	stream_pos = 0;
	samples_left = 0;
#endif
}

/* YM2610 write */
/* a = address */
/* v = value   */
int YM2610Write(int a, u8 v) {
	int addr;
	int ch;

	v &= 0xff; /* adjust to 8 bit bus */

	switch (a & 3) {
	case 0: /* address port 0 */
		YM2610.OPN.ST.address = v;
		YM2610.addr_A1 = 0;

		/* Write register to SSG emulator */
//		if (v < 16) SSG_write(0, v);
		break;

	case 1: /* data port 0    */
		if (YM2610.addr_A1 != 0)
			break; /* verified on real YM2608 */

		YM2610UpdateRequest();
		addr = YM2610.OPN.ST.address;
		YM2610.regs[addr] = v;
		switch (addr & 0xf0) {
		case 0x00: /* SSG section */
			/* Write data to SSG emulator */
			SSG_write(addr, v);
			break;

		case 0x10: /* DeltaT ADPCM */
			switch (addr) {
			case 0x10: /* control 1 */
			case 0x11: /* control 2 */
			case 0x12: /* start address L */
			case 0x13: /* start address H */
			case 0x14: /* stop address L */
			case 0x15: /* stop address H */

			case 0x19: /* delta-n L */
			case 0x1a: /* delta-n H */
			case 0x1b: /* volume */
				OPNB_ADPCMB_write(&YM2610.adpcmb, addr, v);
				break;

			case 0x1c: /*  FLAG CONTROL : Extend Status Clear/Mask */
			{
				u8 statusmask = ~v;
				/* set arrived flag mask */
				for (ch = 0; ch < 6; ch++)
					YM2610.adpcma[ch].flagMask = statusmask & (1 << ch);

				YM2610.adpcmb.status_change_EOS_bit = statusmask & 0x80; /* status flag: set bit7 on End Of Sample */

				/* clear arrived flag */
				YM2610.adpcm_arrivedEndAddress &= statusmask;
			}
				break;
			}
			break;

		case 0x20: /* Mode Register */
			OPNWriteMode(addr, v);
			break;

		default: /* OPN section */
			/* write register */
			OPNWriteReg(addr, v);
			break;
		}
		break;

	case 2: /* address port 1 */
		YM2610.OPN.ST.address = v;
		YM2610.addr_A1 = 1;
		break;

	case 3: /* data port 1    */
		if (YM2610.addr_A1 != 1)
			break; /* verified on real YM2608 */

		YM2610UpdateRequest();
		addr = YM2610.OPN.ST.address | 0x100;
		YM2610.regs[addr | 0x100] = v;
		if (addr < 0x130)
			/* 100-12f : ADPCM A section */
			OPNB_ADPCMA_write(addr, v);
		else
			OPNWriteReg(addr, v);
		break;
	}

	return YM2610.OPN.ST.irq;
}

u8 YM2610Read(int a) {
	int addr = YM2610.OPN.ST.address;

	switch (a & 3) {
	case 0: /* status 0 : YM2203 compatible */
		return FM_STATUS_FLAG(&YM2610.OPN.ST) & 0x83;

	case 1: /* data 0 */
		if (addr < SSG_PORTA)
			return YM2610.regs[addr];
		if (addr == 0xff)
			return 0x01;
		break;

	case 2: /* status 1 : ADPCM status */
		/* ADPCM STATUS (arrived End Address) */
		/* B, --, A5, A4, A3, A2, A1, A0 */
		/* B     = ADPCM-B(DELTA-T) arrived end address */
		/* A0-A5 = ADPCM-A          arrived end address */
		return YM2610.adpcm_arrivedEndAddress;
	}
	return 0;
}

int YM2610TimerOver(int ch) {
	FM_ST *ST = &YM2610.OPN.ST;

	if (ch) {
		/* Timer B */
		TimerBOver();
	} else {
		/* Timer A */
		YM2610UpdateRequest();

		/* Timer Update */
		TimerAOver();

		/* CSM mode key, TL controll */
		if (YM2610.OPN.ST.mode & 0x80) {
			/* CSM mode total level latch and auto key on */
			CSMKeyControll(&YM2610.CH[2]);
		}
	}

	return YM2610.OPN.ST.irq;
}

#define LENGTH 256

//extern volatile uint8_t *lHBuffer, *rHBuffer;
//extern volatile uint8_t *lLBuffer, *rLBuffer;

int16_t L_Mix[LENGTH];
int16_t R_Mix[LENGTH];


/*

	Mixing -- the ADPCM-A channels should be down mixed by one cutting their playback
	frequency in half to 9,259Hz. Each channel is then played on the modulo of their
	respective channel and 3.
	
	ADPCM-B is sampled each clock cycle -- our playback is 27kHz exactly double the
	internal frequency of ADPCM-B. So we calculate ADPCM-B each time the counter rolls
	over and copy it every other time.
	
	0 + 3 + B
	1 + 4 + B
	2 + 5 + B
	0 + 3 + B
	...   and so on

*/


static int chA = 0;

#define mixChannel(P,O) do { int16_t __o = (O); lt += P[0] & __o; rt += P[1] & __o; ct += P[2] & __o; } while(0)
	
// static inline void mixChannel(s32 pan[3], int out) {
// //	uint16_t l = pan[0], r = pan[1];
// //	if(l == r) out >> 1;
// 	lt += pan[0] & out;
// 	rt += pan[1] & out;
// 	ct += pan[2] & out;
// }

static FM_CH *const cch[6] = { 
	&YM2610.CH[1], &YM2610.CH[2],
	&YM2610.CH[4], &YM2610.CH[5]
};

static uint8_t this_fmCh = 0;
static const uint8_t next_chA[] = { 1, 2, 3, 4, 5, 0 };
uint8_t looper = 0;

void YM2610Update(void) {
	int last_fm[6];
	register int32_t lt = 0, rt = 0, ct = 0;
	int i, j, outn, c;
	uint32_t lh_out, ll_out, rh_out, rl_out;
 	int16_t outp;
 	uint8_t outlr;
 	static int check_adpcma = 0;

	//int enable_fm = YM2610.OPN.ST.rate & 1;

	if(enablefm) {
		/* update frequency counter */
		refresh_fc_eg_chan(cch[0]);
		if (YM2610.OPN.ST.mode & 0xc0) {
			/* 3SLOT MODE */
			if (cch[1]->SLOT[SLOT1].Incr == -1) {
				/* 3 slot mode */
				refresh_fc_eg_slot(&cch[1]->SLOT[SLOT1], YM2610.OPN.SL3.fc[1], YM2610.OPN.SL3.kcode[1]);
				refresh_fc_eg_slot(&cch[1]->SLOT[SLOT2], YM2610.OPN.SL3.fc[2], YM2610.OPN.SL3.kcode[2]);
				refresh_fc_eg_slot(&cch[1]->SLOT[SLOT3], YM2610.OPN.SL3.fc[0], YM2610.OPN.SL3.kcode[0]);
				refresh_fc_eg_slot(&cch[1]->SLOT[SLOT4], cch[1]->fc, cch[1]->kcode);
			}
		} else
			refresh_fc_eg_chan(cch[1]);
	
		refresh_fc_eg_chan(cch[2]);
		refresh_fc_eg_chan(cch[3]);

		/* calc SSG count */
		outn = SSG_calc_count(LENGTH);
	}
	
	// Wrapped memory?
	if (YM2610.adpcmb.now_addr == YM2610.adpcmb.limit)
		YM2610.adpcmb.now_addr = 0;

	// Reached end of sample
	if (YM2610.adpcmb.now_addr == YM2610.adpcmb.end) {
		/* 12-06-2001 JB: corrected comparison. Was > instead of == */
		if (YM2610.adpcmb.portstate & 0x10) {
			/* repeat start */
			YM2610.adpcmb.now_addr = YM2610.adpcmb.start;
			YM2610.adpcmb.acc = 0;
			YM2610.adpcmb.adpcmd = ADPCMB_DELTA_DEF;
			YM2610.adpcmb.prev_acc = 0;
			
		} else {
			/* set EOS bit in status register */
			if (YM2610.adpcmb.status_change_EOS_bit)
				YM2610.adpcm_arrivedEndAddress |=
						YM2610.adpcmb.status_change_EOS_bit;

			/* clear PCM BUSY bit (reflected in status register) */
			YM2610.adpcmb.PCM_BSY = 0;
			YM2610.adpcmb.portstate = 0;
			YM2610.adpcmb.adpcml = 0;
			YM2610.adpcmb.prev_acc = 0;
		}
	}	
		
	lh_out = rh_out = ll_out = rl_out = 0;
	
	check_adpcma++;
	
	for (i = 0; i < LENGTH; i++) {
		lt = 0; rt = 0; ct = 0;
		if(enablefm) { 
			/* clear outputs */
			out_fm[1] = 0; out_fm[2] = 0; out_fm[4] = 0; out_fm[5] = 0;

			/* clear outputs SSG */
			out_ssg = 0;

			advance_lfo();
		
			/* advance envelope generator */
			YM2610.OPN.eg_timer += YM2610.OPN.eg_timer_add / 2;
			while (YM2610.OPN.eg_timer >= YM2610.OPN.eg_timer_overflow) {
				YM2610.OPN.eg_timer -= YM2610.OPN.eg_timer_overflow;
				YM2610.OPN.eg_cnt++;
			
				advance_eg_channel(&cch[0]->SLOT[SLOT1]);
				advance_eg_channel(&cch[1]->SLOT[SLOT1]);
				advance_eg_channel(&cch[2]->SLOT[SLOT1]);
				advance_eg_channel(&cch[3]->SLOT[SLOT1]);
			}

			/* calculate FM */
			chan_calc(cch[this_fmCh]);
			this_fmCh = (this_fmCh + 1) & 3;

			chan_calc(cch[this_fmCh]);
			this_fmCh = (this_fmCh + 1) & 3;

			/* calculate SSG */
			//outn = SSG_CALC(outn);
			
			/* mix analog channels */
			//lt = out_ssg; rt = out_ssg; ct = 0;

			mixChannel(YM2610.CH[1].pan, out_fm[1]);	
			mixChannel(YM2610.CH[2].pan, out_fm[2]);	
			mixChannel(YM2610.CH[4].pan, out_fm[4]);	
			mixChannel(YM2610.CH[5].pan, out_fm[5]);
			
		}
		
	//AudioFilter(1);
// 	 "SAMPLERATE/N," 		// Sets audio sample rate
// 							//	0 - Audio disabled
// 							//	9260 - Low quality
// 							//	18520 - Mid quality
// 							//	27780 - High quality

		// at 27780 we play 2/6 channels to match		

		/* ADPCM Channel A divided by three (interleaved) */
		if (YM2610.adpcma[chA].flag) {
			register int32_t mix = pcmbufA_[YM2610.adpcma[chA].now_addr];
			mix *= YM2610.adpcma[chA].vol_mul;
			YM2610.adpcma[chA].now_addr++;
			mix >>= YM2610.adpcma[chA].vol_shift;
			YM2610.adpcma[chA].adpcma_out = mix;
		} 
		chA = next_chA[chA];
		
		if (YM2610.adpcma[chA].flag) {
			register int32_t mix = pcmbufA_[YM2610.adpcma[chA].now_addr];
			mix *= YM2610.adpcma[chA].vol_mul;
			YM2610.adpcma[chA].now_addr++;
			mix >>= YM2610.adpcma[chA].vol_shift;
			YM2610.adpcma[chA].adpcma_out = mix;
		} 
		chA = next_chA[chA];
	
		/* Mix all six channels */
		for(c=0; c<6; c++) {
			mixChannel(YM2610.adpcma[c].pan, YM2610.adpcma[c].adpcma_out);
		}
		
		/* ADPCM Channel B */
		if (YM2610.adpcmb.PCM_BSY) {
			/* ADPCM-B is given 2x weight when mixing to make the channel count
			   an even power of two which makes later math easier */
			YM2610.adpcmb.now_step += YM2610.adpcmb.delta;
			if (YM2610.adpcmb.now_step >= 0x10000) {
				register int32_t mix = pcmbufB_[YM2610.adpcmb.now_addr];
				YM2610.adpcmb.now_step -= 0x10000;
				YM2610.adpcmb.adpcml = (mix * YM2610.adpcmb.volume) >> 8;
				YM2610.adpcmb.now_addr++;
			}
			mixChannel(YM2610.adpcmb.pan, YM2610.adpcmb.adpcml);
		}


		ct >>= 1; lt += ct; rt += ct;
		L_Mix[i] = lt;
		R_Mix[i] = rt;
		
		INTERNAL_TIMER_A( cch[1] );
	}
	INTERNAL_TIMER_B( length );

	// it takes three passes for this to do 256 full steps for each adpcm-a channel
	// 0, 0-255, 1 256-511, 2-512-767 
	// 3, check all channels and reset counter to 0
	if(check_adpcma == 3) {
		for (i = 0; i < 6; i++) {
			if ((YM2610.adpcma[i].now_addr & ((1 << 21) - 1))
			>= ((YM2610.adpcma[i].end) & ((1 << 21) - 1))) {
				YM2610.adpcm_arrivedEndAddress |= YM2610.adpcma[i].flagMask;
				YM2610.adpcma[i].flag = 0;
			}
		}
		check_adpcma = 0;
	}

}

void ym2610_mkstate(gzFile *gzf,int mode) {
	mkstate_data(gzf, &YM2610, sizeof (YM2610), mode);
}

#ifdef SAVE_STATE

STATE_SAVE( ym2610 )
{
	int slot, ch;

	state_save_byte(YM2610.regs, 512);

	state_save_double(&YM2610.OPN.ST.BusyExpire, 1);
	state_save_byte(&YM2610.OPN.ST.address, 1);
	state_save_byte(&YM2610.OPN.ST.irq, 1);
	state_save_byte(&YM2610.OPN.ST.irqmask, 1);
	state_save_byte(&YM2610.OPN.ST.status, 1);
	state_save_long(&YM2610.OPN.ST.mode, 1);
	state_save_byte(&YM2610.OPN.ST.prescaler_sel, 1);
	state_save_byte(&YM2610.OPN.ST.fn_h, 1);
	state_save_long(&YM2610.OPN.ST.TA, 1);
	state_save_long(&YM2610.OPN.ST.TAC, 1);
	state_save_byte(&YM2610.OPN.ST.TB, 1);
	state_save_long(&YM2610.OPN.ST.TBC, 1);

	for (ch = 0; ch < 6; ch++)
	{
		state_save_long(YM2610.CH[ch].op1_out, 2);
		state_save_long(&YM2610.CH[ch].fc, 1);

		for (slot = 0; slot < 4; slot++)
		{
			FM_SLOT *SLOT = &YM2610.CH[ch].SLOT[slot];

			state_save_long(&SLOT->phase, 1);
			state_save_byte(&SLOT->state, 1);
			state_save_long(&SLOT->volume, 1);
		}
	}

	state_save_long(YM2610.OPN.SL3.fc, 3);
	state_save_byte(&YM2610.OPN.SL3.fn_h, 1);
	state_save_byte(YM2610.OPN.SL3.kcode, 3);

	state_save_byte(&YM2610.addr_A1, 1);
	state_save_byte(&YM2610.adpcm_arrivedEndAddress, 1);

	for (ch = 0; ch < 6; ch++)
	{
		state_save_byte(&YM2610.adpcma[ch].flag, 1);
		state_save_byte(&YM2610.adpcma[ch].now_data, 1);
		state_save_long(&YM2610.adpcma[ch].now_addr, 1);
		state_save_long(&YM2610.adpcma[ch].now_step, 1);
		state_save_long(&YM2610.adpcma[ch].adpcma_acc, 1);
		state_save_long(&YM2610.adpcma[ch].adpcma_step, 1);
		state_save_long(&YM2610.adpcma[ch].adpcma_out, 1);
	}

	state_save_byte(&YM2610.adpcmb.portstate, 1);
	state_save_long(&YM2610.adpcmb.now_addr, 1);
	state_save_long(&YM2610.adpcmb.now_step, 1);
	state_save_long(&YM2610.adpcmb.acc, 1);
	state_save_long(&YM2610.adpcmb.prev_acc, 1);
	state_save_long(&YM2610.adpcmb.adpcmd, 1);
	state_save_long(&YM2610.adpcmb.adpcml, 1);

	state_save_long(&option_samplerate, 1);
}

STATE_LOAD( ym2610 )
{
	int slot, ch, r;

	state_load_byte(YM2610.regs, 512);

	state_load_double(&YM2610.OPN.ST.BusyExpire, 1);
	state_load_byte(&YM2610.OPN.ST.address, 1);
	state_load_byte(&YM2610.OPN.ST.irq, 1);
	state_load_byte(&YM2610.OPN.ST.irqmask, 1);
	state_load_byte(&YM2610.OPN.ST.status, 1);
	state_load_long(&YM2610.OPN.ST.mode, 1);
	state_load_byte(&YM2610.OPN.ST.prescaler_sel, 1);
	state_load_byte(&YM2610.OPN.ST.fn_h, 1);
	state_load_long(&YM2610.OPN.ST.TA, 1);
	state_load_long(&YM2610.OPN.ST.TAC, 1);
	state_load_byte(&YM2610.OPN.ST.TB, 1);
	state_load_long(&YM2610.OPN.ST.TBC, 1);

	for (ch = 0; ch < 6; ch++)
	{
		state_load_long(YM2610.CH[ch].op1_out, 2);
		state_load_long(&YM2610.CH[ch].fc, 1);

		for (slot = 0; slot < 4; slot++)
		{
			FM_SLOT *SLOT = &YM2610.CH[ch].SLOT[slot];

			state_load_long(&SLOT->phase, 1);
			state_load_byte(&SLOT->state, 1);
			state_load_long(&SLOT->volume, 1);
		}
	}

	state_load_long(YM2610.OPN.SL3.fc, 3);
	state_load_byte(&YM2610.OPN.SL3.fn_h, 1);
	state_load_byte(YM2610.OPN.SL3.kcode, 3);

	state_load_byte(&YM2610.addr_A1, 1);
	state_load_byte(&YM2610.adpcm_arrivedEndAddress, 1);

	for (ch = 0; ch < 6; ch++)
	{
		state_load_byte(&YM2610.adpcma[ch].flag, 1);
		state_load_byte(&YM2610.adpcma[ch].now_data, 1);
		state_load_long(&YM2610.adpcma[ch].now_addr, 1);
		state_load_long(&YM2610.adpcma[ch].now_step, 1);
		state_load_long(&YM2610.adpcma[ch].adpcma_acc, 1);
		state_load_long(&YM2610.adpcma[ch].adpcma_step, 1);
		state_load_long(&YM2610.adpcma[ch].adpcma_out, 1);
	}

	state_load_byte(&YM2610.adpcmb.portstate, 1);
	state_load_long(&YM2610.adpcmb.now_addr, 1);
	state_load_long(&YM2610.adpcmb.now_step, 1);
	state_load_long(&YM2610.adpcmb.acc, 1);
	state_load_long(&YM2610.adpcmb.prev_acc, 1);
	state_load_long(&YM2610.adpcmb.adpcmd, 1);
	state_load_long(&YM2610.adpcmb.adpcml, 1);

	state_load_long(&option_samplerate, 1);

	for (r = 0; r < 16; r++)
	{
		SSG_write(0, r);
		SSG_write(1, YM2610.regs[r]);
	}

	for (r = 0x30; r <0x9e; r++)
	{
		if ((r & 3) != 3)
		{
			OPNWriteReg(&YM2610.OPN, r, YM2610.regs[r]);
			OPNWriteReg(&YM2610.OPN, r | 0x100, YM2610.regs[r | 0x100]);
		}
	}

	for (r = 0xb0; r < 0xb6; r++)
	{
		if ((r & 3) != 3)
		{
			OPNWriteReg(&YM2610.OPN, r, YM2610.regs[r]);
			OPNWriteReg(&YM2610.OPN, r | 0x100, YM2610.regs[r | 0x100]);
		}
	}

	OPNB_ADPCMA_write(0x101, YM2610.regs[0x101]);
	for (r = 0; r < 6; r++)
	{
		OPNB_ADPCMA_write(r + 0x108, YM2610.regs[r + 0x108]);
		OPNB_ADPCMA_write(r + 0x110, YM2610.regs[r + 0x110]);
		OPNB_ADPCMA_write(r + 0x118, YM2610.regs[r + 0x118]);
		OPNB_ADPCMA_write(r + 0x120, YM2610.regs[r + 0x120]);
		OPNB_ADPCMA_write(r + 0x128, YM2610.regs[r + 0x128]);
	}

	YM2610.adpcmb.volume = 0;

	for (r = 1; r < 16; r++)
	OPNB_ADPCMB_write(&YM2610.adpcmb, r + 0x10, YM2610.regs[r + 0x10]);

	if (pcmbufB)
	YM2610.adpcmb.now_data = *(pcmbufB + (YM2610.adpcmb.now_addr >> 1));
}

#endif /* SAVE_STATE */
