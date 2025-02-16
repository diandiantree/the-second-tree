#include "EchoServer.h"

int main(int argc,char *argv[])
{
    if (argc != 3) 
    { 
        printf("usage: ./tcp ip port\n"); 
        printf("example: ./tcp 192.168.242.128 5085\n\n"); 
        return -1; 
    }
    EchoServer es(argv[1],atoi(argv[2]),2,2);       // 创建服务器对象,并指定ip和端口,atoi将字符数组转换为整数，返回整数
    es.Start();
    return 0;
}