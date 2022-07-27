#pragma once
#include "HXmalloc.h"
#include "NonblockingStack.h"
#include "superBlock.h"

#define SMALL_BLOCK_CATEGORIES  11

typedef struct{
    NonBlockingStackBlock cleanSuperBlockStacks[SMALL_BLOCK_CATEGORIES];
    uint64_t *localSuperBlocks[SMALL_BLOCK_CATEGORIES];
    uint64_t *localSuperBlockBitMaps[SMALL_BLOCK_CATEGORIES];
    uint64_t *chunks[SMALL_BLOCK_CATEGORIES];
    uint64_t chunkUsages[SMALL_BLOCK_CATEGORIES];
    unsigned int managerPageUsages[SMALL_BLOCK_CATEGORIES];
}SmallBlockThreadInfo;

BlockHeader *findSmallVictim(uint64_t size);
void freeSmallBlock(BlockHeader *block, BlockHeader header);
