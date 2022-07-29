#include <x86intrin.h>
#include "SmallBlockCommon.h"
#include "MemoryPool.h"
#include "ThreadInfo.h"
#include "NoisyDebug.h"

#define SUPERBLOCK_INIT 0x7fffffffffffffffUL

static unsigned int getThreadID(uint64_t *superBlockBitmap);
static void getNewSuperBlock(int type);

static int initSmallBlock(int type){
    uint64_t *newChunk = chunkRequest(smallChunkSizes[type]);
    if(__glibc_unlikely(newChunk == NULL)){
        return -1;
    }
    *newChunk = SUPERBLOCK_INIT;
    localThreadInfo->smallBlockInfo.chunks[type] = newChunk;
    localThreadInfo->smallBlockInfo.chunkUsages[type] = 4096 - 8 + smallSuperBlockSizes[type];
    localThreadInfo->smallBlockInfo.activeSuperBlockBitMaps[type] = newChunk;
    localThreadInfo->smallBlockInfo.activeSuperBlocks[type] = newChunk + (4096/8) - 1;
    localThreadInfo->smallBlockInfo.managerPageUsages[type] = 1;
    // set ThreadID to chunk
    newChunk[510] = localThreadInfo->threadID;
    return 0;
}

static void _freeSmallBlock(BlockHeader *block, BlockHeader header, int type){
    uint64_t *superBlockBitmap = getSuperBlockBitmap(header);
    int index = getIndex(header);
    uint64_t mask = 1 << index;
    uint64_t bitmapContent = __atomic_or_fetch(superBlockBitmap, mask, __ATOMIC_RELAXED);
    int freeBlockCount = _popcnt64(bitmapContent);
    if( (index == 63 && freeBlockCount > SUPERBLOCK_CLEANING_TARGET) ||
        (index != 63 && bitmapContent&(1UL<<63) && freeBlockCount == SUPERBLOCK_CLEANING_TARGET) ){
        // should exit cleaning stage
        unsigned int threadID = getThreadID(superBlockBitmap);
        push_nonblocking_stack(
            ((uint64_t*)block) + 1,
            (threadInfoArray[threadID].smallBlockInfo.cleanSuperBlockStacks[type]),
            superBlockSetNext
        );
    }
}

static BlockHeader *findLocalVictim(int type){
    if(__glibc_unlikely(localThreadInfo->smallBlockInfo.activeSuperBlocks[type] == NULL)){
        // initialize
        int initResult = initSmallBlock(type);
        if(__glibc_unlikely(initResult < 0)){
            return NULL;
        }
    }
    int victimIndex = -1;
    uint64_t slotMask = 0;
    uint64_t *superBlock = localThreadInfo->smallBlockInfo.activeSuperBlocks[type];
    uint64_t *superBlockBitmap = localThreadInfo->smallBlockInfo.activeSuperBlockBitMaps[type];
    uint64_t superBlockBitmapContent = *superBlockBitmap;
    if(superBlockBitmapContent == 0){
        // only MSB to be used
        slotMask = (1UL << 63);
        victimIndex = 63;
        getNewSuperBlock(type);
    }else{
        slotMask = _blsi_u64(superBlockBitmapContent);
        victimIndex = _tzcnt_u64(slotMask);
        __atomic_fetch_and(superBlockBitmap, (~slotMask), __ATOMIC_RELAXED);
    }
    BlockHeader *result = superBlock + (smallBlockSizes[type]/sizeof(uint64_t)) * victimIndex;
    *result = packHeader(type, victimIndex, superBlockBitmap);
    return result;
}

static void getNewSuperBlock(int type){
    // check clean stack
    uint64_t *randomCleanBlock = pop_nonblocking_stack(
        (localThreadInfo->smallBlockInfo.cleanSuperBlockStacks[type]),
        superBlockGetNext
    );
    if(randomCleanBlock != NULL){
        BlockHeader *block = randomCleanBlock - 1;
        BlockHeader header = *block;
        uint64_t *superBlockBitmap = getSuperBlockBitmap(header);
        uint64_t *superBlock = (uint64_t*)block - (getIndex(header)) * smallBlockSizes[type]/sizeof(uint64_t);
        localThreadInfo->smallBlockInfo.activeSuperBlocks[type] = superBlock;
        localThreadInfo->smallBlockInfo.activeSuperBlockBitMaps[type] = superBlockBitmap;
        return;
    }
    // get new superBlock from chunk
    uint64_t *chunk = localThreadInfo->smallBlockInfo.chunks[type];
    uint64_t chunkUsage = localThreadInfo->smallBlockInfo.chunkUsages[type];
    unsigned int managerPageUsage = localThreadInfo->smallBlockInfo.managerPageUsages[type];
    if(chunkUsage < smallChunkSizes[type]){
        localThreadInfo->smallBlockInfo.activeSuperBlocks[type] = chunk + chunkUsage/sizeof(uint64_t);
        localThreadInfo->smallBlockInfo.activeSuperBlockBitMaps[type] = 
            chunk + managerPageUsage * 8 / 512 + managerPageUsage / 512;
        localThreadInfo->smallBlockInfo.chunks[type] += smallSuperBlockSizes[type];
        localThreadInfo->smallBlockInfo.managerPageUsages[type] += 1;
    }
    // chunk is full request a new chunk
    int initResult = initSmallBlock(type);
    if(__glibc_unlikely(initResult < 0)){
        localThreadInfo->smallBlockInfo.activeSuperBlocks[type] = NULL;
    }
}

static unsigned int getThreadID(uint64_t *superBlockBitmap){
    uint64_t *bitmapPage = (uint64_t*)(((uint64_t)superBlockBitmap) & (~4095UL));
    return bitmapPage[510]; // second to the last
}

void freeSmallBlock(BlockHeader *block, BlockHeader header){
    int type = getType(header);
    _freeSmallBlock(block, header, type);
}

BlockHeader *findSmallVictim(uint64_t size){
    int type = getSmallType(size);
    return findLocalVictim(type);
}