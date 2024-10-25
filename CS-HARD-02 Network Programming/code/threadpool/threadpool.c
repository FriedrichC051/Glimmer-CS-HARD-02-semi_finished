#include "threadpool.h"
#include<pthread.h>
#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<unistd.h>
#define OPER_THREAD_NUM 2

//Define task struct
typedef struct Task
{
    void (*func)(void* arg);
    void* arg;
}Task;

//Define threadpool struct
struct ThreadPool
{
    //Task queue, defined as a pointer to array
    Task* task_q;
    int qCapacity;
    int qSize; //Number of tasks
    int qFront; //get data
    int qRear; //put data

    pthread_t managerID;
    pthread_t *threadIDs; //worker thread id
    int minNum; //Minimum number of threads
    int maxNum; //Maximum number of threads
    int busyNum; //Number of working threads
    int liveNum; //Number of threads alive
    int exitNum; //Number of threads to kill

    pthread_mutex_t mutexPool;
    pthread_mutex_t mutexBusy;

    pthread_cond_t notFull;
    pthread_cond_t notEmpty;

    int shutdown; 
    /*
      Whether to destroy the thread pool
      if 1 then destroy, if 0 then do not destroy
    */
};

ThreadPool *CreateThreadPool(int min,int max,int qs)
{
    // Create threadpool
    ThreadPool* pool=(ThreadPool*)malloc(sizeof(ThreadPool));
    do{
        if(pool==NULL)
        {
            printf("malloc threadpool failed\n");
            break;
        }

        //Allocate threadIDs memory and use memset to clear
        pool->threadIDs=(pthread_t*)malloc(sizeof(pthread_t)*max); //threadIDs is pointer(array)
        if(pool->threadIDs==NULL)
        {
            printf("malloc threadIDs failed\n");
            break;
        }
        memset(pool->threadIDs,0,sizeof(sizeof(pthread_t)*max));

        // Initialize thread-related parameters
        pool->minNum=min;
        pool->maxNum=max;
        pool->busyNum=0;
        pool->liveNum=min; //The initial number is the same as minNum
        pool->exitNum=0;
        
        // Initialize mutex and condition
        if(pthread_mutex_init(&pool->mutexPool,NULL)||
        pthread_mutex_init(&pool->mutexBusy,NULL)||
        pthread_cond_init(&pool->notFull,NULL)||
        pthread_cond_init(&pool->notEmpty,NULL))
        {
            printf("mutex or condition initialization failed\n");
            break;
        }

        // Initialize task queue
        pool->task_q=(Task*)malloc(sizeof(Task)*qs);
        pool->qCapacity=qs;
        pool->qSize=0;
        pool->qFront=pool->qRear=0;

        // Initialize the terminate parameter
        pool->shutdown=0;

        // Create threads
        pthread_create(&pool->managerID,NULL,manager,pool);
        for(int i=0;i<min;++i)
        {
            pthread_create(&pool->threadIDs[i],NULL,worker,pool);
        }

        return pool;
        
    }while(0);

    //Release resources
    if(pool&&pool->threadIDs)free(pool->threadIDs);
    if(pool&&pool->task_q)free(pool->task_q);
    if(pool)free(pool);
    return NULL;
}

int threadPoolDestroy(ThreadPool* pool)
{
    if(pool==NULL)return -1;

    // Close thread pool
    pool->shutdown=1;

    // Block exit manager thread
    pthread_join(pool->managerID,NULL);

    // Wake up the blocking consumer thread
    for(int i=0;i<pool->liveNum;++i)
    {
        pthread_cond_signal(&pool->notEmpty);
        // Consumer thread will exit in worker function
    }
    // Free heap memory
    if(pool->task_q)
    {
        free(pool->task_q);
    }
    if(pool->threadIDs)
    {
        free(pool->threadIDs);
    }

    pthread_mutex_destroy(&pool->mutexPool);
    pthread_mutex_destroy(&pool->mutexBusy);
    pthread_cond_destroy(&pool->notEmpty);
    pthread_cond_destroy(&pool->notFull);
    free(pool);
    pool=NULL;
}


void threadPoolAdd(ThreadPool* pool,void(*func)(void*),void* arg)
{
    pthread_mutex_lock(&pool->mutexPool);
    while(pool->qSize==pool->qCapacity&&!pool->shutdown)
    {
        // Block producer thread
        pthread_cond_wait(&pool->notFull,&pool->mutexPool);

    }
    if(pool->shutdown)
    {
        pthread_mutex_unlock(&pool->mutexPool);
        return;
    }
    // Add tasks
    pool->task_q[pool->qRear].func=func;
    pool->task_q[pool->qRear].arg=arg;
    pool->qRear=(pool->qRear+1)%pool->qCapacity;
    pool->qSize++;

    // Wake up the worker thread that is blocked because of no tasks
    pthread_cond_signal(&pool->notEmpty);

    pthread_mutex_unlock(&pool->mutexPool);
}

