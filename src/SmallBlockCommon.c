#include <x86intrin.h>
#include "SmallBlockCommon.h"
#include "MemoryPool.h"
#include "ThreadInfo.h"
#include "NoisyDebug.h"

#define SUPERBLOCK_INIT 0x7fffffffffffffffUL
static int blockSizes[] = {
    16, 0, 32, 48, 64, 96, 128, 192, 256, 384, 512
};
static int superBlockSizes[] = {
    1024, 0, 2048, 3072, 4096, 6144, 8192, 12288, 16384, 24576, 32768
};
static int chunkSizes[] = {
    524288, 4096, 1048576, 1568768, 2093056, 3137536, 4182016, 6270976, 8359936, 12537856, 16715776
};

static unsigned int getThreadID(uint64_t *superBlockBitmap);
static void getNewSuperBlock(int type);

static void initSmallBlock(int type){
    uint64_t *newChunk = chunkRequest(chunkSizes[type]);
    *newChunk = SUPERBLOCK_INIT;
    localThreadInfo->smallBlockInfo.chunks[type] = newChunk;
    localThreadInfo->smallBlockInfo.chunkUsages[type] = 4096 - 8 + superBlockSizes[type];
    localThreadInfo->smallBlockInfo.localSuperBlockBitMaps[type] = newChunk;
    localThreadInfo->smallBlockInfo.localSuperBlocks[type] = newChunk + (4096/8) - 1;
    localThreadInfo->smallBlockInfo.managerPageUsages[type] = 1;
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
    if(__glibc_unlikely(localThreadInfo->smallBlockInfo.localSuperBlocks[type] == NULL)){
        // initialize
        initSmallBlock(type);
    }
    int victimIndex = -1;
    uint64_t slotMask = 0;
    uint64_t *superBlock = localThreadInfo->smallBlockInfo.localSuperBlocks[type];
    uint64_t *superBlockBitmap = localThreadInfo->smallBlockInfo.localSuperBlockBitMaps[type];
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
    BlockHeader *result = superBlock + (blockSizes[type]/sizeof(uint64_t)) * victimIndex;
    *result = packHeader(blockSizes[type], victimIndex, superBlockBitmap);
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
        uint64_t *superBlock = (uint64_t*)block - (getIndex(header)) * blockSizes[type]/sizeof(uint64_t);
        localThreadInfo->smallBlockInfo.localSuperBlocks[type] = superBlock;
        localThreadInfo->smallBlockInfo.localSuperBlockBitMaps[type] = superBlockBitmap;
        return;
    }
    // get new superBlock from chunk
    uint64_t *chunk = localThreadInfo->smallBlockInfo.chunks[type];
    uint64_t chunkUsage = localThreadInfo->smallBlockInfo.chunkUsages[type];
    unsigned int managerPageUsage = localThreadInfo->smallBlockInfo.managerPageUsages[type];
    if(chunkUsage < chunkSizes[type]){
        localThreadInfo->smallBlockInfo.localSuperBlocks[type] = chunk + chunkUsage/sizeof(uint64_t);
        localThreadInfo->smallBlockInfo.localSuperBlockBitMaps[type] = 
            chunk + managerPageUsage * 8 / 512 + managerPageUsage / 512;
        localThreadInfo->smallBlockInfo.chunks[type] += superBlockSizes[type];
        localThreadInfo->smallBlockInfo.managerPageUsages[type] += 1;
    }
    // chunk is full request a new chunk
    initSmallBlock(type);
}

static unsigned int getThreadID(uint64_t *superBlockBitmap){
    uint64_t *bitmapPage = (uint64_t*)(((uint64_t)superBlockBitmap) & (~4095UL));
    return bitmapPage[510]; // second to the last
}

static inline int getType(uint64_t size){
    int secondBitIndex = 64 - _lzcnt_u64(size) - 2;
    int base = (secondBitIndex - 3) * 2;
    int offset1 = (size & (1UL << secondBitIndex))!=0;
    int offset2 = _bzhi_u64(size, secondBitIndex)!=0;
    return base + offset1 + offset2;
}

void freeSmallBlock(BlockHeader *block, BlockHeader header){
    size_t size = getSize(header);
    int type = getType(size);
    _freeSmallBlock(block, header, type);
}

BlockHeader *findSmallVictim(uint64_t size){
    int type = getType(size);
    findLocalVictim(type);
}