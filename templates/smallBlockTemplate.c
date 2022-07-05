#include "smallBlock.h"
{{smallBlockHeaders}}

void initSmallBlock(){
{{initBlock}}
}
    
BlockHeader *findSmallVictim(uint64_t size){
    size = size / 16;
    BlockHeader *(*findVictimFunctions[{{blockSizeNumber}}])() = {
{{findVictimFunctions}}
    };
    return (*findVictimFunctions[size])();
}
    
void freeSmallBlock(BlockHeader *block, BlockHeader header){
    size_t size = getSize(header)/16;
    void (*freeBlockFunctions[{{blockSizeNumber}}])(BlockHeader *, BlockHeader) = {
{{freeBlockFunctions}}
    };
    (*freeBlockFunctions[size])(block, header);
}