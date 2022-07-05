#pragma once
#include "HXmalloc.h"

void initSmallBlock();
BlockHeader *findSmallVictim(uint64_t size);
void freeSmallBlock(BlockHeader *block, BlockHeader header);