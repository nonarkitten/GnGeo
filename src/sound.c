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
#include <string.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
//#include "SDL.h"
#include "sound.h"
#include "emu.h"
#include "conf.h"
#include "memory.h"
#include "profiler.h"
//#include "gnutil.h"
#include "ym2610/ym2610.h"

#include <dos/dostags.h>
#include <exec/types.h>
#include <exec/memory.h>
#include <exec/tasks.h>
#include <graphics/gfxbase.h>
#include <libraries/dos.h>
#include <devices/audio.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <stdarg.h>
 
#include <clib/exec_protos.h>
#include <clib/alib_protos.h>
#include <clib/dos_protos.h>

#include <clib/timer_protos.h>
#include <clib/exec_protos.h>


//                    ___ __        _____________ ________   
//  _____   __ __  __| _/|__| ____ |   \______   \\_____  \  
//  \__  \ |  |  \/ __ | |  |/  _ \|   ||       _/ /  / \  \ 
//   / __ \|  |  / /_/ | |  (  (_) )   ||    |   \/   \_/_  \
//  (____  /____/\____ | |__|\____/|___||____|_  /\_____\ \_/
//       \/           \/                       \/        \__)

#include <devices/audio.h>
#include <hardware/intbits.h>
#include <hardware/dmabits.h>
#include <hardware/custom.h>
#include <hardware/cia.h>
#include <exec/exec.h>
#include <proto/exec.h>

//#include "std/debug.h"
//#include "std/memory.h"
//#include "std/types.h"
//#include "system/hardware.h"
#define INTF_LEVEL3 (INTF_VERTB | INTF_BLIT | INTF_COPER)
#define INTF_LEVEL4 (INTF_AUD0 | INTF_AUD1 | INTF_AUD2 | INTF_AUD3)

extern int enablefm;
extern int enable16;

static int enablePAM;

volatile struct Custom* const custom = (void *)0xdff000;
volatile struct CIA* const ciaa = (void *)0xbfe001;
volatile struct CIA* const ciab = (void *)0xbfd000;

typedef enum { CMP_LT = -1, CMP_EQ = 0, CMP_GT = 1 } CmpT;
typedef void* PtrT;

typedef enum { CHAN_0, CHAN_1, CHAN_2, CHAN_3 } ChanT;
typedef void (*AudioIntHandlerT)(ChanT num, PtrT userData);

typedef struct MsgPort MsgPortT;
typedef struct IOAudio IOAudioT;
typedef struct IORequest IORequestT;
typedef struct Interrupt InterruptT;

typedef struct AudioInt {
	InterruptT server;
	InterruptT *oldServer;
	void (*handler)(void);
	PtrT userData;
} AudioIntT;

/* structures used by audio.device */
typedef struct Audio {
	MsgPortT *msgPort;
	IORequestT *ioReq;
	AudioIntT audioInt[4];
} AudioT;

static AudioT TheAudio;
static int AudioInitialized = 0;
static int CpuClock = 0;
static paused = 1;

__saveds __interrupt static int AudioServer(ChanT num asm("a1"));

