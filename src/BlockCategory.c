#include "BlockCategory.h"
#include <x86intrin.h>

int smallBlockSizes[SMALL_BLOCK_CATEGORIES];
// int smallBlockSizes[SMALL_BLOCK_CATEGORIES] = {
//     16, 24, 32, 48, 64, 96, 128, 192, 256, 384, 512, 768, 1024, 1536, 2048, 3072,
//     4096, 4096*2, 4096*3, 4096*4, 4096*5, 4096*6, 4096*7, 4096*8, 4096*9, 4096*10,
//     4096*11, 4096*12, 4096*13, 4096*14, 4096*15, 4096*16, 4096*17, 4096*18, 4096*19,
//     4096*20, 4096*21, 4096*22, 4096*23, 4096*24
// };

int getSmallType(uint64_t size){
    int secondBitIndex = 64 - _lzcnt_u64(size) - 2;
    int base = (secondBitIndex - 3) * 2;
    int offset1 = (size & (1UL << secondBitIndex))!=0;
    int offset2 = _bzhi_u64(size, secondBitIndex)!=0;
    return base + offset1 + offset2;
}

int getMidType(uint64_t size){
    return size/4096 - 1;
}
