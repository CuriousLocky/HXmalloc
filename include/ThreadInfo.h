#pragma once
#include <stddef.h>
#include <stdint.h>
#include "NonblockingStack.h"
#include "SmallBlockCommon.h"

typedef struct _ThreadInfo{
    unsigned int threadID;
    SmallBlockThreadInfo smallBlockInfo;
    struct _ThreadInfo *next;
}ThreadInfo;

extern ThreadInfo *threadInfoArray;

extern __thread ThreadInfo *localThreadInfo;
