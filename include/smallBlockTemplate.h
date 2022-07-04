#pragma once
#include <stddef.h>
#include <stdint.h>
#include "HXmalloc.h"
#include "MemoryPool.h"
#include "superBlock.h"

void initBlock{{blockSize}}();
void freeBlock{{blockSize}}(BlockHeader *block, BlockHeader header);
BlockHeader *findVictim{{blockSize}}();