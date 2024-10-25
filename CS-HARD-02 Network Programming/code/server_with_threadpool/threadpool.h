#ifndef _THREADPOOL_H
#define _THREADPOOL_H

typedef struct ThreadPool ThreadPool;

// Create thread pool and initialize
ThreadPool *CreateThreadPool(int min,int max,int qs);

// Destroy thread pool
int threadPoolDestroy(ThreadPool* pool);

// Add tasks to thread pool
void threadPoolAdd(ThreadPool* pool,void(*func)(void*),void* arg);

// Get the number of working threads
int threadPoolBusyNum(ThreadPool* pool);

// Get the number of threads alive
int threadPoolLiveNum(ThreadPool* pool);

// Worker thread function 
void* worker(void* arg);

// Manager thread function
void* manager(void* arg);

//Thread exit function
void threadExit(ThreadPool* pool);

#endif