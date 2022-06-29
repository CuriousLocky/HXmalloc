#include <threads.h>
#include <x86intrin.h>
#include "HXmalloc8.h"
#include "NonblockingStack.h"

thread_local uint64_t *localChunk8 = NULL;
volatile NonBlockingStackBlock readyChunk8Stack = {.block_16b=0};
volatile NonBlockingStackBlock cleanChunk8Stack = {.block_16b=0};

static void Block8SetNext(Block8 *prevBlock, Block8 *nextBlock){
    *((Block8 **)prevBlock) = nextBlock;
}

static Block8 *Block8GetNext(Block8 *prevBlock){
    return (Block8*)(*((Block8**)prevBlock));
}

void HXfree8(Block8 *block){
    uint64_t *chunkHead = ((uint64_t)block)&(~(BLOCK8_CHUNK_SIZE-1));
    int index = (((uint64_t)block)&(BLOCK8_CHUNK_SIZE-1))/sizeof(Block8);
    int chunkHeadOffset = (index > 64);
    index = index % 64;
    uint64_t mask = 1 << index;
    __atomic_or_fetch(chunkHead+chunkHeadOffset, mask, __ATOMIC_RELAXED);
    uint64_t chunkHeadContent = *chunkHead;
    if(chunkHeadContent & BLOCK8_CLEANING_FLAG){
        // in cleaning stage
        if(__builtin_popcountl(chunkHeadContent)+__builtin_popcountl(chunkHead[1]) >= BLOCK8_CLEANING_TARGET){
            // should exit cleaning stage
            uint64_t racingResult = __atomic_fetch_and(chunkHead, (~BLOCK8_CLEANING_FLAG), __ATOMIC_RELAXED);
            if(racingResult & BLOCK8_CLEANING_FLAG){
                // lucky audience, add the chunk to stack
                push_nonblocking_stack(((Block8*)chunkHead), readyChunk8Stack, Block8SetNext);
            }
        }
    }
}

static Block8 *findLocalVictim(){
    // assuming local chunk is not NULL
    if(localChunk8[0] != 0){
        uint64_t slotMask = _blsi_u64(localChunk8[0]);
        int index = _tzcnt_u64(slotMask);
        __atomic_fetch_and(localChunk8, (~slotMask), __ATOMIC_RELAXED);
        return localChunk8 + (2 * index);
    }
    if(localChunk8[1] != 0){
        uint64_t slotMask = _blsi_u64(localChunk8[1]);
        int index = 64 + _tzcnt_u64(slotMask);
        __atomic_fetch_and(localChunk8 + 1, (~slotMask), __ATOMIC_RELAXED);
        return localChunk8 + (2 * index);
    }

    return NULL;
}

static void getNewLocalChunk(){
    // check ready stack
    Block8 *newLocakChunk = pop_nonblocking_stack(readyChunk8Stack, Block8GetNext);
    if(newLocakChunk != NULL){
        localChunk8 = newLocakChunk;
        return;
    }
    // check clean stack
    newLocakChunk = pop_nonblocking_stack(cleanChunk8Stack, Block8GetNext);
    if(newLocakChunk != NULL){
        localChunk8 = newLocakChunk;
        return;
    }
    // get new chunk from memory pool
    // TODO: implement new chunk creation
}

Block8 *FindVictim8(){
    Block8 *result = NULL;
    // check local chunk
    result = findLocalVictim();
    if(result != NULL){
        return result;
    }
    
    // local chunk is full, set as cleaning
    __atomic_fetch_or(localChunk8, BLOCK8_CLEANING_FLAG, __ATOMIC_RELAXED);
    localChunk8 = NULL;

    // get a new local chunk and try again
    getNewLocalChunk();
    // since only this thread can allocate from the chunk, it will always success
    return findLocalVictim(); 
}