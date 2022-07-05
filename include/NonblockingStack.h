#pragma once
#include <stddef.h>
#include <stdint.h>

typedef union{
    __uint128_t block_16b;
    struct{
        void *ptr;
        uint64_t id;
    }block_struct;
}NonBlockingStackBlock;

/*a pseudo function, returns the top of a nonblocking stack, stack must be direct reference*/
#define pop_nonblocking_stack(stack, get_next)                                                              \
({                                                                                                          \
    NonBlockingStackBlock old_block;                                                                        \
    NonBlockingStackBlock new_block;                                                                        \
    do{                                                                                                     \
        old_block = stack;                                                                                  \
        if(old_block.block_struct.ptr == NULL){                                                             \
            break;                                                                                          \
        }                                                                                                   \
        new_block.block_struct.ptr = get_next(old_block.block_struct.ptr);                                  \
        new_block.block_struct.id = old_block.block_struct.id+1;                                            \
    }while(!__sync_bool_compare_and_swap(&(stack.block_16b), old_block.block_16b, new_block.block_16b));    \
    old_block.block_struct.ptr;                                                                             \
})

/*a pseudo function to push a ptr to a nonblocking stack, stack must be direct reference*/
#define push_nonblocking_stack(pushed_ptr, stack, set_next){                                                \
    NonBlockingStackBlock new_block;                                                                        \
    NonBlockingStackBlock old_block;                                                                        \
    new_block.block_struct.ptr = pushed_ptr;                                                                \
    do{                                                                                                     \
        old_block = stack;                                                                                  \
        set_next(pushed_ptr, old_block.block_struct.ptr);                                                   \
        new_block.block_struct.id = old_block.block_struct.id+1;                                            \
    }while(!__sync_bool_compare_and_swap(&(stack.block_16b), old_block.block_16b, new_block.block_16b));    \
}
