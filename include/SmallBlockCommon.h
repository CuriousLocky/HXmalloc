#pragma once
#include "HXmalloc.h"
#include "NonblockingStack.h"
#include "SuperBlock.h"
#include "BlockCategory.h"

typedef struct{
    NonBlockingStackBlock cleanSuperBlockStack;
    uint64_t *activeSuperBlock;
    uint64_t *activeSuperBlockBitmap;
    uint64_t *chunk;
    uint64_t superBlockUsage;
    uint64_t bitmapUsage;
    uint64_t padding;
}SmallBlockThreadInfo;

typedef struct{
    uint64_t *bitmap;
    uint32_t index;
}SmallBlockInfo;

static inline uint64_t getTag(uint64_t *block){
    uint64_t *chunkStart = (uint64_t*)((uint64_t)block & ~(CHUNK_SIZE-1));
    uint64_t tag = chunkStart[7];
    return tag;
}

static inline SmallBlockInfo getBlockInfo(uint64_t *block, uint64_t size){
    uint64_t *chunkStart = (uint64_t*)((uint64_t)block & ~(CHUNK_SIZE-1));
    uint32_t inChunkAddr = (((uint32_t)((uint64_t)block)) & (CHUNK_SIZE - 1)) - 64;
    uint32_t inChunkIndex = inChunkAddr / size;
    uint64_t *bitmap = chunkStart + (5UL - inChunkIndex/64) % (CHUNK_SIZE/sizeof(uint64_t));
    uint32_t index = inChunkIndex % 64;
    SmallBlockInfo result = {.bitmap = bitmap, .index = index};
    return result;
}

BlockHeader *findSmallVictim(uint64_t size);
void freeSmallBlock(BlockHeader *block, int type, int index, uint64_t *superBlockBitmap);
