#include <x86intrin.h>
#include "SmallBlockCommon.h"
#include "MemoryPool.h"
#include "ThreadInfo.h"
#include "NoisyDebug.h"

#define BITMAP_INIT 0x7fffffffffffffffUL

static NonBlockingStackBlock *getReadyStack(uint64_t *superBlockBitmap);
static int getNewSuperBlock(int type);

static int initSmallChunk(int type){
    // uint64_t *newChunk = chunkRequest(smallChunkSizes[type]);
    uint64_t *newChunk = chunkRequest();
    if(__glibc_unlikely(newChunk == NULL)){
        return -1;
    }
    uint64_t *chunkEnd = getChunkEnd(newChunk);
    packChunkFooter(chunkEnd, smallBlockSizes[type], type, &(localSmallBlockInfo[type].cleanSuperBlockStack));
    localSmallBlockInfo[type].chunk = newChunk;
    localSmallBlockInfo[type].superBlockUsage = smallBlockSizes[type] * 64;
    localSmallBlockInfo[type].activeSuperBlock = newChunk;
    chunkEnd[-3] = BITMAP_INIT;
    localSmallBlockInfo[type].activeSuperBlockBitmap = chunkEnd - 3;
    localSmallBlockInfo[type].bitmapUsage = 4;
    return 0;
}

void freeSmallBlock(BlockHeader *block, int type, int index, uint64_t *superBlockBitmap){
    uint64_t mask = 1UL << index;
    uint64_t bitmapContent = __atomic_add_fetch(superBlockBitmap, mask, __ATOMIC_RELAXED);
    int freeBlockCount = _popcnt64(bitmapContent);
    if( (index == 63 && freeBlockCount >= SUPERBLOCK_CLEANING_TARGET) ||
        (index != 63 && bitmapContent&(1UL<<63) && freeBlockCount == SUPERBLOCK_CLEANING_TARGET) ){
        // should exit cleaning stage
        NonBlockingStackBlock *stackAddr = getReadyStack(superBlockBitmap);
        block[1] = (uint64_t)superBlockBitmap;
        push_nonblocking_stack(
            ((uint64_t*)block),
            (*stackAddr),
            superBlockSetNext
        );
    }
}

static BlockHeader *findLocalVictim(int type){
    if(localSmallBlockInfo[type].activeSuperBlock == NULL){
        // initialize
        int initResult = getNewSuperBlock(type);
        if(__glibc_unlikely(initResult < 0)){
            return NULL;
        }
    }
    int victimIndex = -1;
    uint64_t slotMask = 0;
    uint64_t *superBlock = localSmallBlockInfo[type].activeSuperBlock;
    uint64_t *superBlockBitmap = localSmallBlockInfo[type].activeSuperBlockBitmap;
    uint64_t superBlockBitmapContent = *superBlockBitmap;
    if(superBlockBitmapContent == 0){
        // only MSB to be used
        slotMask = (1UL << 63);
        victimIndex = 63;
        if(__glibc_unlikely(getNewSuperBlock(type) < 0)){
            // failed to get a new super block, retry next time
            localSmallBlockInfo[type].activeSuperBlock = NULL;
        }
    }else{
        slotMask = _blsi_u64(superBlockBitmapContent);
        victimIndex = _tzcnt_u64(slotMask);
        __atomic_fetch_and(superBlockBitmap, (~slotMask), __ATOMIC_RELAXED);
    }
    BlockHeader *result = superBlock + (smallBlockSizes[type]/sizeof(uint64_t)) * victimIndex;
    // *result = packHeader(type, victimIndex, superBlockBitmap);
    return result;
}

static int getNewSuperBlock(int type){
    // check clean stack
    uint64_t superBlockSize = smallBlockSizes[type] * 64;
    uint64_t *randomCleanBlock = pop_nonblocking_stack(
        (localSmallBlockInfo[type].cleanSuperBlockStack),
        superBlockGetNext
    );
    if(randomCleanBlock != NULL){
        uint64_t *superBlockBitmap = (uint64_t*)randomCleanBlock[1];
        __atomic_fetch_and(superBlockBitmap, ~(1UL<<63), __ATOMIC_RELAXED);

        uint64_t inChunkAddr = ((uint64_t)randomCleanBlock) & (CHUNK_SIZE - 1);
        uint64_t chunkAddr = ((uint64_t)randomCleanBlock) & ~(CHUNK_SIZE - 1);
        uint64_t *superBlock = (uint64_t*)(chunkAddr + inChunkAddr / superBlockSize * superBlockSize);

        localSmallBlockInfo[type].activeSuperBlock = superBlock;
        localSmallBlockInfo[type].activeSuperBlockBitmap = superBlockBitmap;
        return 0;
    }
    
    uint64_t *chunk = localSmallBlockInfo[type].chunk;
    uint64_t bitmapUsage = localSmallBlockInfo[type].bitmapUsage;
    uint64_t superBlockUsage = localSmallBlockInfo[type].superBlockUsage;
    // uint64_t chunkUsage = localSmallBlockInfo[type].chunkUsage;
    // unsigned int managerPageUsage = localSmallBlockInfo[type].managerPageUsages;
    if(chunk != NULL && (superBlockUsage + superBlockSize + bitmapUsage * sizeof(uint64_t) <= CHUNK_SIZE)){
        // get new superBlock from chunk
        localSmallBlockInfo[type].activeSuperBlock = (uint64_t*)((uintptr_t)chunk + superBlockUsage);
        uint64_t *newBitmap = chunk + CHUNK_SIZE/sizeof(uint64_t) - bitmapUsage;
        *newBitmap = BITMAP_INIT;
        localSmallBlockInfo[type].activeSuperBlockBitmap = newBitmap;
        localSmallBlockInfo[type].bitmapUsage ++;
        localSmallBlockInfo[type].superBlockUsage += superBlockSize;
        return 0;
    }
    // chunk is full request a new chunk
    return initSmallChunk(type);
}

static NonBlockingStackBlock *getReadyStack(uint64_t *superBlockBitmap){
    uint64_t *chunkEnd = (uint64_t*)((((uint64_t)superBlockBitmap) & ~(CHUNK_SIZE - 1)) + CHUNK_SIZE);
    return (NonBlockingStackBlock*)(chunkEnd[-2]);
}

BlockHeader *findSmallVictim(uint64_t size){
    int type = getSmallType(size);
    __builtin_prefetch(&(localSmallBlockInfo[type]));
    return findLocalVictim(type);
}