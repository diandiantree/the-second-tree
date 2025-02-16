#include "EchoServer.h"

EchoServer::EchoServer(const string& ip,uint16_t port):tcpserver_(ip,port)//调用Tcpserver的构造函数
{
    tcpserver_.setnewconnection(std::bind(&EchoServer::HandleNewConnection,this,std::placeholders::_1));
    tcpserver_.setcloseconnection(std::bind(&EchoServer::HandleClose,this,std::placeholders::_1));
    tcpserver_.seterrorconnection(std::bind(&EchoServer::HandleError,this,std::placeholders::_1));
    tcpserver_.setmessagecallback(std::bind(&EchoServer::HandleMessage,this,std::placeholders::_1,std::placeholders::_2));
    tcpserver_.setsendcompletioncallback(std::bind(&EchoServer::HandleSendcomplete,this,std::placeholders::_1));
    tcpserver_.setepolltimeout(std::bind(&EchoServer::HandleTimeout,this,std::placeholders::_1));
}
EchoServer::~EchoServer()
{
}
void EchoServer::Start()
{
    tcpserver_.start();
}
void EchoServer::HandleNewConnection(int clientfd)
{
    printf("new connection eventfd=%d\n",clientfd);
}
void EchoServer::HandleClose(Connection* connection)
{
    printf("client(eventfd=%d) disconnected.\n",connection->fd());
}
void EchoServer::HandleError(Connection* connection)
{
    printf("client(eventfd=%d) error.\n",connection->fd());
}
void EchoServer::HandleMessage(Connection* conn,std::string& message)
{
    message="reply:"+message;
    int len=message.size();                            //计算报文大小,是没有报文头的需要加报文头
    std::string tmp((char*)&len,4);
    tmp.append(message);
    send(conn->fd(),tmp.data(),tmp.size(),0);
}
void EchoServer::HandleSendcomplete(Connection* conn)
{
    printf("send completion.\n");
}
void EchoServer::HandleTimeout(EventLoop* loop)
{
    printf("epoll_wait() timeout.\n");
}
