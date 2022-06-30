#include "HXmalloc.h"
#include "block16.h"

__attribute__((visibility("default")))
void *malloc(size_t size) __attribute((week, alias("hxmalloc")));

__attribute__((visibility("default")))
void *hxmalloc(size_t size){
    if(size == 0){return NULL;}
    if(size < 48){
        // tiny block
        // TODO: implement tiny block handler functions
        size = (size >> 4) + ((size & 15)!=0);
        switch (size){
            case 1:
                return findVictim16();
            default:
                break;
        }
        return NULL;
    }
    if(size >= 4080){
        // big block
        // TODO: implement big block handler functions
        return NULL;
    }
    // TODO: implement small block handler functions
    size = (size >> 6) + ((size & 63)!=0);
    switch(size){
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
        case 6:
        case 7:
        case 8:
        case 9:
        case 10:
        case 11:
        case 12:
        case 13:
        case 14:
        case 15:
        case 16:
        case 17:
        case 18:
        case 19:
        case 20:
        case 21:
        case 22:
        case 23:
        case 24:
        case 25:
        case 26:
        case 27:
        case 28:
        case 29:
        case 30:
        case 31:
        case 32:
        case 33:
        case 34:
        case 35:
        case 36:
        case 37:
        case 38:
        case 39:
        case 40:
        case 41:
        case 42:
        case 43:
        case 44:
        case 45:
        case 46:
        case 47:
        case 48:
        case 49:
        case 50:
        case 51:
        case 52:
        case 53:
        case 54:
        case 55:
        case 56:
        case 57:
        case 58:
        case 59:
        case 60:
        case 61:
        case 62:
        case 63:
        default:
            break;
    }
    return NULL;
}

__attribute__((visibility("default")))
void free(size_t size) __attribute((week, alias("hxfree")));

__attribute__((visibility("default")))
void hxfree(void *ptr){
    if(ptr == NULL){return NULL;}
    if((uint64_t)ptr % 4096 == 0){
        // big block
        // TODO: implement deallocation
    }
    
}