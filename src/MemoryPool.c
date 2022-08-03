#include "MemoryPool.h"
#define _GNU_SOURCE
#include <sys/mman.h>

void *chunkRequest(size_t chunkSize){
    void *result = mmap(
        NULL, chunkSize, 
        PROT_READ | PROT_WRITE,
        MAP_PRIVATE | MAP_ANONYMOUS,
         -1, 0
    );
    if(result == MAP_FAILED){
        return NULL;
    }
    madvise(result, chunkSize, MADV_SEQUENTIAL);
    return result;
}

void chunkRelease(void *chunk, size_t chunkSize){
    munmap(chunk, chunkSize);
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