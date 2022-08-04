#pragma once
#include "HXmalloc.h"
#include "NonblockingStack.h"
#include "SuperBlock.h"
#include "BlockCategory.h"

typedef struct{
    NonBlockingStackBlock cleanSuperBlockStacks;
    uint64_t *activeSuperBlocks;
    uint64_t *activeSuperBlockBitMaps;
    uint64_t *chunks;
    uint64_t chunkUsages;
    unsigned int managerPageUsages;
}SmallBlockThreadInfo;

BlockHeader *findSmallVictim(uint64_t size);
void freeSmallBlock(BlockHeader *block, BlockHeader header, int type);
