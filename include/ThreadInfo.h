#include <stddef.h>
#include <stdint.h>
#include "NonblockingStack.h"
#include "smallBlock.h"

typedef struct{
    unsigned int threadID;
    SmallBlockThreadInfo smallBlockInfo;

}ThreadInfo;

extern ThreadInfo *threadInfoArray;

__thread ThreadInfo *localThreadInfo;
