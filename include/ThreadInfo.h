#pragma once
#include <stddef.h>
#include <stdint.h>
#include "NonblockingStack.h"
#include "SmallBlockCommon.h"
#include "MidBlockCommon.h"

typedef struct _ThreadInfo{
    unsigned int threadID;
    SmallBlockThreadInfo smallBlockInfo[SMALL_BLOCK_CATEGORIES];
    MidBlockThreadInfo midBlockInfo[MID_BLOCK_CATEGORIES];
    uint64_t *bitmapChunk;
    uint64_t bitmapChunkUsage;    
    struct _ThreadInfo *next;
}ThreadInfo;

extern ThreadInfo *threadInfoArray;

// extern __thread ThreadInfo *localThreadInfo;
extern __thread unsigned int threadID;
extern __thread SmallBlockThreadInfo *localSmallBlockInfo;
extern __thread MidBlockThreadInfo *localMidBlockInfo;
extern __thread uint64_t *localBitmapChunk;
extern __thread uint64_t localBitmapChunkUsage;

void initThreadInfoArray();