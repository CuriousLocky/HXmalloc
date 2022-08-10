#include <x86intrin.h>
#include "SmallBlockCommon.h"
#include "MemoryPool.h"
#include "ThreadInfo.h"
#include "NoisyDebug.h"

#define BITMAP_INIT 0x7fffffffffffffffUL

static NonBlockingStackBlock *getReadyStack(uint64_t *superBlockBitmap);
static int getNewSuperBlock(int type);

static int initSmallChunk(int type){
    uint64_t *newChunk = chunkRequest(smallChunkSizes[type]);
    if(__glibc_unlikely(newChunk == NULL)){
        return -1;
    }
    *(newChunk + 1) = BITMAP_INIT;
    localSmallBlockInfo[type].chunks = newChunk;
    localSmallBlockInfo[type].chunkUsages = 4096 - 8 + smallSuperBlockSizes[type];
    localSmallBlockInfo[type].activeSuperBlockBitMaps = newChunk + 1;
    localSmallBlockInfo[type].activeSuperBlocks = newChunk + (4096/8) - 1;
    localSmallBlockInfo[type].managerPageUsages = 2;
    // set ready stack addr to chunk
    newChunk[0] = (uint64_t)&(localSmallBlockInfo[type].cleanSuperBlockStacks);
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
        push_nonblocking_stack(
            ((uint64_t*)block) + 1,
            (*stackAddr),
            superBlockSetNext
        );
    }
}

static BlockHeader *findLocalVictim(int type){
    if(localSmallBlockInfo[type].activeSuperBlocks == NULL){
        // initialize
        int initResult = getNewSuperBlock(type);
        if(__glibc_unlikely(initResult < 0)){
            return NULL;
        }
    }
    int victimIndex = -1;
    uint64_t slotMask = 0;
    uint64_t *superBlock = localSmallBlockInfo[type].activeSuperBlocks;
    uint64_t *superBlockBitmap = localSmallBlockInfo[type].activeSuperBlockBitMaps;
    uint64_t superBlockBitmapContent = *superBlockBitmap;
    if(superBlockBitmapContent == 0){
        // only MSB to be used
        slotMask = (1UL << 63);
        victimIndex = 63;
        if(__glibc_unlikely(getNewSuperBlock(type) < 0)){
            localSmallBlockInfo[type].activeSuperBlocks = NULL;
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
        (localSmallBlockInfo[type].cleanSuperBlockStacks),
        superBlockGetNext
    );
    if(randomCleanBlock != NULL){
        BlockHeader *block = randomCleanBlock - 1;
        BlockHeader header = *block;
        uint64_t *superBlockBitmap = getSuperBlockBitmap(header);
        int index = getIndex(header);
        __atomic_fetch_and(superBlockBitmap, ~(1UL<<63), __ATOMIC_RELAXED);
        uint64_t *superBlock = (uint64_t*)block - index * smallBlockSizes[type]/sizeof(uint64_t);
        localSmallBlockInfo[type].activeSuperBlocks = superBlock;
        localSmallBlockInfo[type].activeSuperBlockBitMaps = superBlockBitmap;
        return 0;
    }
    // get new superBlock from chunk
    uint64_t *chunk = localSmallBlockInfo[type].chunks;
    uint64_t chunkUsage = localSmallBlockInfo[type].chunkUsages;
    unsigned int managerPageUsage = localSmallBlockInfo[type].managerPageUsages;
    if(chunk != NULL && chunkUsage + smallBlockSizes[type] < smallChunkSizes[type]){
        // interleaved manager page    
        localSmallBlockInfo[type].activeSuperBlocks = chunk + chunkUsage/sizeof(uint64_t);
        localSmallBlockInfo[type].activeSuperBlockBitMaps = chunk + managerPageUsage;
            // chunk + managerPageUsage / 64 + managerPageUsage % 64 * 8;
        localSmallBlockInfo[type].chunkUsages += smallSuperBlockSizes[type];
        localSmallBlockInfo[type].managerPageUsages += 1;
        *(localSmallBlockInfo[type].activeSuperBlockBitMaps) = BITMAP_INIT;
        return 0;
    }
    // chunk is full request a new chunk
    return initSmallChunk(type);
}

static NonBlockingStackBlock *getReadyStack(uint64_t *superBlockBitmap){
    uint64_t *bitmapPage = (uint64_t*)(((uint64_t)superBlockBitmap) & (~4095UL));
    return (NonBlockingStackBlock*)(bitmapPage[0]);
}

BlockHeader *findSmallVictim(uint64_t size){
    int type = getSmallType(size);
    __builtin_prefetch(&(localSmallBlockInfo[type]));
    return findLocalVictim(type);
}