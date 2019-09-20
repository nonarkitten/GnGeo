#ifndef _DEBUG_H_
#define _DEBUG_H_

 
void show_bt(void);
void add_bt(uint32_t pc);
int check_bp(int pc);
void add_bp(int pc);
void del_bp(int pc);
int dbg_68k_run(uint32_t nbcycle);
extern int dbg_step;

#endif
