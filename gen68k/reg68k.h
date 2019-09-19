/*****************************************************************************/
/*     Generator - Sega Genesis emulation - (c) James Ponder 1997-2001       */
/*****************************************************************************/
/*                                                                           */
/* reg68k.h                                                                  */
/*                                                                           */
/*****************************************************************************/

uint32_t reg68k_external_step(void);
uint32_t reg68k_external_execute(uint32_t clocks);
void reg68k_external_autovector(int avno);

void reg68k_internal_autovector(int avno);
void reg68k_internal_vector(int vno, uint32_t oldpc);
