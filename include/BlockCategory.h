#pragma once

#include <stdint.h>
#include "NonblockingStack.h"

int getSmallType(uint64_t size);

#define SMALL_BLOCK_CATEGORIES  64
#define MAX_SMALL_BLOCK_SIZE    (smallBlockSizes[SMALL_BLOCK_CATEGORIES-1])
extern int smallBlockSizes[];

int getMidType(uint64_t size);

#define MID_BLOCK_CATEGORIES    1
// #define MAX_MID_BLOCK_SIZE      (MID_BLOCK_CATEGORIES * 4096)
#define MAX_MID_BLOCK_SIZE      (256UL * 4096)

// chunk tag is a 16-byte data in following format
// [    8-byte readyStackAddr    ][ 2-byte category id ][   6-byte size   ]
// chunk tag is located at chunkStart + 64 - 16
static inline void packChunkTag(uint64_t *chunkStart, size_t size, int category, NonBlockingStackBlock *readyStackAddr){
    uint64_t footer = size | ((uint64_t)category << 48);
    chunkStart[7] = footer;
    chunkStart[6] = (uint64_t)readyStackAddr;
}