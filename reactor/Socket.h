#pragma once

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <string>
#include <errno.h>
#include <unistd.h>
#include <iostream>
#include <functional>
#include "InetAddress.h"

int createnonblocking();  //创建非阻塞的socket
class Socket
{
private:
    const int fd_;
    string ip_;      //如果是listenfd,存放服务端监听的ip，如果是客户端连接的ip，存放对端的ip
    uint16_t port_;  //同上
public:
    Socket(int fd);       //构造函数，创建socket
    ~Socket();            //析构函数，关闭socket
    int fd()const;        //返回socket的fd
    string ip()const;
    uint16_t port()const;
    void set_REUSEADDR(bool on);    /*设置SO_REUSEADDR选项,true-打开,false-关闭 
                                    SO_REUSEADDR允许在绑定操作中重用本地地址。
                                    当一个套接字关闭后，它所绑定的地址并不会立即释放，而是会进入一个 “TIME_WAIT” 状态，持续一段时间（通常是 2 倍的 MSL，即最长报文段寿命）。
                                    在这个状态下，其他进程无法立即绑定到相同的地址。通过设置SO_REUSEADDR为 1，程序可以在套接字关闭后立即重用该地址，而无需等待 “TIME_WAIT” 状态结束。
                                    这在开发服务器程序时非常有用，特别是在调试过程中，你可能需要频繁重启服务器。*/
    void set_NODELAY(bool on);    //禁用Nagle算法
    void set_REUSEPORT(bool on); 
    void set_KEEPALIVE(bool on); 
    void bind(const InetAddress& serveraddr);
    void listen(int n=128);
    int accept();
};