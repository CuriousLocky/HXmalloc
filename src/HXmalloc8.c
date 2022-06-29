#include <threads.h>
#include <x86intrin.h>
#include "HXmalloc8.h"
#include "NonblockingStack.h"
#include "MemoryPool.h"

thread_local uint64_t *localSuperBlock8 = NULL;
volatile NonBlockingStackBlock readySuperBlock8Stack = {.block_16b=0};
volatile NonBlockingStackBlock cleanSuperBlock8Stack = {.block_16b=0};

// XXX: thread local chunk may or maynot cause low memory utility
// since it does aquires more memory chunks, but lazy allocation does compensate for it
thread_local uint8_t *chunk8 = NULL;
thread_local uint32_t chunk8Usage = 0;

static void Block8SetNext(Block8 *prevBlock, Block8 *nextBlock){
    *((Block8 **)prevBlock) = nextBlock;
}

static Block8 *Block8GetNext(Block8 *prevBlock){
    return (Block8*)(*((Block8**)prevBlock));
}

void initBlock8(){
    chunk8 = chunkRequest();
    chunk8Usage = BLOCK8_SUPERBLOCK_SIZE;
    localSuperBlock8 = (uint64_t *)chunk8;
}

void HXfree8(Block8 *block){
    uint64_t *superBlockHead = ((uint64_t)block)&(~(BLOCK8_SUPERBLOCK_SIZE-1));
    int index = (((uint64_t)block)&(BLOCK8_SUPERBLOCK_SIZE-1))/sizeof(Block8);
    int superBlockHeadOffset = (index > 64);
    index = index % 64;
    uint64_t mask = 1 << index;
    __atomic_or_fetch(superBlockHead+superBlockHeadOffset, mask, __ATOMIC_RELAXED);
    uint64_t superBlockHeadContent = *superBlockHead;
    if(superBlockHeadContent & BLOCK8_CLEANING_FLAG){
        // in cleaning stage
        if(__builtin_popcountl(superBlockHeadContent)+__builtin_popcountl(superBlockHead[1]) >= BLOCK8_CLEANING_TARGET){
            // should exit cleaning stage
            uint64_t racingResult = __atomic_fetch_and(superBlockHead, (~BLOCK8_CLEANING_FLAG), __ATOMIC_RELAXED);
            if(racingResult & BLOCK8_CLEANING_FLAG){
                // lucky audience, add the superBlock to stack
                push_nonblocking_stack(((Block8*)superBlockHead), readySuperBlock8Stack, Block8SetNext);
            }
        }
    }
}

static Block8 *findLocalVictim(){
    // assuming local superBlock is not NULL
    if(localSuperBlock8[0] != 0){
        uint64_t slotMask = _blsi_u64(localSuperBlock8[0]);
        int index = _tzcnt_u64(slotMask);
        __atomic_fetch_and(localSuperBlock8, (~slotMask), __ATOMIC_RELAXED);
        return localSuperBlock8 + (2 * index);
    }
    if(localSuperBlock8[1] != 0){
        uint64_t slotMask = _blsi_u64(localSuperBlock8[1]);
        int index = 64 + _tzcnt_u64(slotMask);
        __atomic_fetch_and(localSuperBlock8 + 1, (~slotMask), __ATOMIC_RELAXED);
        return localSuperBlock8 + (2 * index);
    }

    return NULL;
}

static void getNewLocalSuperBlock(){
    // check ready stack
    Block8 *newLocakSuperBlock = pop_nonblocking_stack(readySuperBlock8Stack, Block8GetNext);
    if(newLocakSuperBlock != NULL){
        localSuperBlock8 = newLocakSuperBlock;
        return;
    }
    // check clean stack
    newLocakSuperBlock = pop_nonblocking_stack(cleanSuperBlock8Stack, Block8GetNext);
    if(newLocakSuperBlock != NULL){
        localSuperBlock8 = newLocakSuperBlock;
        return;
    }
    // get new superBlock from chunk
    if(chunk8Usage < CHUNK_SIZE){
        localSuperBlock8 = (uint64_t *)(chunk8 + chunk8Usage);
        chunk8Usage += BLOCK8_SUPERBLOCK_SIZE;
        return;
    }
    // chunk is empty, request a new chunk
    // XXX: chunks are never returned to OS
    chunk8 = chunkRequest();
    chunk8Usage = BLOCK8_SUPERBLOCK_SIZE;
    localSuperBlock8 = (uint64_t *)chunk8;
    return;
}

Block8 *FindVictim8(){
    Block8 *result = NULL;
    // check local superBlock
    result = findLocalVictim();
    if(result != NULL){
        return result;
    }
    
    // local superBlock is full, set as cleaning
    __atomic_fetch_or(localSuperBlock8, BLOCK8_CLEANING_FLAG, __ATOMIC_RELAXED);
    localSuperBlock8 = NULL;

    // get a new local superBlock and try again
    getNewLocalSuperBlock();
    // since only this thread can allocate from the superBlock, it will always success
    return findLocalVictim(); 
}