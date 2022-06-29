#pragma once
#include "HXmalloc.h"

typedef struct{
    uint64_t size;
    uint64_t content;
}Block8;

#define BLOCK8_SUPERBLOCK_SIZE       2048UL
#define BLOCK8_CLEANING_FLAG    1UL
#define BLOCK8_CLEANING_TARGET  120UL

void HXfree8(Block8 *block);
Block8 *FindVictim8();
void initBlock8();
