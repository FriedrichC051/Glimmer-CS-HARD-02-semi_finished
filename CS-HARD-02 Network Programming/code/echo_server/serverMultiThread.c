#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<arpa/inet.h>
#include<pthread.h>

struct SockInFo
{
    struct sockaddr_in addr;
    int fd;
};
struct SockInFo infos[512];


void* working(void* arg);

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

    //initialize SockInFo
    int max=sizeof(infos)/sizeof(infos[0]);
    for(int i=0;i<=max-1;++i)
    {
        bzero(&infos[i],sizeof(infos[i]));
        infos[i].fd=-1;
    }

    //set listening
    chek=listen(fd,128);
    if(chek==-1)
    {
        perror("listen");
        return -1;
    }

    //block and wait for the client link
    int addrlen=sizeof(struct sockaddr_in);
    while(1)//Main Thread
    {
        struct SockInFo* pinfo;
        //find an infos[i] that available
        for(int i=0;i<=max-1;++i)
        {
            if(infos[i].fd==-1)
            {
                pinfo=&infos[i];
                break;
            }
        }
        int cfd=accept(fd,(struct sockaddr*)&pinfo->addr,&addrlen);
        pinfo->fd=cfd;
        if(cfd==-1)
        {
            perror("accept");
            break;
        }
        //create Child Thread
        pthread_t tid;
        pthread_create(&tid,NULL,working,pinfo);
        pthread_detach(tid);
    }
    close(fd);
    return 0;
}

void* working(void* arg)
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
    //reclaim fd
    pinfo->fd=-1;
    return NULL;
}