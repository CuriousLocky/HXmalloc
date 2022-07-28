#include "BlockCategory.h"
#include <x86intrin.h>

int smallBlockSizes[SMALL_BLOCK_CATEGORIES] = {
    16, 24, 32, 48, 64, 96, 128, 192, 256, 384, 512
};

int getSmallType(uint64_t size){
    int secondBitIndex = 64 - _lzcnt_u64(size) - 2;
    int base = (secondBitIndex - 3) * 2;
    int offset1 = (size & (1UL << secondBitIndex))!=0;
    int offset2 = _bzhi_u64(size, secondBitIndex)!=0;
    return base + offset1 + offset2;
}

int smallSuperBlockSizes[SMALL_BLOCK_CATEGORIES] = {
    64 *    16,
    64 *    24,
    64 *    32,
    64 *    48,
    64 *    64,
    64 *    96,
    64 *    128,
    64 *    192,
    64 *    256,
    64 *    384,
    64 *    512
};
int smallChunkSizes[SMALL_BLOCK_CATEGORIES] = {
    4096 + 64 * 510 *   16 / 4096 * 4096,
    4096 + 64 * 510 *   24 / 4096 * 4096,
    4096 + 64 * 510 *   32 / 4096 * 4096,
    4096 + 64 * 510 *   48 / 4096 * 4096,
    4096 + 64 * 510 *   64 / 4096 * 4096,
    4096 + 64 * 510 *   96 / 4096 * 4096,
    4096 + 64 * 510 *   128 / 4096 * 4096,
    4096 + 64 * 510 *   192 / 4096 * 4096,
    4096 + 64 * 510 *   256 / 4096 * 4096,
    4096 + 64 * 510 *   384 / 4096 * 4096,
    4096 + 64 * 510 *   512 / 4096 * 4096,
};

int midBlockSizes[MID_BLOCK_CATEGORIES] = {

};

int midSuperBlockSizes[MID_BLOCK_CATEGORIES] = {

};
