#include <x86intrin.h>
#include "smallBlock{{blockSize}}.h"
#include "MemoryPool.h"
#include "ThreadInfo.h"

#define SUPERBLOCKSIZE ({{blockSize}} * 64)
#define CHUNKSIZE ({{chunkSize}})

void initBlock{{blockSize}}(){
    uint64_t * chunk{{blockSize}} = chunkRequest(CHUNKSIZE);
    *chunk{{blockSize}} = localThreadInfo->threadID;
    localThreadInfo->smallBlockInfo.chunk{{blockSize}} = chunk{{blockSize}};
    localThreadInfo->smallBlockInfo.chunk{{blockSize}}Usage = 4096 - 8 + SUPERBLOCKSIZE;
    localThreadInfo->smallBlockInfo.localSuperBlock{{blockSize}} = chunk{{blockSize}} + (4096 / 8) - 1;
    localThreadInfo->smallBlockInfo.localSuperBlock{{blockSize}}BitMap = chunk{{blockSize}} + 8;
    localThreadInfo->smallBlockInfo.managerPageUsage{{blockSize}} = 2;
}

void freeBlock{{blockSize}}(BlockHeader *block, BlockHeader header){
    uint64_t *superBlockBitMap = getSuperBlockBitMap(header);
    int index = getIndex(header);
    uint64_t mask = 1 << index;
    uint64_t bitMapContent = __atomic_or_fetch(superBlockBitMap, mask, __ATOMIC_RELAXED);
    if(bitMapContent & SUPERBLOCK_CLEANING_FLAG){
        // in cleaning stage
        if(_popcnt64(bitMapContent) >= SUPERBLOCK_CLEANING_TARGET){
            // should exit cleaning stage
            bitMapContent = __atomic_fetch_and(superBlockBitMap, (~SUPERBLOCK_CLEANING_FLAG), __ATOMIC_RELAXED);
            if(bitMapContent & SUPERBLOCK_CLEANING_FLAG){
                // lucky audience, add the super block to stack
                unsigned int threadID = *(uint64_t*)(((uint64_t)superBlockBitMap)&(~4095UL));
                push_nonblocking_stack(
                    ((uint64_t*)block)+1, 
                    (localThreadInfo->smallBlockInfo.cleanSuperBlock{{blockSize}}Stack), 
                    superBlockSetNext
                );
            }
        }        
    }
}

static BlockHeader *findLocalVictim(){
    uint64_t localSuperBlock{{blockSize}}BitMapContent = *(localThreadInfo->smallBlockInfo.localSuperBlock{{blockSize}}BitMap);
    if(localSuperBlock{{blockSize}}BitMapContent == 0){
        // local super block empty
        return NULL;
    }
    uint64_t slotMask = _blsi_u64(localSuperBlock{{blockSize}}BitMapContent);
    int index = _tzcnt_u64(slotMask);
    __atomic_fetch_and(localThreadInfo->smallBlockInfo.localSuperBlock{{blockSize}}BitMap, (~slotMask), __ATOMIC_RELAXED);
    return (BlockHeader*)(localThreadInfo->smallBlockInfo.localSuperBlock{{blockSize}} + index);
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
    BlockHeader *result = NULL;
    // check local super block first
    result = findLocalVictim();
    if(result != NULL){
        return result;
    }

    // local super block is full, set as cleaning, and get a new one
    __atomic_fetch_or((localThreadInfo->smallBlockInfo.localSuperBlock{{blockSize}}BitMap), SUPERBLOCK_CLEANING_FLAG, __ATOMIC_RELAXED);
    getNewLocalSuperBlock();

    // check local super block again, guaranteed to success
    return findLocalVictim();
}

#undef SUPERBLOCKSIZE
#undef CHUNKSIZE
