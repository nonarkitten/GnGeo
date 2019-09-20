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

#include <exec/types.h>
#include <exec/io.h>
#include <exec/memory.h>
#include <devices/timer.h> 
#include <proto/exec.h>
#include <proto/dos.h>
#include <clib/timer_protos.h>

#include <stdlib.h>
#include "conf.h"
#include "emu.h"
#include "timer.h"
#include "state.h"
#include "ym2610.h"

struct Library *TimerBase;

struct timer_struct {
	struct timer_struct *next;	// list of timers
	struct timeval when;		// timer interval (are eclocks faster?)
	void(*func) (int param);	// callback when tiemr expires
	int param;					// parameter to pass to callback
};

static timer_struct *timer_list = NULL;
static struct timeval now;

uint32_t timer_get_time_ms(void) {
	GetSysTime(&now);
	return now.tv_secs * 1000 + now.tv_micro / 1000;
}

void timer_set_time(timer_struct *timer, uint32_t duration_ms) {
	GetSysTime(&now);
	timer->when.tv_secs = duration_ms / 1000;
	timer->when.tv_micro = (duration_ms % 1000) * 1000;
	AddTime(&timer->when, &now);
}

timer_struct *timer_insert(uint32_t duration_ms, int param, timer_callback func) {
	timer_struct *timer = AllocVec(sizeof(timer_struct), MEMF_PUBLIC);
	if (timer) {
		// Initialize timer
		timer_set_time(timer, duration_ms);
		timer->param = param;
		timer->func = func;

		// Insert timer
		timer->next = timer_list;
		timer_list = timer;
		return timer;

	} else {
		debug("YM2610: No timer free!\n");
		return NULL;
	}
}

void timer_free_all (void) {
	timer_struct *timer = timer_list, *next;
	while (timer) {
		next = timer->next;
		FreeVec(timer);
		timer = next;
	}
}

void timer_free(timer_struct * ts) {
	timer_struct *timer = &timer_list;
	while (timer) {
		if (timer->next == ts) {
			timer->next = timer->next->next;
			FreeVec(timer->next);
			break;
		}
		timer = timer->next;
	}
}

void timer_init(void) {
	static struct IORequest timereq;
	OpenDevice("timer.device", 0, &timereq, 0);
	TimerBase = timereq.io_Device;
}

// call frequently to avoid missed timers
void timer_run(void) {
	static timer_struct *timer = NULL;
	timer_struct *next;
	int i;

	GetSysTime(&now);
	if (!timer) timer = timer_list;


	next = timer->next;
	if (CmpTime(&now, &timer->when) == 1) {
		if (timer->func) timer->func(timer->param);
		FreeVec(timer);
	}
	timer = next;
}
