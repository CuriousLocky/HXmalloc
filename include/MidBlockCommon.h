#pragma once
#include "HXmalloc.h"
#include "NonblockingStack.h"
#include "SuperBlock.h"
#include "BlockCategory.h"

#define BITMAP_CHUNK_SIZE       16 * 1024 * 1024

typedef struct{
    NonBlockingStackBlock cleanSuperBlockStacks;
    uint64_t *activeSuperBlocks;
    uint64_t *activeSuperBlockBitMaps;
}MidBlockThreadInfo;

BlockHeader *findMidVictim(uint64_t size);
void freeMidBlock(BlockHeader *block, BlockHeader header, int midType);