#include "Tcpserver.h"
#include <memory>

Tcpserver::Tcpserver(const string& ip,uint16_t port)
{
    acceptor_ = std::make_unique<Acceptor>(eloop,ip,port);
    acceptor_->setnewconnection(std::bind(&Tcpserver::newconnection,this,std::placeholders::_1));   //不是线程安全的，因为传入了this指针，而this指针是指向当前对象的指针，但是如果此时有别的线程在调用Tcpserver的成员函数，就会出现问题，因为他还没有完全被构造出来
    eloop.settimeoutcallback(std::bind(&Tcpserver::epolltimeout,this,std::placeholders::_1));     //设置epoll_wait()超时的回调函数
}
Tcpserver::~Tcpserver()=default;
void Tcpserver::start()
{
    eloop.run();
}
void Tcpserver::newconnection(int clientfd)
{
    auto connection=std::make_unique<Connection>(&eloop,clientfd);
    connections[clientfd]=std::move(connection);    /*为什么不能使用 connections.insert({clientfd, connection}) 或 connections[clientfd] = connection
                                                    connections.insert({clientfd, connection}) 和 connections[clientfd] = connection 都会尝试拷贝 connection
                                                    但 std::unique_ptr 不支持拷贝操作，只支持移动操作。因此，这两种方式都会导致编译错误。*/
    connections[clientfd]->setclosecallback(std::bind(&Tcpserver::closeconnection, this, std::placeholders::_1));
    connections[clientfd]->seterrorcallback(std::bind(&Tcpserver::errorconnection, this, std::placeholders::_1));
    connections[clientfd]->setmessagecallback(std::bind(&Tcpserver::onmessage, this, std::placeholders::_1, std::placeholders::_2));
    connections[clientfd]->setsendcompletioncallback(std::bind(&Tcpserver::sendcompletion, this, std::placeholders::_1));
    newconnection_(clientfd);
}
void Tcpserver::closeconnection(Connection* connection)
{
    closeconnection_(connection);
    connections.erase(connection->fd());  
}
void Tcpserver::errorconnection(Connection* connection)
{
    errorconnection_(connection);
    connections.erase(connection->fd());                    
}
void Tcpserver::onmessage(Connection* conn,std::string message)
{
    onmessage_(conn,message);
}
void Tcpserver::sendcompletion(Connection* conn)
{
    sendcompletion_(conn);
}
void Tcpserver::epolltimeout(EventLoop* loop)
{
    epolltimeout_(loop);
}
void Tcpserver::setnewconnection(std::function<void(int)> fn)
{
    newconnection_=std::move(fn);
}
void Tcpserver::setcloseconnection(std::function<void(Connection*)> fn)
{
    closeconnection_=std::move(fn);
}
void Tcpserver::seterrorconnection(std::function<void(Connection*)> fn)
{
    errorconnection_=std::move(fn);
}
void Tcpserver::setmessagecallback(std::function<void(Connection*,std::string&)> fn)
{
    onmessage_=std::move(fn);
}
void Tcpserver::setsendcompletioncallback(std::function<void(Connection*)> fn)
{
    sendcompletion_=std::move(fn);
}
void Tcpserver::setepolltimeout(std::function<void(EventLoop*)> fn)
{
    epolltimeout_=std::move(fn);
}