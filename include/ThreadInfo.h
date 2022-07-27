#pragma once
#include <stddef.h>
#include <stdint.h>
#include "NonblockingStack.h"
#include "SmallBlockCommon.h"

typedef struct{
    unsigned int threadID;
    SmallBlockThreadInfo smallBlockInfo;
}ThreadInfo;

extern ThreadInfo *threadInfoArray;

extern __thread ThreadInfo *localThreadInfo;
