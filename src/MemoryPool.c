#include <stdint.h>
#include "MemoryPool.h"
#define _GNU_SOURCE
#include <sys/mman.h>
#include "HXmalloc.h"

#define HINT_BASE   (1024UL * 1024 * 1024 * 1024 * 2)
#define HINT_MAX    (1024UL * 1024 * 1024 * 1024 * 30)
static uintptr_t alignedBase = HINT_BASE;

void *chunkRequest(){
    uintptr_t attemptHint = __atomic_fetch_add(&alignedBase, CHUNK_SIZE, __ATOMIC_RELAXED);
    if(attemptHint > HINT_MAX){
        alignedBase = HINT_BASE;
        attemptHint = __atomic_fetch_add(&alignedBase, CHUNK_SIZE, __ATOMIC_RELAXED);
    }
    void *result = mmap(
        (void*)attemptHint, CHUNK_SIZE, 
        PROT_READ | PROT_WRITE,
        MAP_PRIVATE | MAP_ANONYMOUS,
        -1, 0
    );
    if(result == MAP_FAILED){
        return NULL;
    }
    if((uintptr_t)result % CHUNK_SIZE != 0){
        munmap(result, CHUNK_SIZE);
        // overallocate
        result = mmap(
            NULL, 2 * CHUNK_SIZE,
            PROT_READ | PROT_WRITE,
            MAP_PRIVATE | MAP_ANONYMOUS,
            -1, 0
        );
        uintptr_t alignedResult = align((uintptr_t)result, CHUNK_SIZE);
        uint64_t preSize = alignedResult - (uintptr_t)result;
        uint64_t posSize = CHUNK_SIZE - preSize;
        if(preSize > 0){
            munmap(result, preSize);
        }
        if(posSize > 0){
            munmap((void*)(alignedResult + CHUNK_SIZE), posSize);
        }
        result = (void*)alignedResult;
    }
    if(result == MAP_FAILED){
        return NULL;
    }
    madvise(result, CHUNK_SIZE, MADV_SEQUENTIAL);
    return result;
}

void *chunkRequestUnaligned(size_t size){
    void *result = mmap(
        NULL, size,
        PROT_READ | PROT_WRITE,
        MAP_PRIVATE | MAP_ANONYMOUS,
        -1, 0
    );
    if(result == MAP_FAILED){
        return NULL;
    }
    madvise(result, size, MADV_SEQUENTIAL);
    return result;
}

void chunkRelease(void *chunk){
    munmap(chunk, CHUNK_SIZE);
}

void *createMapBlock(size_t size){
    void *result = mmap(
        NULL, size, 
        PROT_READ | PROT_WRITE,
        MAP_PRIVATE | MAP_ANONYMOUS,
         -1, 0
    );
    if(result == MAP_FAILED){
        return NULL;
    }
    return result;
}

void unmapBlock(void *mappedBlock, size_t size){
    munmap(mappedBlock, size);
}