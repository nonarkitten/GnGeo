#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdbool.h>


#include <exec/exec.h>
#include <dos/dos.h>
#include <libraries/lowlevel.h>
#include <devices/gameport.h>
#include <devices/timer.h>
#include <devices/keymap.h>
#include <devices/input.h>
#include <devices/inputevent.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/timer.h>
#include <proto/keymap.h>
#include <proto/lowlevel.h>


#include "emu.h"
#include "screen.h"
#include "event.h"
#include "memory.h"
#include "conf.h"

struct Library *LowLevelBase = NULL;

/* gameport stuff */
static struct MsgPort *gameport_mp = NULL;
static struct IOStdReq *gameport_io = NULL;
static BOOL gameport_is_open = FALSE;
static BOOL gameport_io_in_progress = FALSE;
static struct InputEvent gameport_ie;
static BYTE gameport_ct;        /* controller type */
struct GamePortTrigger gameport_gpt = {
    GPTF_UPKEYS | GPTF_DOWNKEYS,    /* gpt_Keys */
    0,                /* gpt_Timeout */
    1,                /* gpt_XDelta */
    1                /* gpt_YDelta */
};



static GNGEO_BUTTON get_mapid(char *butid) {
    debug("Get mapid %s\n",butid);
    if (!strcmp(butid,"A")) return GN_A;
    if (!strcmp(butid,"B")) return GN_B;
    if (!strcmp(butid,"C")) return GN_C;
    if (!strcmp(butid,"D")) return GN_D;

    if (!strcmp(butid,"UP")) return GN_UP;
    if (!strcmp(butid,"DOWN")) return GN_DOWN;
    if (!strcmp(butid,"UPDOWN")) return GN_UP;

    if (!strcmp(butid,"LEFT")) return GN_LEFT;
    if (!strcmp(butid,"RIGHT")) return GN_RIGHT;
    if (!strcmp(butid,"LEFTRIGHT")) return GN_LEFT;

    if (!strcmp(butid,"JOY")) return GN_UP;

    if (!strcmp(butid,"START")) return GN_START;
    if (!strcmp(butid,"COIN")) return GN_SELECT_COIN;

    if (!strcmp(butid,"MENU")) return GN_MENU_KEY;

    if (!strcmp(butid,"HOTKEY1")) return GN_HOTKEY1;
    if (!strcmp(butid,"HOTKEY2")) return GN_HOTKEY2;
    if (!strcmp(butid,"HOTKEY3")) return GN_HOTKEY3;
    if (!strcmp(butid,"HOTKEY4")) return GN_HOTKEY4;

    return GN_NONE;
}

bool create_joymap_from_string(int player,char *jconf) {
    return true;
}

extern int neo_emu_done;

// bool init_event(void) {
//     int i;

	
// //     jmap = calloc(sizeof(JOYMAP),1);
// // 
// //     create_joymap_from_string(1,CF_STR(cf_get_item_by_name("p1control")));
// //     create_joymap_from_string(2,CF_STR(cf_get_item_by_name("p2control")));
// // 
// //     jmap->jbutton = calloc(conf.nb_joy,sizeof(struct BUT_MAP*));
// //     jmap->jaxe =  calloc(conf.nb_joy,sizeof(struct BUT_MAPJAXIS*));
// //     jmap->jhat =  calloc(conf.nb_joy,sizeof(struct BUT_MAP*));

//     return true;
// }

int handle_pdep_event(void *event) {
    return 0;
}

#define EVGAME 1
#define EVMENU 2

typedef enum {
	P1_UP,
	P1_LEFT,
	P1_DOWN,
	P1_RIGHT,
	P1_RED,
	P1_BLUE,
	P1_GREEN,
	P1_YELLOW,
	P1_COINSELECT,
	P1_START,

	P2_UP,
	P2_LEFT,
	P2_DOWN,
	P2_RIGHT,
	P2_RED,
	P2_BLUE,
	P2_GREEN,
	P2_YELLOW,
	P2_COINSELECT,
	P2_START,
	
	EXIT_GNGEO,
	PAUSE_GNGEO,

	KEY_COUNT
	
} keys_t;

static struct KeyQuery keys[KEY_COUNT] = {
	{ 0x11, 0 }, { 0x20, 0 }, { 0x21, 0 }, { 0x22, 0 }, // P1 ULDR - WASD
	{ 0x23, 0 }, { 0x24, 0 }, { 0x14, 0 }, { 0x15, 0 }, // P1 RBGY - FGTY
	{ 0x50, 0 }, { 0x52, 0 },                           // P1 Select/Coin, Start - F1 F3

	{ 0x4C, 0 }, { 0x4F, 0 }, { 0x4D, 0 }, { 0x4C, 0 }, // P2 ULDR - Cursors
	{ 0x2D, 0 }, { 0x2E, 0 }, { 0x3E, 0 }, { 0x3F, 0 }, // P2 RBGY - Num 4589
	{ 0x51, 0 }, { 0x53, 0 },                           // P2 Select/Coin, Start - F2 F4

	{ 0x45, 0 },										// Quit - Esc
	{ 0x40, 0 }                                         // Pause - Space
};

