#pragma once

#include <stdint.h>

int getSmallType(uint64_t size);

#define SMALL_BLOCK_CATEGORIES  11
extern int smallBlockSizes[];
extern int smallSuperBlockSizes[];
extern int smallChunkSizes[];

#define MID_BLOCK_CATEGORIES    1
extern int midBlockSizes[];
extern int midSuperBlockSizes[];