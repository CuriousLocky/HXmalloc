#pragma once

#include <stdint.h>
#include "NonblockingStack.h"

int getSmallType(uint64_t size);

#define SMALL_BLOCK_CATEGORIES  64
#define MAX_SMALL_BLOCK_SIZE    (smallBlockSizes[SMALL_BLOCK_CATEGORIES-1])
extern int smallBlockSizes[];

int getMidType(uint64_t size);

#define MID_BLOCK_CATEGORIES    1
#define MAX_MID_BLOCK_SIZE      (MID_BLOCK_CATEGORIES * 4096)

inline void packChunkFooter(uint64_t *chunkEnd, size_t size, int category, NonBlockingStackBlock *readyStackAddr){
    uint64_t footer = size | ((uint64_t)category << 48);
    chunkEnd[-1] = footer;
    chunkEnd[-2] = (uint64_t)readyStackAddr;
}