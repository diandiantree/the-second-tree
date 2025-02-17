#include "Acceptor.h"
#include "Connection.h"
#include <memory>

Acceptor::Acceptor(EventLoop& loop, const string& ip, uint16_t port)
    : loop_(&loop),
      listen_socket(nullptr),
      serverchannel(nullptr)
{
    // 创建服务端用于监听的listen_socket,这里会调用Socket的构造函数，为什么要创建它，因为它是指针类型，需要new出来
    listen_socket = std::make_unique<Socket>(createnonblocking());
    // 设置listenfd的属性
    listen_socket->set_KEEPALIVE(true);
    listen_socket->set_NODELAY(true);
    listen_socket->set_REUSEADDR(true);
    listen_socket->set_REUSEPORT(true);

    InetAddress servaddr(ip, port); // 创建服务端的地址协议

    listen_socket->bind(servaddr);
    listen_socket->listen();

    serverchannel = std::make_unique<Channel>(loop_, listen_socket->fd());
    serverchannel->setreadcallback(std::bind(&Acceptor::newconnection, this)); // 设置回调函数
    serverchannel->enabreading(); // 让epoll_wait()监视fd_的读事件
}

Acceptor::~Acceptor() = default; // 析构函数，不需要做什么事情

void Acceptor::newconnection()
{
    int clientfd = listen_socket->accept();
    if (clientfd < 0) {
        perror("accept failed");
        return;
    }
    newconnection_(clientfd);
}

void Acceptor::setnewconnection(std::function<void(int)> fn)
{
    newconnection_ = std::move(fn); // 使用move可以避免拷贝，提高效率
}