/***************************************************************************

  2610intf.c

  The YM2610 emulator supports up to 2 chips.
  Each chip has the following connections:
  - Status Read / Control Write A
  - Port Read / Data Write A
  - Control Write B
  - Data Write B

***************************************************************************/

#include <config.h>
#include <stdio.h>
#include "2610intf.h"
#include "conf.h"
#include "emu.h"
#include "memory.h"
#include "timer.h"

static timer_struct *Timer[2];

/*------------------------- TM2610 -------------------------------*/
/* IRQ Handler */
/*
static void IRQHandler(int n, int irq)
{
    //debug("IRQ!!!\n");
    neogeo_sound_irq(irq);
}*/

/* Timer overflow callback from timer.c */
void timer_callback_2610(int param)
{
    int c = param;

    Timer[c] = 0;
    YM2610TimerOver(c);
}

/* TimerHandler from fm.c */
//static void TimerHandler(int c, int count, double stepTime)
static void TimerHandler(int c, int count, uint32_t stepTime) {
	//debug("TimerHandler %d %d %f\n",c,count,stepTime);
	if (count == 0) {		/* Reset FM Timer */
		if (Timer[c]) {
			timer_free(Timer[c]);
			Timer[c] = 0;
		}
	} else {			/* Start FM Timer */
		//double timeSec = (double) count * stepTime;
		uint32_t timeSec = count * (uint32_t)(stepTime*(1<<TIMER_SH));
		
		if (Timer[c] == 0) {
			Timer[c] =
				(timer_struct *) timer_insert(timeSec, c,
							      timer_callback_2610);
		}
	}
}

void FMTimerInit(void)
{
    Timer[0] = Timer[1] = 0;
    timer_free_all();
}

int YM2610_sh_start(void)
{
    int rate = arg[OPTION_SAMPLERATE];
    //char buf[YM2610_NUMBUF][40];
    void *pcmbufa, *pcmbufb;
    int pcmsizea, pcmsizeb;

    /*
    if (AY8910_sh_start())
	return 1;
    */

    /* Timer Handler set */
    FMTimerInit();
/*
    for (j = 0; j < YM2610_NUMBUF; j++) {
	buf[j][0] = 0;
    }
    stream = stream_init_multi(YM2610_NUMBUF, 0, YM2610UpdateOne);
*/
    pcmbufa = (void *) memory.rom.adpcma.p;
    pcmsizea = memory.rom.adpcma.size;
    pcmbufb = (void *) memory.rom.adpcmb.p;
    pcmsizeb = memory.rom.adpcmb.size;

    //}

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
void YM2610_sh_stop(void)
{
}

/* reset */
void YM2610_sh_reset(void)
{

    YM2610Reset();
}

/************************************************/
/* Status Read for YM2610 - Chip 0		*/
/************************************************/
uint32_t YM2610_status_port_A_r(uint32_t offset)
{
    return YM2610Read(0);
}

uint32_t YM2610_status_port_B_r(uint32_t offset)
{
    return YM2610Read(2);
}

/************************************************/
/* Port Read for YM2610 - Chip 0		*/
/************************************************/
uint32_t YM2610_read_port_r(uint32_t offset)
{
    return YM2610Read(1);
}


/************************************************/
/* Control Write for YM2610 - Chip 0		*/
/* Consists of 2 addresses			*/
/************************************************/
void YM2610_control_port_A_w(uint32_t offset, uint32_t data)
{
    YM2610Write(0, data);
}

void YM2610_control_port_B_w(uint32_t offset, uint32_t data)
{
    YM2610Write(2, data);
}

/************************************************/
/* Data Write for YM2610 - Chip 0		*/
/* Consists of 2 addresses			*/
/************************************************/
void YM2610_data_port_A_w(uint32_t offset, uint32_t data)
{
    YM2610Write(1, data);
}

void YM2610_data_port_B_w(uint32_t offset, uint32_t data)
{
    YM2610Write(3, data);
}

