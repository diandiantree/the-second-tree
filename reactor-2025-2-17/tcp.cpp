#include "EchoServer.h"
#include <signal.h>
#include <memory>

std::unique_ptr<EchoServer> es;       // 创建服务器对象,并指定ip和端口,atoi将字符数组转换为整数，返回整数
void Stop(int sig)
{
 std::cout<<"sig="<<sig<<std::endl;
 //调用EchoServer::Stop()停止服务
 std::cout<<"服务端已停止"<<std::endl;
 es->Stop();
 exit(0);
}
int main(int argc,char *argv[])
{
    if (argc != 3) 
    { 
        printf("usage: ./tcp ip port\n"); 
        printf("example: ./tcp 192.168.242.128 5085\n\n"); 
        return -1; 
    }
    signal(SIGTERM,Stop);//信号15
    signal(SIGINT,Stop);//信号2
    es=std::make_unique<EchoServer>(argv[1],atoi(argv[2]),2,0);
    es->Start();
    return 0;
}