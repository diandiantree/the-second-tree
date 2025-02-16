#pragma once

#include "Tcpserver.h"
#include "Connection.h"
#include "EventLoop.h"

class EchoServer
{       
public:
    EchoServer(const string& ip,uint16_t port);
    ~EchoServer();
    void Start();
    void HandleNewConnection(int clientfd);
    void HandleClose(Connection* connection);
    void HandleError(Connection* connection);
    void HandleMessage(Connection* conn,std::string& message); 
    void HandleSendcomplete(Connection* conn);              
    void HandleTimeout(EventLoop* loop);                  
private:
    Tcpserver tcpserver_;
};