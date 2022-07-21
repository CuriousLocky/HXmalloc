#include <x86intrin.h>
#include "smallBlock{{blockSize}}.h"
#include "MemoryPool.h"
#include "ThreadInfo.h"

#define SUPERBLOCKSIZE ({{blockSize}} * 64)
#define CHUNKSIZE ({{chunkSize}})

static void getNewLocalSuperBlock();
static BlockHeader *findLocalVictim();

void initBlock{{blockSize}}(){
    uint64_t * chunk{{blockSize}} = chunkRequest(CHUNKSIZE);
    *chunk{{blockSize}} = 0x7fffffffffffffffUL;
    localThreadInfo->smallBlockInfo.chunk{{blockSize}} = chunk{{blockSize}};
    localThreadInfo->smallBlockInfo.chunk{{blockSize}}Usage = 4096 - 8 + SUPERBLOCKSIZE;
    localThreadInfo->smallBlockInfo.localSuperBlock{{blockSize}} = chunk{{blockSize}} + (4096 / 8) - 1;
    localThreadInfo->smallBlockInfo.localSuperBlock{{blockSize}}BitMap = chunk{{blockSize}};
    localThreadInfo->smallBlockInfo.managerPageUsage{{blockSize}} = 2;
}

void freeBlock{{blockSize}}(BlockHeader *block, BlockHeader header){
    uint64_t *superBlockBitMap = getSuperBlockBitMap(header);
    int index = getIndex(header);
    uint64_t mask = 1 << index;
    uint64_t bitMapContent = __atomic_or_fetch(superBlockBitMap, mask, __ATOMIC_RELAXED);
    int freeBlockCount = _popcnt64(bitMapContent);
    if( (index == 63 && freeBlockCount > SUPERBLOCK_CLEANING_TARGET) ||
        ( index != 63 && bitMapContent&(1UL<<63) && freeBlockCount == SUPERBLOCK_CLEANING_TARGET) ){
        // should exit cleaning stage
        unsigned int threadID = getSmallBlockThreadID(superBlockBitMap);
        push_nonblocking_stack(
            ((uint64_t*)block)+1, 
            (threadInfoArray[threadID].smallBlockInfo.cleanSuperBlock{{blockSize}}Stack), 
            superBlockSetNext
        );        
    }
}

static BlockHeader *findLocalVictim(){
    int victimIndex = -1;
    uint64_t slotMask = 0;
    uint64_t *superBlock = localThreadInfo->smallBlockInfo.localSuperBlock{{blockSize}};
    uint64_t *localSuperBlockBitmap = localThreadInfo->smallBlockInfo.localSuperBlock{{blockSize}}BitMap;
    uint64_t localSuperBlock{{blockSize}}BitMapContent = *localSuperBlockBitmap;
    if(localSuperBlock{{blockSize}}BitMapContent == 0){
        // only MSB to be used
        slotMask = (1UL<<63);
        victimIndex = 63;
        getNewLocalSuperBlock();
    }else{
        slotMask = _blsi_u64(localSuperBlock{{blockSize}}BitMapContent);
        victimIndex = _tzcnt_u64(slotMask);
        __atomic_fetch_and(localSuperBlockBitmap, (~slotMask), __ATOMIC_RELAXED);
    }
    BlockHeader *result = superBlock + (SUPERBLOCKSIZE/sizeof(uint64_t)) * victimIndex;
    *result = packHeader({{blockSize}}, victimIndex, localSuperBlockBitmap);
    return result;
}

static void getNewLocalSuperBlock(){
    // check clean stack
    uint64_t *randomCleanBlock = pop_nonblocking_stack((localThreadInfo->smallBlockInfo.cleanSuperBlock{{blockSize}}Stack), superBlockGetNext);
    if(randomCleanBlock != NULL){
        BlockHeader *block = randomCleanBlock - 1;
        BlockHeader header = *block;
        uint64_t *superBlockBitMap = getSuperBlockBitMap(header);
        uint64_t *superBlock = (uint64_t*)block - (getIndex(header)) * 2;
        localThreadInfo->smallBlockInfo.localSuperBlock{{blockSize}} = superBlock;
        localThreadInfo->smallBlockInfo.localSuperBlock{{blockSize}}BitMap = superBlockBitMap;
        return;
    }
    // get new superBlock from chunk
    if(localThreadInfo->smallBlockInfo.chunk{{blockSize}}Usage < CHUNKSIZE){
        localThreadInfo->smallBlockInfo.localSuperBlock{{blockSize}} = 
            localThreadInfo->smallBlockInfo.chunk{{blockSize}} + 
            localThreadInfo->smallBlockInfo.chunk{{blockSize}}Usage/sizeof(uint64_t);
        localThreadInfo->smallBlockInfo.localSuperBlock{{blockSize}}BitMap = 
            localThreadInfo->smallBlockInfo.chunk{{blockSize}} + 
            8 * localThreadInfo->smallBlockInfo.managerPageUsage{{blockSize}} / 512 + 
            localThreadInfo->smallBlockInfo.managerPageUsage{{blockSize}} / 512;
        localThreadInfo->smallBlockInfo.chunk{{blockSize}}Usage += SUPERBLOCKSIZE;
        localThreadInfo->smallBlockInfo.managerPageUsage{{blockSize}} += 1;
    }
    // chunk is empty, request a new chunk
    initBlock{{blockSize}}();
}

BlockHeader *findVictim{{blockSize}}(){
    return findLocalVictim();
}

#undef SUPERBLOCKSIZE
#undef CHUNKSIZE
