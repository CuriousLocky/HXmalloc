#include "HXmalloc.h"
#include "SmallBlockCommon.h"
#include "BlockCategory.h"

__attribute__((visibility("default")))
void *malloc(size_t size) __attribute((weak, alias("hxmalloc")));

__attribute__((visibility("default")))
void *hxmalloc(size_t size){
    if(size == 0){return NULL;}
    if(size + 8 <= smallBlockSizes[SMALL_BLOCK_CATEGORIES-1]){
        // small block
        // align to 16
        size += 8;
        size = align(size, 16);
        return findSmallVictim(size) + 1;
    }
    if(size >= midBlockSizes[MID_BLOCK_CATEGORIES-1]){
        // big block
        // TODO: implement big block handler functions
        return NULL;
    }
    // TODO: implement middle block handler functions
    return NULL;
}

__attribute__((visibility("default")))
void free(void *ptr) __attribute((weak, alias("hxfree")));

__attribute__((visibility("default")))
void hxfree(void *ptr){
    if(ptr == NULL){return;}
    BlockHeader *block = getBlockHeader(ptr);
    BlockHeader header = *block;
    uint64_t size = hxmallocUsableSize(ptr);
    if(size <= smallBlockSizes[SMALL_BLOCK_CATEGORIES-1]){
        // small block
        freeSmallBlock(block, header);
    }
}

__attribute__((visibility("default")))
size_t malloc_usable_size(void *__ptr) __attribute((weak, alias("hxmallocUsableSize")));

__attribute__((visibility("default")))
size_t hxmallocUsableSize(void *ptr){
    BlockHeader *block = getBlockHeader(ptr);
    BlockHeader header = *block;
    int type = getType(header);
    if(type < SMALL_BLOCK_CATEGORIES){
        return smallBlockSizes[type];
    }
    return 0;
}
