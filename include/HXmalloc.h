#pragma once

#include <stddef.h>
#include <stdint.h>

typedef uint64_t BlockHeader;

#define SUPERBLOCKADDRMASK  0x0000ffffffffffffUL
#define INDEXMASK           0x003f000000000000UL
#define TYPEMASK            0xffc0000000000000UL

static inline BlockHeader *getBlockHeader(void *ptr){
    uint64_t *ptr64 = (uint64_t*)ptr;
    return (BlockHeader*)(ptr64-1);
}

static inline int getType(uint64_t header){
    return (int)((header & TYPEMASK) >> (48 + 6));
}

static inline uint64_t *getSuperBlockBitmap(uint64_t header){
    return (uint64_t*)(header & SUPERBLOCKADDRMASK);
}

static inline int getIndex(uint64_t header){
    return (header & INDEXMASK) >> 48;
}

/*Align "size" to "alignment", alignment should be 2^n.*/
static inline size_t align(size_t size, size_t alignment){
    return (((size)+(alignment-1)) & (~(alignment-1)));
}

static inline BlockHeader packHeader(int type, int index, uint64_t *superBlockBitmap){
    uint64_t headerContent = 0;
    headerContent |= (uint64_t)type << (48 + 6);
    headerContent |= (uint64_t)index << 48;
    headerContent |= (uint64_t)superBlockBitmap;
    return (BlockHeader)headerContent;
}

void *hxmalloc(size_t size);
void hxfree(void *ptr);
void *hxrealloc(void *ptr, size_t newSize);
size_t hxmallocUsableSize(void *ptr);
