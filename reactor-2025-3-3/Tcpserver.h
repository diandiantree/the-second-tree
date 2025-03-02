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
#include "ThreadPool.h"
#include <vector>
#include <mutex>

class Tcpserver
{
private:
    int timeval;        //多少秒检查一下闹钟
    int timeout;       //多少秒超时
    int timer_fd;
    std::unique_ptr<Channel> timer_channel;    //定时器channel
    std::unique_ptr<EventLoop> main_eloop;     //一个Tcpserver类可以有多个事件循环,但是只有一个主事件循环
    std::vector<std::unique_ptr<EventLoop>> sub_eloops; //从事件循环
    Threadpool* tpl; //线程池,从事件运行在线程池中，一个线程运行一个事件循环
    int threadnum; //线程池的线程数量，也就是从事件循环的数量
    std::mutex m_mutex; //由于connections会被多线程访问，所以需要加锁
    std::unique_ptr<Acceptor> acceptor_; //一个Tcpserver类只有一个acceptor对象
    std::map<int,Connection*> connections;
    std::function<void(Connection*)> closeconnection_;
    std::function<void(Connection*)> errorconnection_;
    std::function<void(Connection*,std::string&)> onmessage_;
    std::function<void(Connection*)> sendcompletion_;
    std::function<void(EventLoop*)> epolltimeout_;
public:
    Tcpserver(const string& ip,uint16_t port,int threadnum=3,int timeval_=5,int timeout_=20);
    ~Tcpserver();
    void start();
    void stop();
    void newconnection(int clientfd);
    void closeconnection(Connection* connection);
    void errorconnection(Connection* connection);
    void onmessage(Connection* conn,std::string message); //处理客户端的请求报文，在Connection类中回调此函数
    void sendcompletion(Connection* conn);               //发送完数据后的回调函数
    void epolltimeout(EventLoop* loop);                  //epoll_wait()超时的回调函数
    void setcloseconnection(std::function<void(Connection*)> fn);
    void seterrorconnection(std::function<void(Connection*)> fn);
    void setmessagecallback(std::function<void(Connection*,std::string&)> fn);
    void setsendcompletioncallback(std::function<void(Connection*)> fn);
    void setepolltimeout(std::function<void(EventLoop*)> fn);
    void handle_timer();  //处理定时器事件
};