#pragma once

#include <stdint.h>

int getSmallType(uint64_t size);

#define SMALL_BLOCK_CATEGORIES  16
#define MAX_SMALL_BLOCK_SIZE    (smallBlockSizes[SMALL_BLOCK_CATEGORIES-1])
extern int smallBlockSizes[];
extern int smallSuperBlockSizes[];
extern int smallChunkSizes[];

int getMidType(uint64_t size);

#define MID_BLOCK_CATEGORIES    128
#define MAX_MID_BLOCK_SIZE      (128 * 4096)
