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
    printf("Get mapid %s\n",butid);
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

bool init_event(void) {
    int i;

    if(!LowLevelBase) LowLevelBase = (struct Library *) OpenLibrary((UBYTE *)"lowlevel.library",0);
	if(!LowLevelBase) exit(-1);
	
//     jmap = calloc(sizeof(JOYMAP),1);
// 
//     create_joymap_from_string(1,CF_STR(cf_get_item_by_name("p1control")));
//     create_joymap_from_string(2,CF_STR(cf_get_item_by_name("p2control")));
// 
//     jmap->jbutton = calloc(conf.nb_joy,sizeof(struct BUT_MAP*));
//     jmap->jaxe =  calloc(conf.nb_joy,sizeof(struct BUT_MAPJAXIS*));
//     jmap->jhat =  calloc(conf.nb_joy,sizeof(struct BUT_MAP*));

    return true;
}

int handle_pdep_event(void *event) {
    return 0;
}

#define EVGAME 1
#define EVMENU 2

// WASD default
int KB_JOY_LEFT = 0x20;
int KB_JOY_RIGHT = 0x22;
int KB_JOY_UP = 0x11;
int KB_JOY_DOWN = 0x21;

int KB_BUTTON_RED = 0x27; // K
int KB_BUTTON_BLUE = 0x28; // L
int KB_BUTTON_GREEN = 0x18; // O
int KB_BUTTON_YELLOW = 0x19; // P

int KB_P1_START = 0x40;
int KB_P1_SELECT = 0x50;
int KB_P1_COIN = 0x50;

//int KB_P2_START = ;
int KB_P2_SELECT = 0x51;
int KB_P2_COIN = 0x51;

#define KEY_COUNT 15
static struct KeyQuery keys[KEY_COUNT] = {
	{ 0x11, 0 }, { 0x20, 0 }, { 0x21, 0 }, { 0x22, 0 }, // ULDR - WASD
	{ 0x27, 0 }, { 0x28, 0 }, { 0x18, 0 }, { 0x19, 0 }, // RBGY - KLOP
	{ 0x40, 0 }, { 0x50, 0 }, { 0x50, 0 },				// P1 Start Select Coin
				 { 0x51, 0 }, { 0x51, 0 }, 				// P2       Select Coin
	{ 0x45, 0 },										// Quit
	{ 0x42, 0 }                                         // Pause
};

int handle_event(void) {
    static ULONG previous = 0;
	UBYTE waspaused;
	ULONG getkey, joypos;
	UBYTE port;
	
    /* CD32 joypad handler code supplied by Gabry (ggreco@iol.it) */

    if (!LowLevelBase) return 0;

	//getkey = GetKey();
	waspaused = keys[14].kq_Pressed;

	QueryKeys(&keys, KEY_COUNT);
	joypos = ReadJoyPort (1);

	if(keys[13].kq_Pressed) exit(0);

	if(!waspaused && keys[14].kq_Pressed) paused ^= 1;

	// Controler bits: D C B A Right Left Down Up
	/* Update P1 */
	port = 0xFF;
	if((joypos & JPF_JOY_UP) || (keys[0].kq_Pressed)) port &= 0xFE;
	if((joypos & JPF_JOY_LEFT) || (keys[1].kq_Pressed)) port &= 0xFB;
	if((joypos & JPF_JOY_DOWN) || (keys[2].kq_Pressed)) port &= 0xFD;
	if((joypos & JPF_JOY_RIGHT) || (keys[3].kq_Pressed)) port &= 0xF7;

	if((joypos & JPF_BUTTON_RED) || (keys[4].kq_Pressed)) port &= 0xEF;
	if((joypos & JPF_BUTTON_BLUE) || (keys[5].kq_Pressed)) port &= 0xDF;
	if((joypos & JPF_BUTTON_GREEN) || (keys[6].kq_Pressed)) port &= 0xBF;
	if((joypos & JPF_BUTTON_YELLOW) || (keys[7].kq_Pressed)) port &= 0x7F;
	memory.intern_p1 = port;
	
	if((joypos & JPF_BUTTON_PLAY) || (keys[8].kq_Pressed)) memory.intern_start &= 0xFE;
	else memory.intern_start |= 1;
	
	if((keys[9].kq_Pressed)) memory.intern_start &= 0xFD;
	else memory.intern_start |= 2;
	
	if((keys[10].kq_Pressed)) memory.intern_coin &= 0xFE;
	else memory.intern_coin |= 1;
	
	/* Update P2 */
	port = 0xFF;
	joypos = ReadJoyPort (0);
	if((joypos & JPF_JOY_UP)) port &= 0xFE;
	if((joypos & JPF_JOY_DOWN)) port &= 0xFD;
	if((joypos & JPF_JOY_LEFT)) port &= 0xFB;
	if((joypos & JPF_JOY_RIGHT)) port &= 0xF7;

	if((joypos & JPF_BUTTON_RED)) port &= 0xEF;
	if((joypos & JPF_BUTTON_BLUE)) port &= 0xDF;
	if((joypos & JPF_BUTTON_GREEN)) port &= 0xBF;
	if((joypos & JPF_BUTTON_YELLOW)) port &= 0x7F;
	memory.intern_p2 = port;
			
	if((joypos & JPF_BUTTON_PLAY)) memory.intern_start &= 0xFB;
	else memory.intern_start |= 4;

	if((keys[11].kq_Pressed)) memory.intern_start &= 0xF7;
	else memory.intern_start |= 8;
	
	if((keys[12].kq_Pressed)) memory.intern_coin &= 0xFD;
	else memory.intern_coin |= 2;

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
