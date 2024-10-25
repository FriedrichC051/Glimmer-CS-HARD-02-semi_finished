#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<arpa/inet.h>
#include<pthread.h>
#include "threadpool.h"

struct SockInFo
{
    struct sockaddr_in addr;
    int fd;
};

typedef struct PoolInFo
{
    ThreadPool* p;
    int fd;
}PoolInFo;

// Receive client connection
void acceptConn(void* arg);

//A working function used for communication
void working(void* arg);

int main()
{
    //create a socket for listening
    int fd=socket(AF_INET,SOCK_STREAM,0);
    if(fd==-1)
    {
        perror("socket");return -1;
    }

    // initialize sockaddr
    struct sockaddr_in saddr;
    saddr.sin_family=AF_INET;
    saddr.sin_port=htons(9999);
    saddr.sin_addr.s_addr=INADDR_ANY;
    
    //bind the local IP & Port
    int chek=bind(fd,(struct sockaddr*)&saddr,sizeof(saddr));
    if(chek==-1)
    {
        perror("bind");
        return -1;
    }

    //set listening
    chek=listen(fd,128);
    if(chek==-1)
    {
        perror("listen");
        return -1;
    }

    //Create thread pool
    ThreadPool* pool=CreateThreadPool(3,8,100);
    PoolInFo* info=(PoolInFo*)malloc(sizeof(PoolInFo));
    info->p=pool;
    info->fd=fd;
    threadPoolAdd(pool,acceptConn,info);

    pthread_exit(NULL);

    /* Thread destruction operation is not set for now */

    return 0;
}

void acceptConn(void* arg)
{   
    PoolInFo* pool_info=(PoolInFo*)arg;

    //block and wait for the client link
    int addrlen=sizeof(struct sockaddr_in);
    while(1)
    {
        struct SockInFo* pinfo;
        
        /*
            Instead of pointing to a SockInFo array, a block of heap memory 
            is allocated to pinfo. 
            And the heap memory is automatically freed in the thread pool
            after the task function [void working(void* arg)] has finished
            executing.
        */

        pinfo=(struct SockInFo*)malloc(sizeof(struct SockInFo));
        pinfo->fd=accept(pool_info->fd,(struct sockaddr*)&pinfo->addr,&addrlen);
        if(pinfo->fd==-1)
        {
            perror("accept");
            break;
        }
        // Add communication task
        threadPoolAdd(pool_info->p,working,pinfo);
    }
    close(pool_info->fd);
}

void working(void* arg)
{
    struct SockInFo* pinfo=(struct SockInFo*)arg;
    struct sockaddr_in caddr=pinfo->addr;
    int cfd=pinfo->fd;

    //connection established, print IP and Port information
    char ip[32];
    printf("client IP: %s, Port: %d\n",
    inet_ntop(AF_INET,&caddr.sin_addr.s_addr,ip,sizeof(ip)),
    ntohs(caddr.sin_port));

    //communication
    while(1)
    {
        //receive data
        char buff[2048];
        int len=recv(cfd,buff,sizeof(buff),0);
        if(len>0)
        {
            printf("client message: %s\n",buff);
            send(cfd,buff,len,0);
        }
        else if(len==0)
        {
            printf("client disconnected\n");
            break;
        }
        else
        {
            perror("recv");
            break;
        }
    }

    //close the descriptor
    close(pinfo->fd);
}