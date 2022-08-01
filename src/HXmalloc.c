#include "HXmalloc.h"
#include "SmallBlockCommon.h"
#include "MidBlockCommon.h"
#include "BlockCategory.h"

__attribute__((visibility("default")))
void *malloc(size_t size) __attribute((weak, alias("hxmalloc")));

__attribute__((visibility("default")))
void *hxmalloc(size_t size){
    if(size == 0){return NULL;}
    if(size + 8 <= MAX_SMALL_BLOCK_SIZE){
        // small block
        // align to 16
        size += 8;
        size = align(size, 16);
        return findSmallVictim(size) + 1;
    }
    size += 16;
    if(size >= MAX_MID_BLOCK_SIZE){
        // big block
        // TODO: implement big block handler functions
        return NULL;
    }
    size = align(size, 4096);
    return findMidVictim(size) + 2;
}

__attribute__((visibility("default")))
void free(void *ptr) __attribute((weak, alias("hxfree")));

__attribute__((visibility("default")))
void hxfree(void *ptr){
    if(ptr == NULL){return;}
    BlockHeader *block = getBlockHeader(ptr);
    BlockHeader header = *block;
    uint64_t size = hxmallocUsableSize(ptr);
    if(size <= MAX_SMALL_BLOCK_SIZE){
        // small block
        freeSmallBlock(block, header);
    }else if(size >= MAX_MID_BLOCK_SIZE){
        // big block
    }else{
        // mid block
        freeMidBlock(block-1, header);
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
    type -= SMALL_BLOCK_CATEGORIES;
    if(type < MID_BLOCK_CATEGORIES){
        return (type + 1) * 4096;
    }
    return 0;
}
