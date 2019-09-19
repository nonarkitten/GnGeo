#ifndef _STATE_H_
#define _STATE_H_

#include "emu.h"
#include "zlib.h"
 
#include <stdbool.h>

typedef enum ST_MODULE_TYPE {
    ST_68k=0,
    ST_Z80,
    ST_YM2610,
    ST_YM2610_FM,
    ST_YM2610_ADPCMA,
    ST_YM2610_ADPCMB,
    ST_TIMER,
    ST_PD4990A,
    ST_NEOGEO, /* all the other reg will go here */
    ST_MODULE_END
}ST_MODULE_TYPE;

typedef enum ST_DATA_TYPE {
    REG_uint8_t=1,
    REG_uint16_t,
    REG_uint32_t,
    REG_INT8,
    REG_INT16,
    REG_INT32
}ST_DATA_TYPE;


typedef struct ST_REG {
    char *reg_name;
    void *data;
    uint8_t num;
    uint32_t size;
    ST_DATA_TYPE type;
    struct ST_REG *next;
}ST_REG;

typedef struct ST_MODULE {
    void (*pre_save_state)(void);
    void (*post_load_state)(void);
    ST_REG *reglist;
}ST_MODULE;


typedef struct M68K_STATE {
    uint32_t dreg[8];
    uint32_t areg[8];
    uint32_t asp;
    uint32_t pc;
    uint32_t sr;
    uint32_t bank;
    uint8_t  ram[0x10000];
}M68K_STATE;

typedef struct Z80_STATE {
    uint16_t PC,SP,AF,BC,DE,HL,IX,IY;
    uint16_t AF2,BC2,DE2,HL2;
    uint8_t  R,R2,IFF1,IFF2,IM,I;
    uint8_t  IRQV,IRQL;
    uint16_t bank[4];
    uint8_t  ram[0x800];
}Z80_STATE;

typedef struct YM2610_STATE {
}YM2610_STATE;

typedef struct NEOGEO_STATE {
    uint16_t vptr;
    Sint16 modulo;
    uint8_t current_pal;
    uint8_t current_fix;
    uint8_t sram_lock;
    uint8_t sound_code;
    uint8_t pending_command;
    uint8_t result_code;
    uint8_t sram[0x10000];
    uint8_t video[0x20000];
    uint8_t pal1[0x2000], pal2[0x2000];
}NEOGEO_STATE;

//SDL_Surface *state_img;

#define STREAD  0
#define STWRITE 1

void create_state_register(ST_MODULE_TYPE module,const char *reg_name,uint8_t num,void *data,int size,ST_DATA_TYPE type);
void set_pre_save_function(ST_MODULE_TYPE module,void (*func)(void));
void set_post_load_function(ST_MODULE_TYPE module,void (*func)(void));
//SDL_Surface *load_state_img(char *game,int slot);
int load_state(char *game,int slot);
int save_state(char *game,int slot);
uint32_t how_many_slot(char *game);
 

void neogeo_init_save_state(void);

#endif

