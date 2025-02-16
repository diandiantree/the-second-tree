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
    std::unique_ptr<EventLoop> main_eloop;     //一个Tcpserver类可以有多个事件循环,但是只有一个主事件循环
    std::vector<std::unique_ptr<EventLoop>> sub_eloops; //从事件循环
    Threadpool* tpl; //线程池,从事件运行在线程池中，一个线程运行一个事件循环
    int threadnum; //线程池的线程数量，也就是从事件循环的数量
    std::mutex m_mutex; //由于connections会被多线程访问，所以需要加锁
    std::unique_ptr<Acceptor> acceptor_; //一个Tcpserver类只有一个acceptor对象
    std::map<int,spConnection> connections;
    std::function<void(int)> newconnection_;
    std::function<void(spConnection)> closeconnection_;
    std::function<void(spConnection)> errorconnection_;
    std::function<void(spConnection,std::string&)> onmessage_;
    std::function<void(spConnection)> sendcompletion_;
    std::function<void(EventLoop*)> epolltimeout_;
public:
    Tcpserver(const string& ip,uint16_t port,int threadnum=3);
    ~Tcpserver();
    void start();
    void newconnection(int clientfd);
    void closeconnection(spConnection connection);
    void errorconnection(spConnection connection);
    void onmessage(spConnection conn,std::string message); //处理客户端的请求报文，在Connection类中回调此函数
    void sendcompletion(spConnection conn);               //发送完数据后的回调函数
    void epolltimeout(EventLoop* loop);                  //epoll_wait()超时的回调函数
    void setnewconnection(std::function<void(int)> fn);
    void setcloseconnection(std::function<void(spConnection)> fn);
    void seterrorconnection(std::function<void(spConnection)> fn);
    void setmessagecallback(std::function<void(spConnection,std::string&)> fn);
    void setsendcompletioncallback(std::function<void(spConnection)> fn);
    void setepolltimeout(std::function<void(EventLoop*)> fn);
    void delete_connection(std::shared_ptr<Connection> con);
};