static int InitAudio() {
	struct GfxBase *GfxBase;
	AudioT *audio = &TheAudio;

	if(AudioInitialized) return 1;

	if(NULL != (GfxBase = (struct GfxBase *)OpenLibrary("graphics.library",0L))) {
		CpuClock = (GfxBase->DisplayFlags & PAL) ? 3546895L : 3579545L;
		CloseLibrary( (struct Library *) GfxBase);
	} else {
		return 0;
	}

	memset(audio, 0, sizeof(TheAudio));

	if ((audio->msgPort = CreateMsgPort())) {
		audio->ioReq = (IORequestT *)AllocVec(sizeof(IOAudioT), MEMF_PUBLIC | MEMF_CLEAR);
		//(IORequestT *)NewRecord(IOAudioT);

		{
			IOAudioT *ioReq = (IOAudioT *)audio->ioReq;
			uint8_t channels[] = { 15 };

			ioReq->ioa_Request.io_Message.mn_ReplyPort = audio->msgPort;
			ioReq->ioa_Request.io_Message.mn_Node.ln_Pri = ADALLOC_MAXPREC;
			ioReq->ioa_Request.io_Command = ADCMD_ALLOCATE;
			ioReq->ioa_Request.io_Flags = ADIOF_NOWAIT;
			ioReq->ioa_AllocKey = 0;
			ioReq->ioa_Data = channels;
			ioReq->ioa_Length = sizeof(channels);
		}

		if (OpenDevice(AUDIONAME, 0L, audio->ioReq, 0L) == 0) {
			int i;

			for (i = 0; i < 4; i++) {
				InterruptT *audioInt = &audio->audioInt[i].server;

				audioInt->is_Node.ln_Type = NT_INTERRUPT;
				audioInt->is_Node.ln_Pri = 127;
				audioInt->is_Node.ln_Name = "AudioServer";
				audioInt->is_Data = (void *)i;
				audioInt->is_Code = (void *)AudioServer;

				audio->audioInt[i].oldServer = SetIntVector(INTB_AUD0 + i, audioInt);
			}

			AudioInitialized = 1;
			return 1;
		}

		FreeVec(audio->ioReq);
		DeleteMsgPort(audio->msgPort);
	}

	return 0;
}
static void KillAudio() {
  AudioT *audio = &TheAudio;
  int i;

	if(AudioInitialized) {
		for (i = 0; i < 4; i++)
			SetIntVector(INTB_AUD0 + i, audio->audioInt[i].oldServer);

		CloseDevice(audio->ioReq);
		FreeVec(audio->ioReq);
		DeleteMsgPort(audio->msgPort);
	}
}
static int AudioFilter(int on) {
	int old = !(ciaa->ciapra & CIAF_LED);

	if(on) ciaa->ciapra &= ~CIAF_LED;
	else ciaa->ciapra |= CIAF_LED;

	return old;
}
static void AudioSetVolume(ChanT num, uint8_t level) {
  custom->aud[num].ac_vol = (uint16_t)level;
}
static void AudioSetPeriod(ChanT num, uint16_t period) {
  custom->aud[num].ac_per = period;
}
static void AudioSetSampleRate(ChanT num, uint32_t rate) {
  custom->aud[num].ac_per = (uint16_t)(CpuClock / rate);
}
static void AudioAttachSamples(ChanT num, uint16_t *data, uint32_t length) {
  custom->aud[num].ac_ptr = data;
  custom->aud[num].ac_len = (uint16_t)(length >> 1);
}
static uint16_t *AllocAudioData(size_t length) {
  return AllocVec((length + 1) % ~1, MEMF_CHIP | MEMF_CLEAR);
}
static void FreeAudioData(uint16_t *data) {
  FreeVec(data);
}

//                                       _________________   ____ __________  ________   
//     ____   ____    ____   ____  ____ /   _____/\_____  \ |    |   \      \ \_____  \  
//    / ___\ /    \  / ___\_/ __ \/  _ \\_____  \  /  (_)  \|    |   /   |   \ |  |_)  \ 
//   / /_/  )   |  \/ /_/  )  ___(  (_) )        \/         \    |  /    |    \|        \
//   \___  /|___|  /\___  / \___  >____/_______  /\_______  /______/\____|__  /_______  /
//  /_____/      \//_____/      \/             \/         \/                \/        \/ 

#define BUFFER_LEN 256
#define PLAYBACK_RATE 27778

uint8_t *lHBuffer, *rHBuffer;
uint8_t *lLBuffer, *rLBuffer;
uint8_t *_lHBuffer, *_rHBuffer;
uint8_t *_lLBuffer, *_rLBuffer;

extern int16_t L_Mix[BUFFER_LEN];
extern int16_t R_Mix[BUFFER_LEN];

