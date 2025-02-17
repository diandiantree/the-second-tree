// 网络通讯的客户端程序。
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <ctime>

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        printf("usage:./client ip port\n"); 
        printf("example:./client 192.168.150.128 5085\n\n"); 
        return -1;
    }

    int sockfd;
    struct sockaddr_in servaddr;
    char buf[1024];
 
    if ((sockfd=socket(AF_INET,SOCK_STREAM,0))<0) { printf("socket() failed.\n"); return -1; }
    
    memset(&servaddr,0,sizeof(servaddr));
    servaddr.sin_family=AF_INET;
    servaddr.sin_port=htons(atoi(argv[2]));
    servaddr.sin_addr.s_addr=inet_addr(argv[1]);

    if (connect(sockfd, (struct sockaddr *)&servaddr,sizeof(servaddr)) != 0)
    {
        printf("connect(%s:%s) failed.\n",argv[1],argv[2]); close(sockfd);  return -1;
    }
    printf("开始时间: %ld\n",time(0));
    printf("connect ok.\n");
    for (int ii=1;ii<3;ii++)
    {
        memset(buf,0,sizeof(buf));
        sprintf(buf,"这是第%d只🐎😄😄。",ii);

        char tmpbuf[1024];                 // 临时的buffer，报文头部+报文内容。
        memset(tmpbuf,0,sizeof(tmpbuf));
        int len=strlen(buf);                 // 计算报文的大小。
        memcpy(tmpbuf,&len,4);       // 拼接报文头部。
        memcpy(tmpbuf+4,buf,len);  // 拼接报文内容，这里用tmpbuf会覆盖掉原有的tmpbuf，覆盖掉就没有报文长度了，就会解析错误，发不出去

        send(sockfd,tmpbuf,len+4,0);  // 把请求报文发送给服务端。

        recv(sockfd,&len,4,0);            // 先读取4字节的报文头部。

        memset(buf,0,sizeof(buf));
        recv(sockfd,buf,len,0);           // 读取报文内容。
       // printf("recv:%s\n",buf);
    }
    printf("结束时间: %ld\n",time(0));
    return 0;
} 
