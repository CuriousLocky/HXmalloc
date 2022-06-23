#include "HXmalloc.h"

__attribute__((visibility("default")))
void *malloc(size_t size) __attribute((week, alias("hxmalloc")));

__attribute__((visibility("default")))
void *hxmalloc(size_t size){
    if(size == 0){return NULL;}
    
}

__attribute__((visibility("default")))
void free(size_t size) __attribute((week, alias("hxfree")));

__attribute__((visibility("default")))
void hxfree(void *ptr){
    if(ptr == NULL){return NULL;}
    
}