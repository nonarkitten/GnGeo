#include "lz4w.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>

#include <exec/exec.h>
#include <exec/execbase.h>
#include <devices/timer.h>
#include <dos/dos.h>
#include <workbench/startup.h>

#define LITERAL_LENGTH_MASK 0xF000
#define LITERAL_LENGTH_SFT  12
#define MATCH_LENGTH_MASK   0x0F00
#define MATCH_LENGTH_SFT    8
#define MATCH_OFFSET_MASK   0x00FF
#define MATCH_OFFSET_SFT    0

#define MIN_MATCH_SIZE      1

#define BUFFSIZE            256

#define MAX(A,B) ((A)<(B)?(B):(A))

static uint16_t_t *out, *literal, *out_base, *literal_base;

typedef struct { int curOffset, refOffset, length, savedWord; } Match;

uint32_t lz4w_length(void *source) { return *(uint32_t*)source; }

static void findBestMatch_from(uint16_t_t* wdata, Match *m, int from, int ind, int wdata_size) {
	int refOffset = from;
	int curOffset = ind;
	int len = 0;
	
	while((curOffset < wdata_size) && (wdata[refOffset++] == wdata[curOffset++]) && (len < 0x10))
		len++;
    
    m->curOffset = ind;
    m->refOffset = from;
    m->length = len;
    m->savedWord = len - 1;
}

static int findBestMatch(uint16_t_t* wdata, Match *best, int ind, int wdata_size) {
	int savedWord = 0, i;
	int matched = 0;

	// nothing we can do
	if (ind >= 0) {
        // search inside the 0x100 words sized windows
        for (i = MAX(0, ind - 0x100); i < ind; i++) {
            Match match;
            
            findBestMatch_from(wdata, &match, i, ind, wdata_size);

            if (match.savedWord > savedWord) {
                *best = match;
                savedWord = match.savedWord;
                matched = 1;
            }
        }
    }
	return matched;
}

static int Match_getRelativeOffset(Match *match) {
    if(match->curOffset > match->refOffset)
        return match->curOffset - match->refOffset;
    else
        return match->refOffset - match->curOffset;
}

static void addSegment(Match* match) {
	int literal_len = literal - literal_base; // in words
	int matchAdjLength = MAX(match->length - MIN_MATCH_SIZE, 0);
	int matchOffset = Match_getRelativeOffset(match);

    *out++ = (literal_len << 12) | (matchAdjLength << 8) | ((matchOffset - 1) & 0xFF);
	CopyMem(literal_base, out, literal_len * 2);
	literal = literal_base;
	out += literal_len;
}

static void addSegment_imm(int curOff, int refOff, int len) {
	Match m = { curOff, refOff, len, len - 1 };
	addSegment(&m);
}

// out = lz4w_compress(in, in_size, silent, &out_size);
void* lz4w_compress(void* data, int in_size, int silent, int* out_size) {
	uint16_t_t* wdata = (uint16_t_t*)data;
	int wdata_size = in_size / 2;
	int ind = 0;
	Match match;

	out_base = out = (uint16_t_t*)AllocVec(in_size + (in_size >> 2), MEMF_PUBLIC);
	if(!out) return NULL;

	literal_base = literal = (uint16_t_t*)AllocVec(1024, MEMF_PUBLIC);
	if(!literal) return NULL;

	// write data length in Big Endian format
	*(uint32_t_t*)out = in_size; out += 2;
	
	// not enough data ? --> don't attempt any compression
	if (in_size < 2) {
		*(uint32_t_t*)out = 0; out += 2;
        
	} else while (ind < wdata_size) {
	
		if(findBestMatch(wdata, &match, ind, wdata_size)) {
			// match found, add and adjust index
			addSegment(&match);
			ind += match.length;
			
		} else {
			// no match found --> just add as literal
			*literal++ = wdata[ind++];

			// max size for literal, add segment
			if ((literal - literal_base) == 0xF) {
				addSegment_imm(1, 0, 0);
			}
		}
	} // end while

	// last segment
	addSegment_imm(0, 0, 0);

	// mark end with empty literal and empty match
    addSegment_imm(0, 0, 0);

	// don't forget the last byte...
	if ((in_size & 1) == 0) *out++ = 0x0000;
	else *out++ = 0x8000 | ((uint8_t_t*)data)[in_size - 1];

    FreeVec(literal_base);
    *out_size = 2 * (out - out_base);
	return (void*)out_base;
}
