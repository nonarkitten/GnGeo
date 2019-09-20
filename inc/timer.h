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

#ifndef _TIMER_H_
#define _TIMER_H_

typedef struct timer_struct timer_struct; 
typedef void(*timer_callback)(int);

uint32_t timer_get_time_ms(void);
timer_struct *timer_insert(uint32_t, int, timer_callback);
void timer_free(timer_struct * ts);
void timer_free_all(void);
void timer_init(void);

// call frequently to avoid missed timers
void my_timer(void);

#endif
