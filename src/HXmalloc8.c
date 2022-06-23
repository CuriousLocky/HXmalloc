#include "HXmalloc8.h"

__thread uint64_t *localChunk8;

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
        if(__builtin_popcountl(chunkHeadContent + *(chunkHead + 1)) > BLOCK8_CLEANING_TARGET){
            // should exit cleaning stage
            uint64_t racingResult = __atomic_fetch_and(chunkHead, (~BLOCK8_CLEANING_FLAG), __ATOMIC_RELAXED);
            if(racingResult & BLOCK8_CLEANING_FLAG){
                // lucky audience, add the chunk to stack
                // TODO: add the chunk to the stack
            }
        }
    }
}