int handle_event(void) {
    static ULONG previous = 0;
	UBYTE waspaused;
	ULONG getkey, joypos0, joypos1;
	UBYTE port;
	UBYTE fire1p1, fire2p1;
	UBYTE fire1p2, fire2p2;
	
    /* CD32 joypad handler code supplied by Gabry (ggreco@iol.it) */

    if (!LowLevelBase) return 0;

	//getkey = GetKey();
	waspaused = keys[PAUSE_GNGEO].kq_Pressed;

	QueryKeys(&keys, KEY_COUNT);

	if(keys[EXIT_GNGEO].kq_Pressed) exit(0);
	if(!waspaused && keys[PAUSE_GNGEO].kq_Pressed) paused ^= 1;

	// Controler bits: D C B A Right Left Down Up
	/* Update P1 */
	if(arg[OPTION_P1JOY] || arg[OPTION_P13BUTTON]) {
		joypos1 = ReadJoyPort(1);
		if(arg[OPTION_P13BUTTON]) {
			fire1p1 = (fire1p1 << 1) | !!(*(volatile uint16_t*)0xDFF016 & 0x0100);
			fire2p1 = (fire2p1 << 1) | !!(*(volatile uint16_t*)0xDFF016 & 0x0400);
			if (fire1p1 & 3 == 2) joypos1 |= JPF_BUTTON_BLUE;
			if (fire2p1 & 3 == 2) joypos1 |= JPF_BUTTON_YELLOW;
		}
	} else {
		joypos1 = 0;
	}
	
	port = 0xFF;
	if((joypos1 & JPF_JOY_DOWN) || (keys[P1_DOWN].kq_Pressed)) port &= 0xFD;
	else if((joypos1 & JPF_JOY_UP) || (keys[P1_UP].kq_Pressed)) port &= 0xFE;
	if((joypos1 & JPF_JOY_RIGHT) || (keys[P1_RIGHT].kq_Pressed)) port &= 0xF7;
	else if((joypos1 & JPF_JOY_LEFT) || (keys[P1_LEFT].kq_Pressed)) port &= 0xFB;

	if((joypos1 & JPF_BUTTON_RED) || (keys[P1_RED].kq_Pressed)) port &= 0xEF;
	if((joypos1 & JPF_BUTTON_BLUE) || (keys[P1_BLUE].kq_Pressed)) port &= 0xDF;
	if((joypos1 & JPF_BUTTON_GREEN) || (keys[P1_GREEN].kq_Pressed)) port &= 0xBF;
	if((joypos1 & JPF_BUTTON_YELLOW) || (keys[P1_YELLOW].kq_Pressed)) port &= 0x7F;
	memory.intern_p1 = port;

	joypos0 = arg[OPTION_P2JOY] ? ReadJoyPort (0) : 0;
	if(arg[OPTION_P2JOY] || arg[OPTION_P23BUTTON]) {
		joypos0 = ReadJoyPort(0);
		if(arg[OPTION_P23BUTTON]) {
			fire1p2 = (fire1p2 << 1) | !!(*(volatile uint16_t*)0xDFF016 & 0x0100);
			fire2p2 = (fire2p2 << 1) | !!(*(volatile uint16_t*)0xDFF016 & 0x0400);
			if (fire1p2 & 3 == 2) joypos0 |= JPF_BUTTON_BLUE;
			if (fire2p2 & 3 == 2) joypos0 |= JPF_BUTTON_YELLOW;
		}
	} else {
		joypos0 = 0;
	}
	port = 0xFF;
	if((joypos0 & JPF_JOY_DOWN) || (keys[P2_DOWN].kq_Pressed)) port &= 0xFD;
	else if((joypos0 & JPF_JOY_UP) || (keys[P2_UP].kq_Pressed)) port &= 0xFE;
	if((joypos0 & JPF_JOY_RIGHT) || (keys[P2_RIGHT].kq_Pressed)) port &= 0xF7;
	else if((joypos0 & JPF_JOY_LEFT) || (keys[P2_LEFT].kq_Pressed)) port &= 0xFB;

	if((joypos0 & JPF_BUTTON_RED) || (keys[P2_RED].kq_Pressed)) port &= 0xEF;
	if((joypos0 & JPF_BUTTON_BLUE) || (keys[P2_BLUE].kq_Pressed)) port &= 0xDF;
	if((joypos0 & JPF_BUTTON_GREEN) || (keys[P2_GREEN].kq_Pressed)) port &= 0xBF;
	if((joypos0 & JPF_BUTTON_YELLOW) || (keys[P2_YELLOW].kq_Pressed)) port &= 0x7F;
	memory.intern_p2 = port;

	port = 0x8F;
	if((joypos1 & JPF_BUTTON_PLAY) || (keys[P1_START].kq_Pressed)) port &= 0xFE;
	if((joypos0 & JPF_BUTTON_PLAY) || (keys[P2_START].kq_Pressed)) port &= 0xFB;
	memory.intern_start = port;

	port = 0x07;
	if(keys[P1_COINSELECT].kq_Pressed) port &= 0x06;
	if(keys[P2_COINSELECT].kq_Pressed) port &= 0x05;
	memory.intern_coin = port;

    return 0;
}

/*
int handle_event(void) {
	return handle_event_inter(EVGAME);
}
*/
static int last = -1;
static int counter = 40;

void reset_event(void) { }

int wait_event(void) {
    return 0;
}
