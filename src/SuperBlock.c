#include "SuperBlock.h"

uint64_t *superBlockGetNext(uint64_t *prevSuperBlock){
    return (uint64_t*)(*prevSuperBlock);
}

void superBlockSetNext(uint64_t *prevSuperBlock, uint64_t *nextSuperBlock){
    *(prevSuperBlock) = (uint64_t)nextSuperBlock;
}
