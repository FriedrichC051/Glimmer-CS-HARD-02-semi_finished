#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<arpa/inet.h>

int main()
{
    // 1. create a socket for listening
    int fd=socket(AF_INET,SOCK_STREAM,0);
    if(fd==-1)
    {
        perror("socket");return -1;
    }

    //2. bind the local IP & Port
    
    // initialize sockaddr
    struct sockaddr_in saddr;
    saddr.sin_family=AF_INET;
    saddr.sin_port=htons(9999);
    saddr.sin_addr.s_addr=INADDR_ANY;
    
    int chek=bind(fd,(struct sockaddr*)&saddr,sizeof(saddr));
    if(chek==-1)
    {
        perror("bind");
        return -1;
    }

    //3. set listening
    chek=listen(fd,128);
    if(chek==-1)
    {
        perror("listen");
        return -1;
    }

    //4. block and wait for the client link
    struct sockaddr_in caddr;
    int addrlen=sizeof(caddr);
    int cfd=accept(fd,(struct sockaddr*)&caddr,&addrlen);
    if(cfd==-1)
    {
        perror("accept");
        return -1;
    }

    //5. connection established, print IP and Port information
    char ip[32];
    printf("client IP: %s, Port: %d\n",
    inet_ntop(AF_INET,&caddr.sin_addr.s_addr,ip,sizeof(ip)),
    ntohs(caddr.sin_port));

    //5. communication
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

    //6. close the descriptor
    close(fd);
    close(cfd);
    return 0;
}