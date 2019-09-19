#ifndef _MVS_H_
#define _MVS_H_

//#include "SDL.h"

/* compatibility layer */
/*
#define s8  signed char
#define s16 signed short
#define s32 signed long

#define u8  uint8_t
#define u16 uint16_t
#define u32 unsigned long
*/
#define s8  Sint8
#define s16 Sint16
#define s32 Sint32

#define u8  uint8_t
#define u16 uint16_t
#define u32 uint32_t

#define ALIGN_DATA
#ifndef static inline
#define static inline static __inline__
#endif
#define SOUND_SAMPLES 512

#define Limit(val, max, min)                    \
{                                               \
        if (val > max) val = max;               \
        else if (val < min) val = min;          \
}

#endif
