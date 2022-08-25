#pragma once

#include <stddef.h>
#include <stdint.h>
#include <x86intrin.h>
#include "MemoryPool.h"

typedef uint64_t BlockHeader;

#define SUPERBLOCKADDRMASK  0x0000ffffffffffffUL
#define INDEXMASK           0x003f000000000000UL
#define TYPEMASK            0xffc0000000000000UL

static inline uint64_t *getChunkEnd(uint64_t *chunkStart){
    return (uint64_t*)(((uintptr_t)chunkStart) + CHUNK_SIZE);
}

/*Align "size" to "alignment", alignment should be 2^n.*/
static inline size_t align(size_t size, size_t alignment){
    return (((size)+(alignment-1)) & (~(alignment-1)));
}

void *hxmalloc(size_t size);
void hxfree(void *ptr);
void *hxrealloc(void *ptr, size_t newSize);
size_t hxmallocUsableSize(void *ptr);
