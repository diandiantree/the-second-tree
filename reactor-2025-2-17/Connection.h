#pragma once

#include <functional>
#include "Socket.h"
#include "InetAddress.h"
#include "Channel.h"
#include "EventLoop.h"
#include "Buffer.h"
#include <memory>
#include <atomic>
#include "TimeStamp.h"
#include <sys/syscall.h>

class Connection;
using spConnection=std::shared_ptr<Connection>;
class Connection:public std::enable_shared_from_this<Connection>  //这里使用std::enable_shared_from_this<Connection>是为了在Connection类中能够使用shared_from_this()函数，返回一个指向当前对象的shared_ptr指针
                                                                //因为在类中的回调函数中，原本我们传递的是this指针，而this指针是一个普通指针，通过这种方式，调用shared_from_this()函数，返回一个指向当前对象的shared_ptr指针，这样就可以保证对象的生命周期
{
    using spConnection=std::shared_ptr<Connection>;
private:
    EventLoop* loop_;   //Acceptor对应的事件循环，但是这个事件循环是从外面传入的，不属于这个类的对象
    std::unique_ptr<Socket> client_socket;
    std::unique_ptr<Channel> clientchannel;
    Buffer inputbuffer_; //接收缓冲区
    Buffer outputbuffer_;//发送缓冲区
    atomic<bool> disconnect_; //连接是否断开，在IO线程中会改变这个值，在工作线程中会判断它的值，所以需要用原子操作
    std::function<void(spConnection)> closecallback_;                 //连接断开，关闭fd_的回调函数，回调Connection::closecallback_()
    std::function<void(spConnection)> errorcallback_;                 //fd_发生错误的回调函数，回调Connection::errorcallback_()
    std::function<void(spConnection,std::string&)> messagecallback_;  //fd_读事件的回调函数，回调Connection::onmessage()
    std::function<void(spConnection)> sendcompletioncallback_;        //发送完数据后的回调函数
    TimeStamp last_time;                                             //时间戳，创建connection对象时为当前时间，每接收到一个报文(即从接收缓冲区中读取完数据)，把时间戳更新为当前时间
public:
    Connection(EventLoop* loop,int clientfd);
    ~Connection();
    int fd()const;        //返回socket的fd
    string ip()const;
    uint16_t port();
    void closecallback();          //TCP连接关闭(断开)的回调函数,供Channel回调
    void errorcallback();          //TCP连接错误的回调函数,供Channel回调
    void setclosecallback(function<void(spConnection)> fn);   
    void seterrorcallback(function<void(spConnection)> fn);
    void setmessagecallback(function<void(spConnection,std::string&)> fn);  
    void setsendcompletioncallback(function<void(spConnection)> fn);
    void writecallback(); 
    void onmessage();              //处理对端发送过来的消息
    void sendmessage(const char* data,size_t size);           //发送消息给对端，不管在任何线程中，都是调用这个函数发送数据
    void send_inloop(const std::string& message);           //如果在IO线程中，就直接发送，如果不在IO线程中，即在工作线程中，就把此函数传给IO线程去执行   
    bool timeout(time_t now,int val); //判断连接是否超时
};