void RemixAmiga14bit(void) {
	uint32_t i;
	uint16_t lt, rt;
	uint32_t lh_out, rh_out, ll_out, rl_out;

	for(i=0; i<BUFFER_LEN; i++) {
		lt = L_Mix[i]; rt = R_Mix[i];

		/* 4-byte buffering, shift previous 8-bit up */
		lh_out <<= 8; rh_out <<= 8; 

		/* Or in the low 8-bits from each of our accumulators */
		lh_out |= (uint8_t)(lt >> 8);  
		rh_out |= (uint8_t)(rt >> 8);

		if(enable16) {
			ll_out <<= 8; rl_out <<= 8;
			ll_out |= (uint8_t)(lt & 255); 
			rl_out |= (uint8_t)(rt & 255);
		}

 		if((i & 3) == 3) {
 			/* Write out the high bytes as one 32-bit op */
 			*(uint32_t*)&lHBuffer[i - 3] = lh_out;
 			*(uint32_t*)&rHBuffer[i - 3] = rh_out;
 			
 			if(enable16) {
				/* Parallel sign extend each byte with a 2-bit right shift */
				ll_out = ((ll_out & 0xFCFCFCFC ) >> 2) | (((ll_out & 0x80808080) >> 1) * 3);
				rl_out = ((rl_out & 0xFCFCFCFC ) >> 2) | (((rl_out & 0x80808080) >> 1) * 3);
			
				/* And write out the low bytes as one 32-bit op */
				*(uint32_t*)&lLBuffer[i - 3] = ll_out;
				*(uint32_t*)&rLBuffer[i - 3] = rl_out;
 			}
 		}
	}
}

void RemixAmigaAmmx(void) {
	// To-do, make this zero-copy
	memcpy( lHBuffer, L_Mix, 2 * BUFFER_LEN );
	memcpy( rHBuffer, R_Mix, 2 * BUFFER_LEN );
}

__saveds __interrupt static int AudioServer(ChanT num asm("a1")) {
	extern void YM2610Update_Amiga(void);
	extern  uint32_t timerSound;
	uint32_t timerTemp;
	void *temp;
	
	if(!paused) {
		if(arg[OPTION_BENCH]) timerTemp = getMilliseconds();

		if(enablePAM) {
			// kick off next chunk	
			AudioAttachSamples(0, rHBuffer, BUFFER_LEN);
			AudioAttachSamples(2, lHBuffer, BUFFER_LEN);

			YM2610Update();
			RemixAmigaAmmx();

			// swap buffers
			temp = rHBuffer; rHBuffer = _rHBuffer; _rHBuffer = temp;
			temp = lHBuffer; lHBuffer = _lHBuffer; _lHBuffer = temp;
		} else {
			// kick off next chunk	
			AudioAttachSamples(0, rHBuffer, BUFFER_LEN);
			AudioAttachSamples(1, lLBuffer, BUFFER_LEN);	
			AudioAttachSamples(2, lHBuffer, BUFFER_LEN);
			AudioAttachSamples(3, rLBuffer, BUFFER_LEN);

			YM2610Update();
			RemixAmiga14bit();

			// swap buffers
			temp = rHBuffer; rHBuffer = _rHBuffer; _rHBuffer = temp;
			temp = rLBuffer; rLBuffer = _rLBuffer; _rLBuffer = temp;
			temp = lHBuffer; lHBuffer = _lHBuffer; _lHBuffer = temp;
			temp = lLBuffer; lLBuffer = _lLBuffer; _lLBuffer = temp;
		}

		if(arg[OPTION_BENCH]) timerSound += (uint32_t)((int)getMilliseconds() - (int)timerTemp);
	}
	custom->intreq = 1 << INTB_AUD0;
	return 0;
}

void pause_audio(int pause) {
	if(paused && !pause) {
		if(enablePAM) {
			YM2610Update();
			RemixAmigaAmmx();
			AudioAttachSamples(0, rHBuffer, BUFFER_LEN);
			AudioAttachSamples(2, lHBuffer, BUFFER_LEN);

			// Enabled DMA all at once
			custom->dmacon = DMAF_SETCLR 
				| (1 << DMAB_AUD0)
				| (1 << DMAB_AUD2)
				;
		} else {
			YM2610Update();
			RemixAmiga14bit();
			AudioAttachSamples(0, rHBuffer, BUFFER_LEN);
			AudioAttachSamples(1, lLBuffer, BUFFER_LEN);	
			AudioAttachSamples(2, lHBuffer, BUFFER_LEN);
			AudioAttachSamples(3, rLBuffer, BUFFER_LEN);

			// Enabled DMA all at once
			custom->dmacon = DMAF_SETCLR 
				| (1 << DMAB_AUD0)
				| (1 << DMAB_AUD1)
				| (1 << DMAB_AUD2)
				| (1 << DMAB_AUD3)
				;
		}
		// Enable interrupt handler only on channel 0
		custom->intena = INTF_SETCLR | (1 << INTB_AUD0);

	} else if(pause && !paused) {
		// Disable channel 0 interrupt handler
		custom->intena = 1 << INTB_AUD0;
		// Disable DMA on all channels immediately
		custom->dmacon = 0
			| (1 << DMAB_AUD0)
			| (1 << DMAB_AUD1)
			| (1 << DMAB_AUD2)
			| (1 << DMAB_AUD3)
			;
	}
	paused = pause;
}

