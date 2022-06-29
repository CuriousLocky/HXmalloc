#include <threads.h>
#include <x86intrin.h>
#include "HXmalloc8.h"
#include "NonblockingStack.h"

thread_local uint64_t *localChunk8;
volatile NonBlockingStackBlock readyChunk8Stack = {.block_16b=0};

void Block8SetNext(Block8 *prevBlock, Block8 *nextBlock){
    *((Block8 **)prevBlock) = nextBlock;
}

Block8 *Block8GetNext(Block8 *prevBlock){
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

Block8 *findLocalVictim(){
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
    // local chunk is full, set as cleaning
    __atomic_fetch_or(localChunk8, BLOCK8_CLEANING_FLAG, __ATOMIC_RELAXED);

    return NULL;
}

