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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include "conf.h"
#include "emu.h"
#include "timer.h"
#include "state.h"
#include "ym2610/ym2610.h"

typedef struct timer_struct {
	struct timer_struct *next;	// process each timer as a list
	struct TimeVal when;		// timer interval (are eclocks faster?)
	void(*func) (int param);	// callback when tiemr expires
	int param;					// parameter to pass to callback
} timer_struct;

static timer_struct *timer_list = NULL;
static struct TimeVal now;

Uint32 timer_get_time_ms(void) {
	GetSysTime(&now);
	return now.Seconds * 1000 + now.Microseconds / 1000;
}

timer_struct *insert_timer(Uint32 duration_ms, int param, timer_callback func) {
	timer_struct *timer = AllocVec(sizeof(timer_struct), MEMF_PUBLIC);
	if (timer) {
		// Initialize timer
		GetSysTime(&now);
		timer->when.Seconds = duration_ms / 1000;
		timer->when.Microseconds = (duration_ms % 1000) * 1000;
		timer->AddTime(&timer->when, now);
		timer->func = func;
		timer->param = param;

		// Insert timer
		timer->next = timer_list;
		timer_list = timer;
		return timer;

	} else {
		debug("YM2610: No timer free!\n");
		return NULL;
	}
}

void free_all_timer(void) {
	timer_struct *timer = timer_list, *next;
	while (timer) {
		next = timer->next;
		FreeVec(timer);
		timer = next;
	}
}

void del_timer(timer_struct * ts) {
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

// call frequently to avoid missed timers
void my_timer(void) {
	static timer_struct *timer = NULL;
	timer_struct *next;
	GetSysTime(&now);
	if (!timer) timer = timer_list;

	next = timer->next;
	if (CmpTime(now, timer->when) == 1) {
		if (timers[i].func) timers[i].func(timers[i].param);
		FreeVec(timer);
	}
	timer = next;
}
