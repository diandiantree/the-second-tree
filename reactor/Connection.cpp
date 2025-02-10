#include "Connection.h"

Connection::Connection(EventLoop* loop,int clientfd)
{
    client_socket = new Socket(clientfd);
    clientchannel=new Channel(loop->getep(),clientfd);
    clientchannel->setreadcallback(std::bind(&Connection::onmessage,this));
    clientchannel->setwritecallback(std::bind(&Connection::setwritecallback,this));
    clientchannel->setclosecallback(std::bind(&Connection::closecallback,this));
    clientchannel->seterrorcallback(std::bind(&Connection::errorcallback,this));
    clientchannel->useET();                         //采用边缘触发
    clientchannel->enabreading();                   // 为新客户端连接准备读事件，并添加到epoll中
}
Connection::~Connection()
{
    delete clientchannel;
    delete client_socket;
}
int Connection::fd()const
{
    return client_socket->fd();
}
string Connection::ip()const
{
    return client_socket->ip();
}
uint16_t Connection::port()
{
    return client_socket->port();
}
void Connection::setclosecallback(function<void(Connection*)> fn)
{
    closecallback_=fn;
}
void Connection::seterrorcallback(function<void(Connection*)> fn)
{
    errorcallback_=fn;
}
void Connection::closecallback()
{
    closecallback_(this);
}          
void Connection::errorcallback()
{
    errorcallback_(this);
} 
void Connection::onmessage()
{   
    char buffer[1024];
    while (true)                                    // 由于使用非阻塞IO，一次读取buffer大小数据，直到全部的数据读取完毕。
    {   
        bzero(&buffer, sizeof(buffer));
        ssize_t nread = read(fd(), buffer, sizeof(buffer));    // 这行代码用了read()，也可以用recv()，一样的，不要纠结。
    if (nread > 0)      // 成功的读取到了数据。
    {
        inputbuffer_.append(buffer,nread);            //接收到数据后先放到接收缓冲区中
    } 
    else if (nread == -1 && errno == EINTR)         // 读取数据的时候被信号中断，继续读取。
    {  
        continue;
    } 
    else if (nread == -1 && ((errno == EAGAIN) || (errno == EWOULDBLOCK))) // 全部的数据已读取完毕,接收缓冲区中没有数据了
    {
        while(true)
        {
        std::string message=inputbuffer_.getmessage();
        if(message==" ") break;
        std::cout << "received message: " << message << std::endl;
        messagecallback_(this,message);
        }
        break;
    } 
    else if (nread == 0)  // 客户端连接已断开
    {  
        closecallback();    //回调Tcpserver::closecallback
        break;
    }
    }
}
void Connection::setmessagecallback(function<void(Connection*,std::string&)> fn)
{
    messagecallback_=std::move(fn);
}
void Connection::setwritecallback()
{
   int writen=::send(fd(),outputbuffer_.data(),outputbuffer_.size(),0); //尝试把outputbuffer_中的全部数据发送给对端，send函数返回成功发送的字节数
    if(writen>0) outputbuffer_.erase(0,writen);  //发送成功后，把已发送的数据从outputbuffer_中删除
    if(outputbuffer_.size()==0) 
    {clientchannel->disabwriting(); //如果outputbuffer_中没有数据了，就不需要再监视fd_的写事件了
    sendcompletioncallback_(this); //发送完数据后的回调函数
    }
}
void Connection::setsendcompletioncallback(function<void(Connection*)> fn)
{
    sendcompletioncallback_=fn;
}
void Connection::sendmessage(const char* data,size_t size)
{
    outputbuffer_.append(data,size);            //发送消息前先放到发送缓冲区中
    clientchannel->enabwriting();               // 为fd_添加写事件
}
