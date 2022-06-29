#pragma once
#include <stddef.h>

/*The size of each memory chunk in bytes, the basic unit to request
memory from the OS*/
#define CHUNK_SIZE (64*1024*1024)

/*Request a chunk of memory*/
void *chunkRequest();

/*Release a memory chunk*/
void chunkRelease(void *chunk);

/*Create a block by mapping*/
void *createMapBlock(size_t size);

/*Unmap a mapped block*/
void unmapBlock(void *mappedBlock, size_t size);
