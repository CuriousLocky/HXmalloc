#pragma once
#include "HXmalloc.h"
#include "NonblockingStack.h"
#include "superBlock.h"

typedef struct{
    NonBlockingStackBlock cleanSuperBlockStacks[4];
    uint64_t *localSuperBlocks[4];
    uint64_t *localSuperBlockBitMaps[4];
    uint64_t *chunks[4];
    uint64_t chunkUsages[4];
    unsigned int managerPageUsages[4];
}SmallBlockThreadInfo;

BlockHeader *findSmallVictim(uint64_t size);
void freeSmallBlock(BlockHeader *block, BlockHeader header);
