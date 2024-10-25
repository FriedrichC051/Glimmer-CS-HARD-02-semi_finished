#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<arpa/inet.h>
#define SERVER_IP "127.0.0.1"

int main()
{
    // 1. create a socket for communication
    int fd=socket(AF_INET,SOCK_STREAM,0);
    if(fd==-1)
    {
        perror("socket");return -1;
    }

    //2. connect to server (with server IP and Port)
    struct sockaddr_in saddr;
    saddr.sin_family=AF_INET;
    saddr.sin_port=htons(9999);
    inet_pton(AF_INET,SERVER_IP,&saddr.sin_addr.s_addr);

    int chek=connect(fd,(struct sockaddr*)&saddr,sizeof(saddr));
    if(chek==-1)
    {
        perror("connect");
        return -1;
    }

    //3. communication
    while(1)
    {
        //send data
        char buff[2048];
        sprintf(buff,"Senpai, Ciallo～(∠·ω< )⌒★");
        send(fd,buff,strlen(buff)+1,0);

        //receive data
        memset(buff,0,sizeof(buff));
        int len=recv(fd,buff,sizeof(buff),0);
        if(len>0)
        {
            printf("server message: %s\n",buff);
        }
        else if(len==0)
        {
            printf("server disconnected\n");
            break;
        }
        else
        {
            perror("recv");
            break;
        }
        sleep(1);
    }

    //4. close the descriptor
    close(fd);
    return 0;
}