#include "ThreadInfo.h"
#include "NonblockingStack.h"

#define MAX_THREAD

ThreadInfo *threadInfoArray;
static ThreadInfo _threadInfoArray[4096];
static unsigned int threadInfoArrayUsage;
static NonBlockingStackBlock inactiveThreadInfoStack = {.block_16b = 0};

__thread ThreadInfo *localThreadInfo;

__attribute__ ((constructor))
void initThreadInfo(){
    threadInfoArray = _threadInfoArray;
    unsigned int threadID = __atomic_fetch_add(&threadInfoArrayUsage, 1, __ATOMIC_RELAXED);
    localThreadInfo = &(threadInfoArray[threadID]);
    localThreadInfo->threadID = threadID;
    initSmallBlock();
}