struct MsgPort *TimerMP;      // Message port pointer
struct Timerequest *TimerIO;  // I/O structure pointer
#include "conf.h"
static int initd = 0;
static uint16_t oldPamelaState = 0;

int init_audio(void) { 
	int samplerate = arg[OPTION_SAMPLERATE];
	if(initd) return 1; else initd = 1;
	
	debug("Initializing sound to %dHz\r\n", samplerate);
	if((0xDFF016 & 0x7E) == 0x02) enablePAM = 1, arg[OPTION_BITRATE] = 16;
	
	if(!InitAudio()) {
		debug("Failed to initialize audio!");
		return 0;
	}

	if(enablePAM) {
		// RLLR
		lHBuffer = AllocAudioData(BUFFER_LEN * 2);
		rHBuffer = AllocAudioData(BUFFER_LEN * 2);
		_lHBuffer = AllocAudioData(BUFFER_LEN * 2);
		_rHBuffer = AllocAudioData(BUFFER_LEN * 2);

		// enable 16-bit on channels 0-3
		oldPamelaState = *((volatile uint16_t*)0xDFF29E);
		*((volatile uint16_t*)0xDFF29E) = 0x800F; 

	} else {
		// RLLR
		lHBuffer = AllocAudioData(BUFFER_LEN);
		rHBuffer = AllocAudioData(BUFFER_LEN);
		lLBuffer = AllocAudioData(BUFFER_LEN);
		rLBuffer = AllocAudioData(BUFFER_LEN);
		_lHBuffer = AllocAudioData(BUFFER_LEN);
		_rHBuffer = AllocAudioData(BUFFER_LEN);
		_lLBuffer = AllocAudioData(BUFFER_LEN);
		_rLBuffer = AllocAudioData(BUFFER_LEN);

	}
	
	AudioSetVolume(0, 64); 
	AudioSetSampleRate(0, 27776);
	AudioSetVolume(2, 64); 
	AudioSetSampleRate(2, 27776);
	if(!enablePAM) {
		AudioSetVolume(1, 1 ); 
		AudioSetSampleRate(1, 27776);
		AudioSetVolume(3, 1 ); 
		AudioSetSampleRate(3, 27776);
	} else {
		AudioSetVolume(1, 0 ); 
		AudioSetVolume(3, 0 ); 
	}

	return 1;
}

void close_audio(void) { 
	if(!initd) return; else initd = 0;

	Forbid();
	pause_audio(1);
	
	KillAudio();
	
	// clear any pending interrupt
	custom->intreq = 1 << INTB_AUD0;

	FreeAudioData(lHBuffer); lHBuffer = 0;
	FreeAudioData(rHBuffer); rHBuffer = 0;
	FreeAudioData(_lHBuffer); _lHBuffer = 0;
	FreeAudioData(_rHBuffer); _rHBuffer = 0;
	if(enablePAM) {
		*((volatile uint16_t*)0xDFF29E) = ~oldPamelaState;
		*((volatile uint16_t*)0xDFF29E) = 0x8000 | oldPamelaState;
	} else {
		FreeAudioData(lLBuffer); lLBuffer = 0;
		FreeAudioData(rLBuffer); rLBuffer = 0;
		FreeAudioData(_lLBuffer); _lLBuffer = 0;
		FreeAudioData(_rLBuffer); _rLBuffer = 0;
	}
	Permit();
	
	debug("Deinitialized sound\r\n");	
}
