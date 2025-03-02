#include "EchoServer.h"
#include <signal.h>
#include <memory>

std::unique_ptr<EchoServer> es;       // 创建服务器对象,并指定ip和端口,atoi将字符数组转换为整数，返回整数
void Stop_tcp(int sig)
{
 std::cout<<"sig="<<sig<<std::endl;
 std::cout<<"服务端已停止"<<std::endl;
 es->Stop();
 exit(0);
}
int main(int argc,char *argv[])
{
    if (argc != 3) 
    {  
        printf("please input: ./tcp 192.168.242.128 5085\n"); 
        return -1; 
    }
    signal(SIGTERM,Stop_tcp);//信号15
    signal(SIGINT,Stop_tcp);//信号2
    es=std::make_unique<EchoServer>(argv[1],atoi(argv[2]),3,0);
    es->Start();
    return 0;
}