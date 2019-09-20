/* Here come as much as I can to preserve mame source code */

#ifndef H_MAME_LAYER
#define H_MAME_LAYER

#include <stdlib.h>
#include <string.h>

#include "emu.h"
#include "roms.h"

#define int8_t  int8_t
#define int16_t int16_t
#define int32_t int32_t

#define uint8_t  uint8_t
#define uint16_t uint16_t
#define uint32_t uint32_t

/* macros for accessing bytes and words within larger chunks */
#ifndef BIGENDIAN

#define uint8_t_XOR_BE(a)  				((a) ^ 1)		/* read/write a byte to a 16-bit space */
#define uint8_t_XOR_LE(a)  				(a)
#define uint8_t4_XOR_BE(a) 				((a) ^ 3)		/* read/write a byte to a 32-bit space */
#define uint8_t4_XOR_LE(a) 				(a)
#define WORD_XOR_BE(a)  				((a) ^ 2)		/* read/write a word to a 32-bit space */
#define WORD_XOR_LE(a)  				(a)
#define uint8_t8_XOR_BE(a) 				((a) ^ 7)		/* read/write a byte to a 64-bit space */
#define uint8_t8_XOR_LE(a) 				(a)
#define WORD2_XOR_BE(a)  				((a) ^ 6)		/* read/write a word to a 64-bit space */
#define WORD2_XOR_LE(a)  				(a)
#define DWORD_XOR_BE(a)  				((a) ^ 4)		/* read/write a dword to a 64-bit space */
#define DWORD_XOR_LE(a)  				(a)

#else

#define uint8_t_XOR_BE(a)  				(a)
#define uint8_t_XOR_LE(a)  				((a) ^ 1)		/* read/write a byte to a 16-bit space */
#define uint8_t4_XOR_BE(a) 				(a)
#define uint8_t4_XOR_LE(a) 				((a) ^ 3)		/* read/write a byte to a 32-bit space */
#define WORD_XOR_BE(a)  				(a)
#define WORD_XOR_LE(a)  				((a) ^ 2)		/* read/write a word to a 32-bit space */
#define uint8_t8_XOR_BE(a) 				(a)
#define uint8_t8_XOR_LE(a) 				((a) ^ 7)		/* read/write a byte to a 64-bit space */
#define WORD2_XOR_BE(a)  				(a)
#define WORD2_XOR_LE(a)  				((a) ^ 6)		/* read/write a word to a 64-bit space */
#define DWORD_XOR_BE(a)  				(a)
#define DWORD_XOR_LE(a)  				((a) ^ 4)		/* read/write a dword to a 64-bit space */

#endif

/* Useful macros to deal with bit shuffling encryptions */
#define BIT(x,n) (((x)>>(n))&1)

#define BITSWAP8(val,B7,B6,B5,B4,B3,B2,B1,B0) \
		((BIT(val,B7) << 7) | \
		 (BIT(val,B6) << 6) | \
		 (BIT(val,B5) << 5) | \
		 (BIT(val,B4) << 4) | \
		 (BIT(val,B3) << 3) | \
		 (BIT(val,B2) << 2) | \
		 (BIT(val,B1) << 1) | \
		 (BIT(val,B0) << 0))

#define BITSWAP16(val,B15,B14,B13,B12,B11,B10,B9,B8,B7,B6,B5,B4,B3,B2,B1,B0) \
		((BIT(val,B15) << 15) | \
		 (BIT(val,B14) << 14) | \
		 (BIT(val,B13) << 13) | \
		 (BIT(val,B12) << 12) | \
		 (BIT(val,B11) << 11) | \
		 (BIT(val,B10) << 10) | \
		 (BIT(val, B9) <<  9) | \
		 (BIT(val, B8) <<  8) | \
		 (BIT(val, B7) <<  7) | \
		 (BIT(val, B6) <<  6) | \
		 (BIT(val, B5) <<  5) | \
		 (BIT(val, B4) <<  4) | \
		 (BIT(val, B3) <<  3) | \
		 (BIT(val, B2) <<  2) | \
		 (BIT(val, B1) <<  1) | \
		 (BIT(val, B0) <<  0))

