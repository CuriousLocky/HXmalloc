#include <x86intrin.h>
#include "MidBlockCommon.h"
#include "MemoryPool.h"
#include "ThreadInfo.h"
#include "NonblockingStack.h"
#include "BlockCategory.h"
#include "NoisyDebug.h"

#define BITMAP_INIT 0x7fffffffffffffffUL

static NonBlockingStackBlock *getReadyStack(BlockHeader *block, int index, int midType);
static int getNewSuperBlock(int midType);

static int initMidBlock(int midType){
    uint64_t *newChunk = chunkRequest((midType+1) * 4096 * 64);
    if(__glibc_unlikely(newChunk == NULL)){
        return -1;
    }
    if(localBitmapChunkUsage >= BITMAP_CHUNK_SIZE/8 || localBitmapChunk == NULL){
        // bitmap chunk full, get a new bitmap chunk
        uint64_t *newBitmapChunk = chunkRequest(BITMAP_CHUNK_SIZE);
        if(newBitmapChunk == NULL){
            return -1;
        }
        localBitmapChunk = newBitmapChunk;
        localBitmapChunkUsage = 0;        
    }
    localMidBlockInfo[midType].activeSuperBlocks = newChunk;
    localMidBlockInfo[midType].activeSuperBlockBitMaps = 
        localBitmapChunk +
        localBitmapChunkUsage;
    localBitmapChunkUsage += 1;
    // init Bitmap
    *(localMidBlockInfo[midType].activeSuperBlockBitMaps) = BITMAP_INIT;

    // set ready stack to chunk
    *newChunk = (uint64_t)&(localMidBlockInfo[midType].cleanSuperBlockStacks);
    return 0;
}

void freeMidBlock(BlockHeader *block, int midType, int index, uint64_t *superBlockBitmap){
    uint64_t mask = 1UL << index;
    uint64_t bitmapContent = __atomic_add_fetch(superBlockBitmap, mask, __ATOMIC_RELAXED);
    int freeBlockCount = _popcnt64(bitmapContent);
    if( (index == 63 && freeBlockCount >= SUPERBLOCK_CLEANING_TARGET) ||
        (index != 63 && bitmapContent&(1UL<<63) && freeBlockCount == SUPERBLOCK_CLEANING_TARGET) ){
        // should exit cleaning stage
        NonBlockingStackBlock *stackAddr = getReadyStack(block, index, midType);
        push_nonblocking_stack(
            ((uint64_t*)block) + 2,
            (*stackAddr),
            superBlockSetNext
        );
    }
}

static BlockHeader *findLocalVictim(int midType){
    if(localMidBlockInfo[midType].activeSuperBlocks == NULL){
        // initialize
        int initResult = getNewSuperBlock(midType);
        if(initResult < 0){
            // init failed
            return NULL;
        }
    }
    int victimIndex = -1;
    uint64_t slotMask = 0;
    uint64_t *superBlock = localMidBlockInfo[midType].activeSuperBlocks;
    uint64_t *superBlockBitmap = localMidBlockInfo[midType].activeSuperBlockBitMaps;
    uint64_t superBlockBitmapContent = *superBlockBitmap;
    if(superBlockBitmapContent == 0){
        // only MSB to be used
        slotMask = (1UL << 63);
        victimIndex = 63;
        if(__glibc_unlikely(getNewSuperBlock(midType) < 0)){
            localMidBlockInfo[midType].activeSuperBlocks = NULL;
        }
    }else{
        slotMask = _blsi_u64(superBlockBitmapContent);
        victimIndex = _tzcnt_u64(slotMask);
        __atomic_fetch_and(superBlockBitmap, (~slotMask), __ATOMIC_RELAXED);
    }
    // printf("superBlockBitmapContent is 0x%lx\n", superBlockBitmapContent);
    BlockHeader *result = superBlock + (midType+1) * 4096/sizeof(uint64_t) * victimIndex;
    *(result + 1) = packHeader(midType + SMALL_BLOCK_CATEGORIES, victimIndex, superBlockBitmap);
    return result;
}

static int getNewSuperBlock(int midType){
    // check clean stack
    uint64_t *randomCleanBlock = pop_nonblocking_stack(
        (localMidBlockInfo[midType].cleanSuperBlockStacks),
        superBlockGetNext
    );
    if(randomCleanBlock != NULL){
        // new active super block from clean stack
        BlockHeader *block = randomCleanBlock - 2;
        BlockHeader header = *(block + 1);
        uint64_t *superBlockBitmap = getSuperBlockBitmap(header);
        int index = getIndex(header);
        __atomic_fetch_and(superBlockBitmap, ~(1UL<<63), __ATOMIC_RELAXED);
        uint64_t *superBlock = (uint64_t*)block - index * 4096 * (midType+1)/sizeof(uint64_t);
        localMidBlockInfo[midType].activeSuperBlocks = superBlock;
        localMidBlockInfo[midType].activeSuperBlockBitMaps = superBlockBitmap;
        return 0;
    }
    // chunk is full request a new chunk
    return initMidBlock(midType);
}

static NonBlockingStackBlock *getReadyStack(BlockHeader *block, int index, int midType){
    // Thread ID stored in first block header padding
    uint64_t *firstBlock = (uint64_t*)block - index * 4096 * (midType+1) / sizeof(uint64_t);
    return (NonBlockingStackBlock*)(*(firstBlock));
}

BlockHeader *findMidVictim(uint64_t size){
    int midType = getMidType(size);
    return findLocalVictim(midType);
}