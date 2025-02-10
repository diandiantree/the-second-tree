#pragma once

#include "Socket.h"
#include "Epoll.h"
#include "Channel.h"
#include "EventLoop.h"
#include "Acceptor.h"
#include "Connection.h"
#include <functional>
#include <map>
#include <memory>

class Tcpserver
{
private:
    EventLoop eloop;     //一个Tcpserver类可以有多个事件循环，现在是单线程，暂时用一个事件循环
    std::unique_ptr<Acceptor> acceptor_; //一个Tcpserver类只有一个acceptor对象
    std::map<int,std::unique_ptr<Connection>> connections;
    std::function<void(int)> newconnection_;
    std::function<void(Connection*)> closeconnection_;
    std::function<void(Connection*)> errorconnection_;
    std::function<void(Connection*,std::string&)> onmessage_;
    std::function<void(Connection*)> sendcompletion_;
    std::function<void(EventLoop*)> epolltimeout_;
public:
    Tcpserver(const string& ip,uint16_t port);
    ~Tcpserver();
    void start();
    void newconnection(int clientfd);
    void closeconnection(Connection* connection);
    void errorconnection(Connection* connection);
    void onmessage(Connection* conn,std::string message); //处理客户端的请求报文，在Connection类中回调此函数
    void sendcompletion(Connection* conn);               //发送完数据后的回调函数
    void epolltimeout(EventLoop* loop);                  //epoll_wait()超时的回调函数
    void setnewconnection(std::function<void(int)> fn);
    void setcloseconnection(std::function<void(Connection*)> fn);
    void seterrorconnection(std::function<void(Connection*)> fn);
    void setmessagecallback(std::function<void(Connection*,std::string&)> fn);
    void setsendcompletioncallback(std::function<void(Connection*)> fn);
    void setepolltimeout(std::function<void(EventLoop*)> fn);
};