#define BITSWAP24(val,B23,B22,B21,B20,B19,B18,B17,B16,B15,B14,B13,B12,B11,B10,B9,B8,B7,B6,B5,B4,B3,B2,B1,B0) \
		((BIT(val,B23) << 23) | \
		 (BIT(val,B22) << 22) | \
		 (BIT(val,B21) << 21) | \
		 (BIT(val,B20) << 20) | \
		 (BIT(val,B19) << 19) | \
		 (BIT(val,B18) << 18) | \
		 (BIT(val,B17) << 17) | \
		 (BIT(val,B16) << 16) | \
		 (BIT(val,B15) << 15) | \
		 (BIT(val,B14) << 14) | \
		 (BIT(val,B13) << 13) | \
		 (BIT(val,B12) << 12) | \
		 (BIT(val,B11) << 11) | \
		 (BIT(val,B10) << 10) | \
		 (BIT(val, B9) <<  9) | \
		 (BIT(val, B8) <<  8) | \
		 (BIT(val, B7) <<  7) | \
		 (BIT(val, B6) <<  6) | \
		 (BIT(val, B5) <<  5) | \
		 (BIT(val, B4) <<  4) | \
		 (BIT(val, B3) <<  3) | \
		 (BIT(val, B2) <<  2) | \
		 (BIT(val, B1) <<  1) | \
		 (BIT(val, B0) <<  0))

#define BITSWAP32(val,B31,B30,B29,B28,B27,B26,B25,B24,B23,B22,B21,B20,B19,B18,B17,B16,B15,B14,B13,B12,B11,B10,B9,B8,B7,B6,B5,B4,B3,B2,B1,B0) \
		((BIT(val,B31) << 31) | \
		 (BIT(val,B30) << 30) | \
		 (BIT(val,B29) << 29) | \
		 (BIT(val,B28) << 28) | \
		 (BIT(val,B27) << 27) | \
		 (BIT(val,B26) << 26) | \
		 (BIT(val,B25) << 25) | \
		 (BIT(val,B24) << 24) | \
		 (BIT(val,B23) << 23) | \
		 (BIT(val,B22) << 22) | \
		 (BIT(val,B21) << 21) | \
		 (BIT(val,B20) << 20) | \
		 (BIT(val,B19) << 19) | \
		 (BIT(val,B18) << 18) | \
		 (BIT(val,B17) << 17) | \
		 (BIT(val,B16) << 16) | \
		 (BIT(val,B15) << 15) | \
		 (BIT(val,B14) << 14) | \
		 (BIT(val,B13) << 13) | \
		 (BIT(val,B12) << 12) | \
		 (BIT(val,B11) << 11) | \
		 (BIT(val,B10) << 10) | \
		 (BIT(val, B9) <<  9) | \
		 (BIT(val, B8) <<  8) | \
		 (BIT(val, B7) <<  7) | \
		 (BIT(val, B6) <<  6) | \
		 (BIT(val, B5) <<  5) | \
		 (BIT(val, B4) <<  4) | \
		 (BIT(val, B3) <<  3) | \
		 (BIT(val, B2) <<  2) | \
		 (BIT(val, B1) <<  1) | \
		 (BIT(val, B0) <<  0))



#define running_machine GAME_ROMS

//#define malloc_or_die(b) malloc(b)
#define alloc_array_or_die(type,size) ((type*)malloc_or_die(sizeof(type)*size))
uint32_t memory_region_length( GAME_ROMS *r, char *region );
uint8_t *memory_region( GAME_ROMS *r, char *region );
void *malloc_or_die(uint32_t b);

#endif
