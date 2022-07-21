#pragma once
#include "HXmalloc.h"
#include "NonblockingStack.h"

typedef struct{
{{smallBlockMetaData}}
}SmallBlockThreadInfo;

void initSmallBlock();
BlockHeader *findSmallVictim(uint64_t size);
void freeSmallBlock(BlockHeader *block, BlockHeader header);
uint64_t getSmallBlockThreadID(uint64_t *superBlockBitmap);