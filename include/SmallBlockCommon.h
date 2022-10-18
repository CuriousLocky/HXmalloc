#pragma once
#include "HXmalloc.h"
#include "NonblockingStack.h"
#include "BlockCategory.h"

#define SUPERBLOCK_CLEANING_FLAG    1UL
#define SUPERBLOCK_CLEANING_TARGET  32

#define superBlockGetNext(prevSuperBlock) (uint64_t*)(*(uint64_t*)prevSuperBlock)

#define superBlockSetNext(prevSuperBlock, nextSuperBlock) *(prevSuperBlock)=(uint64_t)nextSuperBlock;

typedef struct{
    NonBlockingStackBlock cleanSuperBlockStack;
    uint64_t *activeSuperBlock;
    uint64_t *activeSuperBlockBitmap;
    uint64_t *chunk;
    uint64_t chunkUsage;
    uint64_t bitmapUsage;
    uint64_t padding;
}SmallBlockThreadInfo;

typedef struct{
    uint64_t *bitmap;
    uint32_t index;
}SmallBlockInfo;

static inline uint64_t getTag(uint64_t *block){
    uint64_t *chunkStart = (uint64_t*)((uint64_t)block & ~(CHUNK_SIZE-1));
    uint64_t tag = chunkStart[0];
    return tag;
}

static inline SmallBlockInfo getBlockInfo(uint64_t *block, uint32_t size){
    uint32_t superBlockBundleSize = size * 64 + 64;
    uint64_t *chunkStart = (uint64_t*)((uint64_t)block & ~(CHUNK_SIZE-1));
    uint32_t inChunkAddr = (((uint32_t)((uint64_t)block)) & (CHUNK_SIZE - 1)) - 64;
    uint32_t bundleIndex = inChunkAddr / superBlockBundleSize;
    // uint64_t *bitmap = chunkStart + (5UL - inChunkIndex/64) % (CHUNK_SIZE/sizeof(uint64_t));
    uint64_t *bitmap = (uint64_t*)((uintptr_t)chunkStart + 64 + bundleIndex * superBlockBundleSize);
    uint32_t inBundleAddr = inChunkAddr % superBlockBundleSize;
    uint32_t index = (inBundleAddr - 64)/size;
    SmallBlockInfo result = {.bitmap = bitmap, .index = index};
    return result;
}

BlockHeader *findSmallVictim(uint64_t size);
void freeSmallBlock(BlockHeader *block, int type, int index, uint64_t *superBlockBitmap);
