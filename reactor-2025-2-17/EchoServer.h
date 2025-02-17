#pragma once

#include "Tcpserver.h"
#include "Connection.h"
#include "EventLoop.h"
#include "ThreadPool.h"
#include "Tpl.h"
#include "Socket.h"

class EchoServer
{  
friend class Socket;     
public:
    EchoServer(const string& ip,uint16_t port,int subthreadnum=2,int workthreadnum=2);
    ~EchoServer();
    void Start();
    void Stop();
    void HandleNewConnection(int clientfd);
    void HandleClose(spConnection connection);
    void HandleError(spConnection connection);
    void HandleMessage(spConnection conn,std::string& message); 
    void HandleSendcomplete(spConnection conn);              
    void HandleTimeout(EventLoop* loop);     
    void Handledata(spConnection conn,std::string& message);
    static void showinfo(int clientfd,InetAddress& clientaddr);             
private:
    Tcpserver tcpserver_;
    Tpl* work_tpl;
};