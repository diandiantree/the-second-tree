#pragma once

#include <functional>
#include "Socket.h"
#include "InetAddress.h"
#include "Channel.h"
#include "EventLoop.h"
#include <memory>

class Acceptor
{
private:
    EventLoop* loop_;                            //Acceptor对应的事件循环，但是这个事件循环是从外面传入的，不属于这个类的对象
    std::unique_ptr<Socket> listen_socket;
    std::unique_ptr<Channel> serverchannel;
    std::function<void(int)> newconnection_;      //处理新客户端连接请求的回调函数,将指向Tcpserver::newconnection()
public:
    Acceptor(EventLoop& loop,const string& ip,uint16_t port);
    ~Acceptor();
    void handle_newconnection();
    void setnewconnection(std::function<void(int)>);
};