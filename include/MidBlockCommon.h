#pragma once
#include "HXmalloc.h"
#include "NonblockingStack.h"
#include "SuperBlock.h"
#include "BlockCategory.h"

#define BITMAP_CHUNK_SIZE       16 * 1024 * 1024

typedef struct{
    NonBlockingStackBlock cleanSuperBlockStacks[MID_BLOCK_CATEGORIES];
    uint64_t *activeSuperBlocks[MID_BLOCK_CATEGORIES];
    uint64_t *activeSuperBlockBitMaps[MID_BLOCK_CATEGORIES];
    uint64_t *bitmapChunk;
    uint64_t bitmapChunkUsage;
    uint64_t *bitmapPage;
    unsigned int bitmapPageUsage;
}MidBlockThreadInfo;

BlockHeader *findMidVictim(uint64_t size);
void freeMidBlock(BlockHeader *block, BlockHeader header);