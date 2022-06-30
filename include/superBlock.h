#pragma once
#include <stdint.h>

#define SUPERBLOCK_CLEANING_FLAG    1UL
#define SUPERBLOCK_CLEANING_TARGET  32

static uint64_t *superBlockGetNext(uint64_t *prevSuperBlock){
    return (uint64_t*)(*prevSuperBlock);
}

static uint64_t *superBlockSetNext(uint64_t *prevSuperBlock, uint64_t *nextSuperBlock){
    *(prevSuperBlock) = (uint64_t)nextSuperBlock;
}