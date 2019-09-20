 
#ifndef _SCREEN_H_
#define _SCREEN_H_
 
#include <stdlib.h>
#include <ctype.h>
#include <math.h>
#include <string.h>

#include <exec/exec.h>
#include <dos/dos.h>

#include "emu.h"
//#include "list.h"

#define PITCH 768

// typedef struct RGB2YUV
// {
//   uint16_t y;
//   uint8_t  u;
//   uint8_t  v;
//   uint32_t yuy2;
// }RGB2YUV;

//extern RGB2YUV rgb2yuv[65536];
extern uint8_t* bufferpixels;
//void init_rgb2yuv_table(void);
 
//SDL_Surface *screen;
//SDL_Surface *buffer, *sprbuf, *fps_buf, *scan, *fontbuf;
//SDL_Surface *triplebuf[2];

Rect visible_area;
 
int yscreenpadding;

uint8_t interpolation;
uint8_t nblitter; 
uint8_t neffect;
uint8_t scale;
uint8_t fullscreen;

uint8_t get_effect_by_name(char *name);
uint8_t get_blitter_by_name(char *name);
void print_blitter_list(void);
void print_effect_list(void);
//void screen_change_blitter_and_effect(char *bname,char *ename);
//LIST* create_effect_list(void);
//LIST* create_blitter_list(void);

int screen_init();
int screen_reinit(void);
int screen_resize(int w, int h);
void screen_update();
void screen_close();

void screen_fullscreen();

#endif
