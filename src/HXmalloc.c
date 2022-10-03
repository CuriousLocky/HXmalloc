#include <string.h>
#include <x86intrin.h>
#include "HXmalloc.h"
#include "SmallBlockCommon.h"
// #include "MidBlockCommon.h"
#include "BlockCategory.h"
#include "NoisyDebug.h"

__attribute__((visibility("default")))
void *malloc(size_t size) __attribute((weak, alias("hxmalloc")));

__attribute__((visibility("default")))
void *hxmalloc(size_t size){
    if(size == 0){return NULL;}
    if(size <= MAX_SMALL_BLOCK_SIZE){
        // small block
        // align to 16
        size = align(size, 16);
        return findSmallVictim(size);
    }
    if(size >= MAX_MID_BLOCK_SIZE){
        // big block
        // TODO: implement big block handler functions
        return NULL;
    }
    size = align(size, 4096);
    return NULL;
}

__attribute__((visibility("default")))
void free(void *ptr) __attribute((weak, alias("hxfree")));

__attribute__((visibility("default")))
void hxfree(void *ptr){
    if(ptr == NULL){return;}
    uint64_t *block = (uint64_t*)ptr;
    uint64_t tag = getTag(block);
    uint64_t size = tag & ((1UL << 48) - 1);
    int type = tag >> 48;
    SmallBlockInfo blockInfo = getBlockInfo(block, size);
    uint64_t *superBlockBitmap = blockInfo.bitmap;
    int index = blockInfo.index;
    if(type < SMALL_BLOCK_CATEGORIES){
        // small block
        freeSmallBlock(block, type, index, superBlockBitmap);
    }else if(type >= SMALL_BLOCK_CATEGORIES + MID_BLOCK_CATEGORIES){
        // big block
    }else{
        // mid block
        // freeMidBlock(block-1, type - SMALL_BLOCK_CATEGORIES, index, superBlockBitmap);
    }
}

// __attribute__((visibility("default")))
// size_t malloc_usable_size(void *__ptr) __attribute((weak, alias("hxmallocUsableSize")));

// __attribute__((visibility("default")))
// size_t hxmallocUsableSize(void *ptr){
//     BlockHeader *block = getBlockHeader(ptr);
//     BlockHeader header = *block;
//     int type = getType(header);
//     if(type < SMALL_BLOCK_CATEGORIES){
//         return smallBlockSizes[type];
//     }
//     type -= SMALL_BLOCK_CATEGORIES;
//     if(type < MID_BLOCK_CATEGORIES){
//         return (type + 1) * 4096;
//     }
//     return 0;
// }

__attribute__((visibility("default")))
void *calloc(size_t nmemb, size_t size) __attribute((weak, alias("hxcalloc")));

__attribute__((visibility("default")))
void *hxcalloc(size_t nmemb, size_t size){
    size_t totalSize = nmemb * size;
    void *result = hxmalloc(totalSize);
    memset(result, 0, totalSize);
    return result;
}

// __attribute__((visibility("default")))
// void *realloc(void *ptr, size_t size) __attribute((weak, alias("hxrealloc")));

// __attribute__((visibility("default")))
// void *hxrealloc(void *ptr, size_t size){
//     if(size < 0){
//         hxfree(ptr);
//     }
//     size_t usableSize = hxmallocUsableSize(ptr);
//     if(usableSize >= size){
//         return ptr;
//     }
//     void *newBlock = hxmalloc(size);
//     memcpy(newBlock, ptr, size);
//     hxfree(ptr);
//     return newBlock;
// }