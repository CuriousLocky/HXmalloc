#include <x86intrin.h>
#include "MidBlockCommon.h"
#include "MemoryPool.h"
#include "ThreadInfo.h"
#include "NonblockingStack.h"
#include "BlockCategory.h"
#include "NoisyDebug.h"

#define BITMAP_INIT 0x7fffffffffffffffUL

static unsigned int getThreadID(BlockHeader *block, int index, int midType);
static int getNewSuperBlock(int midType);

static int initMidBlock(int midType){
    uint64_t *newChunk = chunkRequest((midType+1) * 4096 * 64);
    if(__glibc_unlikely(newChunk == NULL)){
        return -1;
    }
    if(localMidBlockInfo->bitmapChunkUsage >= BITMAP_CHUNK_SIZE/8 || localMidBlockInfo->bitmapChunk == NULL){
        // bitmap chunk full, get a new bitmap chunk
        uint64_t *newBitmapChunk = chunkRequest(BITMAP_CHUNK_SIZE);
        if(newBitmapChunk == NULL){
            return -1;
        }
        localMidBlockInfo->bitmapChunk = newBitmapChunk;
        localMidBlockInfo->bitmapChunkUsage = 0;        
    }
    localMidBlockInfo->activeSuperBlocks[midType] = newChunk;
    localMidBlockInfo->activeSuperBlockBitMaps[midType] = 
        localMidBlockInfo->bitmapChunk +
        localMidBlockInfo->bitmapChunkUsage;
    localMidBlockInfo->bitmapChunkUsage += 1;
    // init Bitmap
    *(localMidBlockInfo->activeSuperBlockBitMaps[midType]) = BITMAP_INIT;

    // set ThreadID to chunk
    *newChunk = threadID;
    return 0;
}

void freeMidBlock(BlockHeader *block, BlockHeader header, int midType){
    uint64_t *superBlockBitmap = getSuperBlockBitmap(header);
    int index = getIndex(header);
    uint64_t mask = 1UL << index;
    uint64_t bitmapContent = __atomic_or_fetch(superBlockBitmap, mask, __ATOMIC_RELAXED);
    int freeBlockCount = _popcnt64(bitmapContent);
    if( (index == 63 && freeBlockCount >= SUPERBLOCK_CLEANING_TARGET) ||
        (index != 63 && bitmapContent&(1UL<<63) && freeBlockCount == SUPERBLOCK_CLEANING_TARGET) ){
        // should exit cleaning stage
        unsigned int blockThreadID = getThreadID(block, index, midType);
        if(blockThreadID == threadID){
            push_nonblocking_stack(
                ((uint64_t*)block) + 2,
                (localMidBlockInfo->cleanSuperBlockStacks[midType]),
                superBlockSetNext
            );
        }else{
            push_nonblocking_stack(
                ((uint64_t*)block) + 2,
                (threadInfoArray[blockThreadID].midBlockInfo.cleanSuperBlockStacks[midType]),
                superBlockSetNext
            );
        }
    }
}

static BlockHeader *findLocalVictim(int midType){
    if(localMidBlockInfo->activeSuperBlocks[midType] == NULL){
        // initialize
        int initResult = getNewSuperBlock(midType);
        if(initResult < 0){
            // init failed
            return NULL;
        }
    }
    int victimIndex = -1;
    uint64_t slotMask = 0;
    uint64_t *superBlock = localMidBlockInfo->activeSuperBlocks[midType];
    uint64_t *superBlockBitmap = localMidBlockInfo->activeSuperBlockBitMaps[midType];
    uint64_t superBlockBitmapContent = *superBlockBitmap;
    if(superBlockBitmapContent == 0){
        // only MSB to be used
        slotMask = (1UL << 63);
        victimIndex = 63;
        if(__glibc_unlikely(getNewSuperBlock(midType) < 0)){
            localMidBlockInfo->activeSuperBlocks[midType] = NULL;
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
        (localMidBlockInfo->cleanSuperBlockStacks[midType]),
        superBlockGetNext
    );
    if(randomCleanBlock != NULL){
        // new active super block from clean stack
        BlockHeader *block = randomCleanBlock - 2;
        BlockHeader header = *(block + 1);
        uint64_t *superBlockBitmap = getSuperBlockBitmap(header);
        __atomic_fetch_and(superBlockBitmap, ~(1UL<<63), __ATOMIC_RELAXED);
        uint64_t *superBlock = (uint64_t*)block - (getIndex(header)) * 4096 * (midType+1)/sizeof(uint64_t);
        localMidBlockInfo->activeSuperBlocks[midType] = superBlock;
        localMidBlockInfo->activeSuperBlockBitMaps[midType] = superBlockBitmap;
        return 0;
    }
    // chunk is full request a new chunk
    return initMidBlock(midType);
}

static unsigned int getThreadID(BlockHeader *block, int index, int midType){
    // Thread ID stored in first block header padding
    uint64_t *firstBlock = (uint64_t*)block - index * 4096 * (midType+1) / sizeof(uint64_t);
    return *(firstBlock);
}

BlockHeader *findMidVictim(uint64_t size){
    int midType = getMidType(size);
    return findLocalVictim(midType);
}