int threadPoolBusyNum(ThreadPool* pool)
{
    pthread_mutex_lock(&pool->mutexBusy);
    int busyNum=pool->busyNum;
    pthread_mutex_unlock(&pool->mutexBusy);
    return busyNum;
}

int threadPoolLiveNum(ThreadPool* pool)
{
    pthread_mutex_lock(&pool->mutexPool);
    int liveNum=pool->liveNum;
    pthread_mutex_unlock(&pool->mutexPool);
    return liveNum;
}

void* worker(void* arg)
{
    ThreadPool* pool=(ThreadPool*)arg;
    while(1)
    {
        pthread_mutex_lock(&pool->mutexPool);
        // If the task queue is empty
        while(pool->qSize==0&&!pool->shutdown)
        {
            // Block the worker thread
            pthread_cond_wait(&pool->notEmpty,&pool->mutexPool);

            // Determine whether the thread needs to be destroyed
            if(pool->exitNum>0)
            {
                pool->exitNum--;
                if(pool->liveNum>pool->minNum)
                {
                    pool->liveNum--;
                    pthread_mutex_unlock(&pool->mutexPool);
                    // Remember to unlock the mutex before exit thread to avoid dead lock 
                    threadExit(pool);
                }
            }
        }
        //Check if the thread pool is closed
        if(pool->shutdown)
        {
            pthread_mutex_unlock(&pool->mutexPool); // Avoid dead lock
            threadExit(pool);
        }
        // Get a task from queue
        Task task;
        task.func=pool->task_q[pool->qFront].func;
        task.arg=pool->task_q[pool->qFront].arg;

        // Move the head node, maintain a ring queue
        pool->qFront=(pool->qFront+1)%pool->qCapacity;
        pool->qSize--;
        // The current task is popped from queue, so the producer thread can wake up 
        pthread_cond_signal(&pool->notFull);

        //Unlock pool mutex
        pthread_mutex_unlock(&pool->mutexPool);


        printf("thread %ld start working...\n",pthread_self());
        pthread_mutex_lock(&pool->mutexBusy);
        pool->busyNum++;
        pthread_mutex_unlock(&pool->mutexBusy);

        // Comsumer threads start comsuming the task
        task.func(task.arg);
        free(task.arg);
        task.arg=NULL;

        printf("thread %ld end working...\n",pthread_self());
        pthread_mutex_lock(&pool->mutexBusy);
        pool->busyNum--;
        pthread_mutex_unlock(&pool->mutexBusy);        
    }
    return NULL;
}

void* manager(void* arg)
{
    ThreadPool* pool=(ThreadPool*)arg;
    while(!pool->shutdown)
    {
        // Check every 3 sec
        sleep(3);

        //Get the number of tasks and threads in thread pool
        pthread_mutex_lock(&pool->mutexPool);
        int qSize=pool->qSize;
        int liveNum=pool->liveNum;
        pthread_mutex_unlock(&pool->mutexPool);

        // Get the number of busy threads
        pthread_mutex_lock(&pool->mutexBusy);
        int busyNum=pool->busyNum;
        pthread_mutex_unlock(&pool->mutexBusy);

        // Add threads
        if(qSize>liveNum-busyNum&&liveNum<pool->maxNum)
        {
            pthread_mutex_lock(&pool->mutexPool);
            int cnt=0;
            for(int i=0;i<pool->maxNum&&cnt<OPER_THREAD_NUM&&
            pool->liveNum<pool->maxNum;++i)
            {
                // Find an available thread id
                if(pool->threadIDs[i]==0)
                {
                    pthread_create(&pool->threadIDs[i],NULL,worker,pool);
                    cnt++;
                    pool->liveNum++;                    
                }
            }
            pthread_mutex_unlock(&pool->mutexPool);
        }
        
        //Destroy threads
        if(busyNum*2<liveNum&&liveNum>pool->minNum)
        {
            pthread_mutex_lock(&pool->mutexPool);
            pool->exitNum=OPER_THREAD_NUM;
            pthread_mutex_unlock(&pool->mutexPool);
            // Let the worker thread actively exit (kill itself)
            for(int i=0;i<OPER_THREAD_NUM;++i)
            {
                pthread_cond_signal(&pool->notEmpty);
            }
        }
    }
    return NULL;
}

void threadExit(ThreadPool* pool)
{
    pthread_t tid=pthread_self();
    for(int i=0;i<pool->maxNum;++i)
    {
        if(pool->threadIDs[i]==tid)
        {
            pool->threadIDs[i]=0;
            printf("threadExit() called, %ld exiting...\n",tid);
            break;
        }
    }
    pthread_exit(NULL);
}