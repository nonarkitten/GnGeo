#ifndef __2610INTF_H__
#define __2610INTF_H__
 
#include <config.h>
#include "ym2610.h"

#define YM2610UpdateRequest()

/************************************************/
/* Sound Hardware Start				*/
/************************************************/
int YM2610_sh_start(void);

/************************************************/
/* Sound Hardware Stop				*/
/************************************************/
void YM2610_sh_stop(void);

void YM2610_sh_reset(void);
void timer_callback_2610(int param);

/************************************************/
/* Chip 0 functions								*/
/************************************************/
uint32_t YM2610_status_port_A_r(uint32_t offset);
uint32_t YM2610_status_port_B_r(uint32_t offset);
uint32_t YM2610_read_port_r(uint32_t offset);
void YM2610_control_port_A_w(uint32_t offset, uint32_t data);
void YM2610_control_port_B_w(uint32_t offset, uint32_t data);
void YM2610_data_port_A_w(uint32_t offset, uint32_t data);
void YM2610_data_port_B_w(uint32_t offset, uint32_t data);

#endif
/**************** end of file ****************/
