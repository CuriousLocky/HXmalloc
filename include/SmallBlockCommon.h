#pragma once
#include "HXmalloc.h"
#include "NonblockingStack.h"
#include "SuperBlock.h"
#include "BlockCategory.h"

typedef struct{
    NonBlockingStackBlock cleanSuperBlockStacks[SMALL_BLOCK_CATEGORIES];
    uint64_t *activeSuperBlocks[SMALL_BLOCK_CATEGORIES];
    uint64_t *activeSuperBlockBitMaps[SMALL_BLOCK_CATEGORIES];
    uint64_t *chunks[SMALL_BLOCK_CATEGORIES];
    uint64_t chunkUsages[SMALL_BLOCK_CATEGORIES];
    unsigned int managerPageUsages[SMALL_BLOCK_CATEGORIES];
}SmallBlockThreadInfo;

BlockHeader *findSmallVictim(uint64_t size);
void freeSmallBlock(BlockHeader *block, BlockHeader header);
