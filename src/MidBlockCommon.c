#include <x86intrin.h>
#include "MidBlockCommon.h"
#include "MemoryPool.h"
#include "ThreadInfo.h"
#include "NonblockingStack.h"
#include "BlockCategory.h"

#define SUPERBLOCK_INIT 0x7fffffffffffffffUL

static unsigned int getThreadID(BlockHeader *block, int index, int midType);
static void getNewSuperBlock(int midType);

static int initMidBlock(int midType){
    uint64_t *newChunk = chunkRequest((midType+1) * 4096 * 64);
    if(__glibc_unlikely(newChunk == NULL)){
        return -1;
    }
    if(localThreadInfo->midBlockInfo.bitmapPageUsage >= 4096/8){
        // bitmap page full, get a new bitmap page
        if(localThreadInfo->midBlockInfo.bitmapChunkUsage >= BITMAP_CHUNK_SIZE){
            // bitmap chunk full, get a new bitmap chunk
            uint64_t *newBitmapChunk = chunkRequest(BITMAP_CHUNK_SIZE);
            if(newBitmapChunk == NULL){
                return -1;
            }
            localThreadInfo->midBlockInfo.bitmapChunk = newBitmapChunk;
            localThreadInfo->midBlockInfo.bitmapChunkUsage = 0;
        }
        localThreadInfo->midBlockInfo.bitmapPage = 
            localThreadInfo->midBlockInfo.bitmapChunk +
            localThreadInfo->midBlockInfo.bitmapChunkUsage/sizeof(uint64_t);
        localThreadInfo->midBlockInfo.bitmapChunkUsage += 4096;
    }

    // TODO: interleave bitmap page layout
    localThreadInfo->midBlockInfo.activeSuperBlocks[midType] = newChunk;
    localThreadInfo->midBlockInfo.activeSuperBlockBitMaps[midType] = 
        localThreadInfo->midBlockInfo.bitmapPage + 
        localThreadInfo->midBlockInfo.bitmapPageUsage;
    localThreadInfo->midBlockInfo.bitmapPageUsage ++;

    // init Bitmap
    *(localThreadInfo->midBlockInfo.activeSuperBlockBitMaps[midType]) = SUPERBLOCK_INIT;

    // set ThreadID to chunk
    *newChunk = localThreadInfo->threadID;
    return 0;
}

static void _freeMidBlock(BlockHeader *block, BlockHeader header, int midType){
    uint64_t *superBlockBitmap = getSuperBlockBitmap(header);
    int index = getIndex(header);
    uint64_t mask = 1 << index;
    uint64_t bitmapContent = __atomic_or_fetch(superBlockBitmap, mask, __ATOMIC_RELAXED);
    int freeBlockCount = _popcnt64(bitmapContent);
    if( (index == 63 && freeBlockCount > SUPERBLOCK_CLEANING_TARGET) ||
        (index != 63 && bitmapContent&(1UL<<63) && freeBlockCount == SUPERBLOCK_CLEANING_TARGET) ){
        // should exit cleaning stage
        unsigned int threadID = getThreadID(block, index, midType);
        push_nonblocking_stack(
            ((uint64_t*)block) + 1,
            (threadInfoArray[threadID].midBlockInfo.cleanSuperBlockStacks[midType]),
            superBlockSetNext
        );
    }
}

static BlockHeader *findLocalVictim(int midType){
    if(__glibc_unlikely(localThreadInfo->midBlockInfo.activeSuperBlocks[midType] == NULL)){
        // initialize
        int initResult = initMidBlock(midType);
        if(__glibc_unlikely(initResult < 0)){
            // init failed
            return NULL;
        }
    }
    int victimIndex = -1;
    uint64_t slotMask = 0;
    uint64_t *superBlock = localThreadInfo->midBlockInfo.activeSuperBlocks[midType];
    uint64_t *superBlockBitmap = localThreadInfo->midBlockInfo.activeSuperBlockBitMaps[midType];
    uint64_t superBlockBitmapContent = *superBlockBitmap;
    if(superBlockBitmapContent == 0){
        // only MSB to be used
        slotMask = (1UL << 63);
        victimIndex = 63;
        getNewSuperBlock(midType);
    }else{
        slotMask = _blsi_u64(superBlockBitmapContent);
        victimIndex = _tzcnt_u64(slotMask);
        __atomic_fetch_and(superBlockBitmap, (~slotMask), __ATOMIC_RELAXED);
    }
    BlockHeader *result = superBlock + (midType+1) * 4096/sizeof(uint64_t) * victimIndex;
    *(result + 1) = packHeader(midType + SMALL_BLOCK_CATEGORIES, victimIndex, superBlockBitmap);
    return result;
}

static void getNewSuperBlock(int midType){
    // check clean stack
    uint64_t *randomCleanBlock = pop_nonblocking_stack(
        (localThreadInfo->midBlockInfo.cleanSuperBlockStacks[midType]),
        superBlockGetNext
    );
    if(randomCleanBlock != NULL){
        // new active super block from clean stack
        BlockHeader *block = randomCleanBlock - 1;
        BlockHeader header = *block;
        uint64_t *superBlockBitmap = getSuperBlockBitmap(header);
        uint64_t *superBlock = (uint64_t*)block - (getIndex(header)) * 4096 * (midType+1)/sizeof(uint64_t);
        localThreadInfo->midBlockInfo.activeSuperBlocks[midType] = superBlock;
        localThreadInfo->midBlockInfo.activeSuperBlockBitMaps[midType] = superBlockBitmap;
        return;
    }
    // chunk is full request a new chunk
    int initResult = initMidBlock(midType);
    if(__glibc_unlikely(initResult < 0)){
        localThreadInfo->midBlockInfo.activeSuperBlocks[midType] = NULL;
    }
}

static unsigned int getThreadID(BlockHeader *block, int index, int midType){
    // Thread ID stored in first block header padding
    uint64_t *firstBlock = (uint64_t*)block - index * 4096 * (midType+1) / sizeof(uint64_t);
    return *(firstBlock);
}

void freeMidBlock(BlockHeader *block, BlockHeader header){
    int midType = getType(header) - SMALL_BLOCK_CATEGORIES;
    _freeMidBlock(block, header, midType);
}

BlockHeader *findMidVictim(uint64_t size){
    int midType = getMidType(size);
    return findLocalVictim(midType);
}