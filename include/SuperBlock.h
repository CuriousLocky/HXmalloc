#pragma once
#include <stdint.h>

#define SUPERBLOCK_CLEANING_FLAG    1UL
#define SUPERBLOCK_CLEANING_TARGET  32

uint64_t *superBlockGetNext(uint64_t *prevSuperBlock);

void superBlockSetNext(uint64_t *prevSuperBlock, uint64_t *nextSuperBlock);
