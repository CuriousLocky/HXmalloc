#include "smallBlock.h"
{{smallBlockHeaders}}

void initSmallBlock(){
{{initBlock}}
}
    
BlockHeader *findSmallVictim(uint64_t size){
    int funcIndex = size / 16 - 1;
    BlockHeader *(*findVictimFunctions[{{blockSizeNumber}}])() = {
{{findVictimFunctions}}
    };
    return (*findVictimFunctions[funcIndex])();
}
    
void freeSmallBlock(BlockHeader *block, BlockHeader header){
    size_t size = getSize(header);
    int funcIndex = size / 16 - 1;
    void (*freeBlockFunctions[{{blockSizeNumber}}])(BlockHeader *, BlockHeader) = {
{{freeBlockFunctions}}
    };
    (*freeBlockFunctions[funcIndex])(block, header);
}

uint64_t getSmallBlockThreadID(uint64_t *superBlockBitmap){
    uint64_t *bitmapPage = (uint64_t*)(((uint64_t)superBlockBitmap) & (~4095UL));
    return bitmapPage[510]; // second to the last
}
