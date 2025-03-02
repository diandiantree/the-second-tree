#pragma once

#include <arpa/inet.h>
#include <netinet/in.h>
#include <string>
using namespace std;

//socket的地址协议类
class InetAddress{
private:
    sockaddr_in addr_;                                  //表示地址协议的结构体
public:
    InetAddress()=default;
    InetAddress(const string& ip,uint16_t port);        //listenfd用这个构造函数
    InetAddress(const sockaddr_in addr);                //客户端连上来的fd用这个构造函数
    ~InetAddress();  
    const char* ip()const;                              //返回字符串表示的地址,如:192.168.242.128
    uint16_t port()const;                               //返回整数表示的端口，如:5008
    const sockaddr *addr()const;                        //返回addr_成员的地址，并强转为sockaddr类型
};