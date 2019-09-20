/***************************************************************************

  2610intf.c

  The YM2610 emulator supports up to 2 chips.
  Each chip has the following connections:
  - Status Read / Control Write A
  - Port Read / Data Write A
  - Control Write B
  - Data Write B

***************************************************************************/

#include "config.h"

#include <stdio.h>
#include "2610intf.h"
#include "conf.h"
#include "emu.h"
#include "memory.h"
#include "timer.h"

static timer_struct *Timer[2];

/*------------------------- TM2610 -------------------------------*/
/* Timer overflow callback from timer.c */
void timer_callback_2610(int param) {
	int c = param;

	Timer[c] = 0;
	YM2610TimerOver(c);
}

/* TimerHandler from fm.c */
static void TimerHandler(int c, int count, double stepTime) {
	if (count == 0) {		/* Reset FM Timer */
		if (Timer[c]) {
			timer_free(Timer[c]);
			Timer[c] = 0;
		}
	} else {			/* Start FM Timer */
		double timeSec = (double)count * stepTime;
		if (Timer[c] == 0) {
			Timer[c] = (timer_struct *)timer_insert(timeSec, c, timer_callback_2610);
		}
	}
}
void FMTimerInit(void) {
	Timer[0] = Timer[1] = 0;
	timer_free_all();
}

int YM2610_sh_start(void) {
	int rate = arg[OPTION_SAMPLERATE];
	void *pcmbufa, *pcmbufb;
	int pcmsizea, pcmsizeb;

	/* Timer Handler set */
	FMTimerInit();

	pcmbufa = (void *)memory.rom.adpcma.p;
	pcmsizea = memory.rom.adpcma.size;
	pcmbufb = (void *)memory.rom.adpcmb.p;
	pcmsizeb = memory.rom.adpcmb.size;

    /**** initialize YM2610 ****/
	/*
	   if (YM2610Init(8000000,rate,
	   pcmbufa,pcmsizea,pcmbufb,pcmsizeb,
	   TimerHandler,IRQHandler) == 0)
	 */
	YM2610Init(8000000, rate,
		pcmbufa, pcmsizea, pcmbufb, pcmsizeb,
		TimerHandler, neogeo_sound_irq);
	return 0;
}

/************************************************/
/* Sound Hardware Stop				*/
/************************************************/
void YM2610_sh_stop(void) { }

/* reset */
void YM2610_sh_reset(void) {
	YM2610Reset();
}

/************************************************/
/* Status Read for YM2610 - Chip 0		*/
/************************************************/
uint32_t YM2610_status_port_A_r(uint32_t offset) {
	return YM2610Read(0);
}

uint32_t YM2610_status_port_B_r(uint32_t offset) {
	return YM2610Read(2);
}

/************************************************/
/* Port Read for YM2610 - Chip 0		*/
/************************************************/
uint32_t YM2610_read_port_r(uint32_t offset) {
	return YM2610Read(1);
}


/************************************************/
/* Control Write for YM2610 - Chip 0		*/
/* Consists of 2 addresses			*/
/************************************************/
void YM2610_control_port_A_w(uint32_t offset, uint32_t data) {
	YM2610Write(0, data);
}

void YM2610_control_port_B_w(uint32_t offset, uint32_t data) {
	YM2610Write(2, data);
}

/************************************************/
/* Data Write for YM2610 - Chip 0		*/
/* Consists of 2 addresses			*/
/************************************************/
void YM2610_data_port_A_w(uint32_t offset, uint32_t data) {
	YM2610Write(1, data);
}

void YM2610_data_port_B_w(uint32_t offset, uint32_t data) {
	YM2610Write(3, data);
}

