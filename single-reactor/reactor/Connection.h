#pragma once

#include <functional>
#include "Socket.h"
#include "InetAddress.h"
#include "Channel.h"
#include "EventLoop.h"
#include "Buffer.h"

class Connection
{
private:
    EventLoop* loop_;   //Acceptor对应的事件循环，但是这个事件循环是从外面传入的，不属于这个类的对象
    Socket* client_socket;
    Channel* clientchannel;
    Buffer inputbuffer_; //接收缓冲区
    Buffer outputbuffer_;//发送缓冲区
    std::function<void(Connection*)> closecallback_;                //连接断开，关闭fd_的回调函数，回调Connection::closecallback_()
    std::function<void(Connection*)> errorcallback_;                //fd_发生错误的回调函数，回调Connection::errorcallback_()
    std::function<void(Connection*,std::string&)> messagecallback_;  //fd_读事件的回调函数，回调Connection::onmessage()
    std::function<void(Connection*)> sendcompletioncallback_;         //发送完数据后的回调函数
public:
    Connection(EventLoop* loop,int clientfd);
    ~Connection();
    int fd()const;        //返回socket的fd
    string ip()const;
    uint16_t port();
    void closecallback();          //TCP连接关闭(断开)的回调函数,供Channel回调
    void errorcallback();          //TCP连接错误的回调函数,供Channel回调
    void setclosecallback(function<void(Connection*)> fn);   
    void seterrorcallback(function<void(Connection*)> fn);
    void setmessagecallback(function<void(Connection*,std::string&)> fn);  
    void setsendcompletioncallback(function<void(Connection*)> fn);
    void setwritecallback(); 
    void onmessage();              //处理对端发送过来的消息
    void sendmessage(const char* data,size_t size);           //发送消息给对端
};