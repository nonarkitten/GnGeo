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
#define MESSAGE_DELAY 3.0

#include <string.h>

#include "messages.h"
#include "video.h"
#include "emu.h"
#include "timer.h"
//#include "frame_skip.h"
#include "screen.h"
#include "sound.h"
#include <stdarg.h>

#include "font.h"

static char message[256] = { 0 };
static timer_struct *msg_timer;
extern 

void stop_message(int param) {
	msg_timer = NULL;
	message[0] = 0;
}

void draw_message(const char *string, ...) {
	va_list args;
	va_start (args, string);
	vsprintf (message, string, args);
	va_end (args);

	if (msg_timer == NULL) {
		msg_timer = timer_insert(MESSAGE_DELAY, 0, stop_message);
	} else {
		timer_set_time(msg_timer, MESSAGE_DELAY);
	}
    //strcpy(message, string);
}

static inline void draw_pixel(int X,int Y,int C) {
	bufferpixels[((X)<<1) + ((Y) * PITCH)]=(C);
}

extern void dtostr(char *out, int precision, double value);
void render_message(double fps) {
	static char display_buffer[256] = { 0 };
	uint8_t * bitmap, c;
	int len, i, x, y, bytes_per_row;
	int bits, bytes;
	char buffer[32];

	if (arg[OPTION_SHOWFPS]) {
		//fps /= 120.0;
		dtostr(buffer, 2, fps);
		len = sprintf(display_buffer, "%s fps %s", buffer, message);
	} else {
		len = sprintf(display_buffer, "        %s", message);
	}

	bytes_per_row = (font_6x8.width + 7) / 8;
	for (i = 0; i < len; i++) {
		c = display_buffer[i];
		if ((c >= font_6x8.first_char) && (c <= font_6x8.last_char)) {
			c -= font_6x8.first_char;
			bitmap = &font_6x8.font_bitmap[(font_6x8.height * bytes_per_row) * c];
			for (y = 0; y < font_6x8.height; y++) {
				bits = font_6x8.width;
				for (bytes = 0; bytes < bytes_per_row; bytes++) {
					c = *bitmap++;
					for (x = 0; x < ((bits > 7) ? 8 : bits); x++) {
						if (c & 0x80) draw_pixel(20 + x + (bytes + (i * bytes_per_row)) * font_6x8.width, 4 + y, 0xFFFF);
						c <<= 1;
					}
					//bits -= 8;
				}
			}
		}
	}
	
}


 
