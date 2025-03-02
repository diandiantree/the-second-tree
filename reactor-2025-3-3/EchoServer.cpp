#include "EchoServer.h"
#include "TimeStamp.h"
#include <sys/syscall.h>

std::mutex send_mutex;

EchoServer::EchoServer(const string& ip, uint16_t port, int subthreadnum, int workthreadnum)
    : tcpserver_(ip, port, subthreadnum) // 调用 Tcpserver 的构造函数
{
    work_tpl = &(Tpl::Get_pl(workthreadnum)); // 创建工作线程池，用于处理业务逻辑，因为业务有可能会阻塞，所以要把 IO 和业务逻辑分开，让从事件循环处理 IO，让工作线程处理业务逻辑
    tcpserver_.setcloseconnection(std::bind(&EchoServer::HandleClose, this, std::placeholders::_1));
    tcpserver_.seterrorconnection(std::bind(&EchoServer::HandleError, this, std::placeholders::_1));
    tcpserver_.setmessagecallback(std::bind(&EchoServer::HandleMessage, this, std::placeholders::_1, std::placeholders::_2));
    tcpserver_.setsendcompletioncallback(std::bind(&EchoServer::HandleSendcomplete, this, std::placeholders::_1));
    tcpserver_.setepolltimeout(std::bind(&EchoServer::HandleTimeout, this, std::placeholders::_1));
}

EchoServer::~EchoServer()
{
   
}

void EchoServer::Start()
{
    tcpserver_.start();
}

void EchoServer::Stop()
{
    tcpserver_.stop();
}

void EchoServer::HandleClose(spConnection connection)
{
   std::cout<<TimeStamp::Now().totime()<<"client("<<connection->fd()<<") disconnected"<<std::endl;
}

void EchoServer::HandleError(spConnection connection)
{
    printf("client(eventfd=%d) error.\n", connection->fd());
}

void EchoServer::HandleMessage(spConnection conn, std::string& message)
{
    if (work_tpl->size() == 0)
    {
        Handledata(conn, message);
    }
    else
    {
        work_tpl->enqueue(std::bind(&EchoServer::Handledata, this, conn, message)); //  工作线程池与处理业务逻辑的函数绑定，工作线程池与负责处理IO的线程池是分开的
    }
//因为线程池采用单例模式，所以在EchoServer的头文件中添加了Tpl.h头文件，这样就可以在EchoServer类中使用新的线程池了
}

void EchoServer::HandleSendcomplete(spConnection conn)
{
    //printf("send completion.\n");
}

void EchoServer::HandleTimeout(EventLoop* loop)
{
    printf("epoll_wait() timeout.\n");
}

void EchoServer::Handledata(spConnection conn, std::string& message)
{
    //std::cout<<TimeStamp::Now().totime()<< "received message: " << message << std::endl;
    message = "reply:" + message; // 业务处理的代码，虽然只有一行，简单的回显，但确实是业务处理，也就是说这一行应该交给工作线程去完成
    int len = message.size(); // 计算报文大小,是没有报文头的需要加报文头
    std::string tmp((char*)&len, 4);
    tmp.append(message);
    conn->sendmessage(tmp.data(), tmp.size()); //发送给outbuffer,不是直接发送给对端
}