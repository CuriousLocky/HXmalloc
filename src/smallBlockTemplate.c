#include <threads.h>
#include <x86intrin.h>
#include "block{{blockSize}}.h"
#include "NonblockingStack.h"
#include "MemoryPool.h"

#define SUPERBLOCKSIZE ({{blockSize}} * 64)
#define CHUNKSIZE (4096/8*{{blockSize}}*64)

thread_local uint64_t *localSuperBlock{{blockSize}} = NULL;
thread_local uint64_t *localSuperBlock{{blockSize}}BitMap = NULL;

// used but already cleaned super blocks
volatile NonBlockingStackBlock cleanSuperBlock{{blockSize}}Stack = {.block_16b=0};

thread_local uint64_t *chunk{{blockSize}} = NULL;
thread_local uint64_t chunk{{blockSize}}Usage = 0;
thread_local unsigned int managerPageUsage = 0; 

void initBlock{{blockSize}}(){
    chunk{{blockSize}} = chunkRequest();
    chunk{{blockSize}}Usage = 4096 + SUPERBLOCKSIZE;
    localSuperBlock{{blockSize}} = chunk{{blockSize}} + (4096 / sizeof(uint64_t)) - 1;
    localSuperBlock{{blockSize}}BitMap = chunk{{blockSize}};
    managerPageUsage = sizeof(uint64_t);
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
                push_nonblocking_stack(((uint64_t*)block)+1, cleanSuperBlock{{blockSize}}Stack, superBlockSetNext);
            }
        }        
    }
}

static BlockHeader *findLocalVictim(){
    uint64_t localSuperBlock{{blockSize}}BitMapContent = *localSuperBlock{{blockSize}}BitMap;
    if(localSuperBlock{{blockSize}}BitMapContent == 0){
        // local super block empty
        return NULL;
    }
    uint64_t slotMask = _blsi_u64(localSuperBlock{{blockSize}}BitMapContent);
    int index = _tzcnt_u64(slotMask);
    __atomic_fetch_and(localSuperBlock{{blockSize}}BitMap, (~slotMask), __ATOMIC_RELAXED);
    return (BlockHeader*)(localSuperBlock{{blockSize}} + index);
}

static void getNewLocalSuperBlock(){
    // check clean stack
    uint64_t *randomCleanBlock = pop_nonblocking_stack(cleanSuperBlock{{blockSize}}Stack, superBlockGetNext);
    if(randomCleanBlock != NULL){
        BlockHeader *header = randomCleanBlock - 1;
        uint64_t *superBlockBitMap = getSuperBlockBitMap(header);
        uint64_t *superBlock = (uint64_t*)header - (getIndex(header)) * 2;
        localSuperBlock{{blockSize}} = superBlock;
        localSuperBlock{{blockSize}}BitMap = superBlockBitMap;
        return;
    }
    // get new superBlock from chunk
    if(chunk{{blockSize}}Usage < CHUNK_SIZE){
        localSuperBlock{{blockSize}} = chunk{{blockSize}} + chunk{{blockSize}}Usage/sizeof(uint64_t);
        localSuperBlock{{blockSize}}BitMap = chunk{{blockSize}} + managerPageUsage/sizeof(uint64_t);
        chunk{{blockSize}}Usage += SUPERBLOCKSIZE;
        managerPageUsage += sizeof(uint64_t);
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
    __atomic_fetch_or(localSuperBlock{{blockSize}}BitMap, SUPERBLOCK_CLEANING_FLAG, __ATOMIC_RELAXED);
    getNewLocalSuperBlock();

    // check local super block again, guaranteed to success
    return findLocalVictim();
}

#undef SUPERBLOCKSIZE
#undef CHUNKSIZE
