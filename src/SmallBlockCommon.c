#include <x86intrin.h>
#include "SmallBlockCommon.h"
#include "MemoryPool.h"
#include "ThreadInfo.h"
#include "NoisyDebug.h"

#define BITMAP_INIT 0x7fffffffffffffffUL

static unsigned int getThreadID(uint64_t *superBlockBitmap);
static int getNewSuperBlock(int type);

static int initSmallChunk(int type){
    uint64_t *newChunk = chunkRequest(smallChunkSizes[type]);
    if(__glibc_unlikely(newChunk == NULL)){
        return -1;
    }
    *(newChunk + 1) = BITMAP_INIT;
    localSmallBlockInfo->chunks[type] = newChunk;
    localSmallBlockInfo->chunkUsages[type] = 4096 - 8 + smallSuperBlockSizes[type];
    localSmallBlockInfo->activeSuperBlockBitMaps[type] = newChunk + 1;
    localSmallBlockInfo->activeSuperBlocks[type] = newChunk + (4096/8) - 1;
    localSmallBlockInfo->managerPageUsages[type] = 2;
    // set ThreadID to chunk
    newChunk[0] = threadID;
    return 0;
}

void freeSmallBlock(BlockHeader *block, BlockHeader header, int type){
    uint64_t *superBlockBitmap = getSuperBlockBitmap(header);
    int index = getIndex(header);
    uint64_t mask = 1UL << index;
    uint64_t bitmapContent = __atomic_or_fetch(superBlockBitmap, mask, __ATOMIC_RELAXED);
    int freeBlockCount = _popcnt64(bitmapContent);
    if( (index == 63 && freeBlockCount >= SUPERBLOCK_CLEANING_TARGET) ||
        (index != 63 && bitmapContent&(1UL<<63) && freeBlockCount == SUPERBLOCK_CLEANING_TARGET) ){
        // should exit cleaning stage
        unsigned int blockThreadID = getThreadID(superBlockBitmap);
        if(blockThreadID == threadID){
            push_nonblocking_stack(
                ((uint64_t*)block) + 1,
                (localSmallBlockInfo->cleanSuperBlockStacks[type]),
                superBlockSetNext
            );
        }else{
            push_nonblocking_stack(
                ((uint64_t*)block) + 1,
                (threadInfoArray[blockThreadID].smallBlockInfo.cleanSuperBlockStacks[type]),
                superBlockSetNext
            );
        }
    }
}

static BlockHeader *findLocalVictim(int type){
    if(localSmallBlockInfo->activeSuperBlocks[type] == NULL){
        // initialize
        int initResult = getNewSuperBlock(type);
        if(__glibc_unlikely(initResult < 0)){
            return NULL;
        }
    }
    int victimIndex = -1;
    uint64_t slotMask = 0;
    uint64_t *superBlock = localSmallBlockInfo->activeSuperBlocks[type];
    uint64_t *superBlockBitmap = localSmallBlockInfo->activeSuperBlockBitMaps[type];
    uint64_t superBlockBitmapContent = *superBlockBitmap;
    if(superBlockBitmapContent == 0){
        // only MSB to be used
        slotMask = (1UL << 63);
        victimIndex = 63;
        if(__glibc_unlikely(getNewSuperBlock(type) < 0)){
            localSmallBlockInfo->activeSuperBlocks[type] = NULL;
        }
    }else{
        slotMask = _blsi_u64(superBlockBitmapContent);
        victimIndex = _tzcnt_u64(slotMask);
        __atomic_fetch_and(superBlockBitmap, (~slotMask), __ATOMIC_RELAXED);
    }
    BlockHeader *result = superBlock + (smallBlockSizes[type]/sizeof(uint64_t)) * victimIndex;
    *result = packHeader(type, victimIndex, superBlockBitmap);
    return result;
}

static int getNewSuperBlock(int type){
    // check clean stack
    uint64_t *randomCleanBlock = pop_nonblocking_stack(
        (localSmallBlockInfo->cleanSuperBlockStacks[type]),
        superBlockGetNext
    );
    if(randomCleanBlock != NULL){
        BlockHeader *block = randomCleanBlock - 1;
        BlockHeader header = *block;
        uint64_t *superBlockBitmap = getSuperBlockBitmap(header);
        __atomic_fetch_and(superBlockBitmap, ~(1UL<<63), __ATOMIC_RELAXED);
        uint64_t *superBlock = (uint64_t*)block - (getIndex(header)) * smallBlockSizes[type]/sizeof(uint64_t);
        localSmallBlockInfo->activeSuperBlocks[type] = superBlock;
        localSmallBlockInfo->activeSuperBlockBitMaps[type] = superBlockBitmap;
        return 0;
    }
    // get new superBlock from chunk
    uint64_t *chunk = localSmallBlockInfo->chunks[type];
    uint64_t chunkUsage = localSmallBlockInfo->chunkUsages[type];
    unsigned int managerPageUsage = localSmallBlockInfo->managerPageUsages[type];
    if(chunk != NULL && chunkUsage + smallBlockSizes[type] < smallChunkSizes[type]){
        // interleaved manager page    
        localSmallBlockInfo->activeSuperBlocks[type] = chunk + chunkUsage/sizeof(uint64_t);
        localSmallBlockInfo->activeSuperBlockBitMaps[type] = chunk + managerPageUsage;
            // chunk + managerPageUsage / 64 + managerPageUsage % 64 * 8;
        localSmallBlockInfo->chunkUsages[type] += smallSuperBlockSizes[type];
        localSmallBlockInfo->managerPageUsages[type] += 1;
        *(localSmallBlockInfo->activeSuperBlockBitMaps[type]) = BITMAP_INIT;
        return 0;
    }
    // chunk is full request a new chunk
    return initSmallChunk(type);
}

static unsigned int getThreadID(uint64_t *superBlockBitmap){
    uint64_t *bitmapPage = (uint64_t*)(((uint64_t)superBlockBitmap) & (~4095UL));
    return bitmapPage[0];
}

BlockHeader *findSmallVictim(uint64_t size){
    int type = getSmallType(size);
    return findLocalVictim(type);
}