#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#define __USE_GNU
#include <dlfcn.h>
#include "ThreadInfo.h"
#include "NonblockingStack.h"
#include "MemoryPool.h"

#define MAX_THREAD

ThreadInfo *threadInfoArray;
static int threadMax;
static unsigned int threadInfoArrayUsage;
static NonBlockingStackBlock inactiveThreadInfoStack = {.block_16b = 0};
static pthread_key_t inactiveKey;

// __thread ThreadInfo *localThreadInfo;
__thread unsigned int threadID;
__thread SmallBlockThreadInfo *localSmallBlockInfo;
__thread MidBlockThreadInfo *localMidBlockInfo;
__thread uint64_t *localBitmapChunk;
__thread uint64_t localBitmapChunkUsage;

static void find_thread_create();
static void setThreadInfoInactive(void *arg);

static inline ThreadInfo *threadInfoGetNext(ThreadInfo *prevThreadInfo){
    return prevThreadInfo->next;
}

static inline void threadInfoSetNext(ThreadInfo *prevThreadInfo, ThreadInfo *nextThreadInfo){
    prevThreadInfo->next = nextThreadInfo;
}

// Initiate ThreadInfo for this thread
static void initThreadInfo(){
    // try to pop inactive ThreadInfo stack
    unsigned int tempThreadID = 0;
    ThreadInfo *targetThreadInfo = pop_nonblocking_stack(
        inactiveThreadInfoStack,
        threadInfoGetNext
    );
    if(targetThreadInfo == NULL){
        tempThreadID = __atomic_fetch_add(&threadInfoArrayUsage, 1, __ATOMIC_RELAXED);
        if(__glibc_unlikely((int)tempThreadID >= threadMax)){
            exit(EXIT_FAILURE);
        }
        targetThreadInfo = &(threadInfoArray[tempThreadID]);
        targetThreadInfo->threadID = tempThreadID;
    }
    // localThreadInfo = targetThreadInfo;
    localSmallBlockInfo = targetThreadInfo->smallBlockInfo;
    localMidBlockInfo = targetThreadInfo->midBlockInfo;
    threadID = targetThreadInfo->threadID;
    localBitmapChunk = targetThreadInfo->bitmapChunk;
    localBitmapChunkUsage = targetThreadInfo->bitmapChunkUsage;
    // use pthread_key to register thread destroyer
    pthread_key_create(&inactiveKey, setThreadInfoInactive);
    // set some meaningless value to make key effective
    pthread_setspecific(inactiveKey, (void*)0x8353);
}

// Initiate ThreadInfo Array, thread_create overriding and init ThreadInfo for current thread
__attribute__ ((constructor))
void initThreadInfoArray(){
    {
        int size = 16;
        for(int i = 0; i < SMALL_BLOCK_CATEGORIES; i++){
            smallBlockSizes[i] = size;
            if(i % 2 == 0){
                size += size/2;
            }else{
                size += size/3;
            }
        }
    }
    char buffer[128] = {0};
    int threadsMaxFile = open("/proc/sys/kernel/threads-max", O_RDONLY);
    threadMax = 0;
    if(threadsMaxFile != -1){
        if(read(threadsMaxFile, buffer, 128) != -1){
            // open and read successfully
            threadMax = atoi(buffer);
        }
        close(threadsMaxFile);
    }
    threadInfoArray = chunkRequestUnaligned(sizeof(ThreadInfo) * threadMax);
    initThreadInfo();
    find_thread_create();
}

// Override pthread_create to make sure initThreadInfo() is executed for every thread created
typedef int (*pthread_create_fpt)(pthread_t *, const pthread_attr_t *, void *(*)(void*), void *);
static pthread_create_fpt thread_create = NULL;
static void find_thread_create(){
    char *error;
    dlerror();  // Clear any existing error

    thread_create = (pthread_create_fpt)dlsym(RTLD_NEXT, "pthread_create");
    if ((error = dlerror()) != NULL) {
        fprintf(stderr, "%s\n", error);
        exit(EXIT_FAILURE);
    }
}
typedef struct Task{
    void *(*routine)(void *);
    void *arg;
}Task;

void *wrapped_task(void *task){
    initThreadInfo();
    Task* task_content = task;
    void *(*routine)(void *) = task_content->routine;
    void *arg = task_content->arg;
    hxfree(task);
    return routine(arg);
}

int HX_pthread_create(pthread_t *thread, const pthread_attr_t *attr,
                   void *(*start_routine)(void *), void *arg){
    Task *task = hxmalloc(sizeof(Task));
    task->routine = start_routine;
    task->arg = arg;
    
    return thread_create(thread, attr, wrapped_task, task);
}

__attribute__((visibility("default")))
int pthread_create(pthread_t *thread, const pthread_attr_t *attr,
                   void *(*start_routine)(void *), void *arg)
                   __attribute__((weak, alias("HX_pthread_create")));

// set the current ThreadInfo inactive after exiting
static void setThreadInfoInactive(void *arg){
    push_nonblocking_stack(
        &(threadInfoArray[threadID]), 
        inactiveThreadInfoStack,
        threadInfoSetNext
    );
}