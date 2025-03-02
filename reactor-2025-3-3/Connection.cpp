#include "Connection.h"

Connection::Connection(EventLoop* loop, int clientfd) : loop_(loop), disconnect_(false)
{
    client_socket = make_unique<Socket>(clientfd);
    clientchannel = make_unique<Channel>(loop_, clientfd);
    clientchannel->setreadcallback(std::bind(&Connection::onmessage, this));
    clientchannel->setwritecallback(std::bind(&Connection::send_client, this));
    clientchannel->setclosecallback(std::bind(&Connection::close_client, this));
    clientchannel->seterrorcallback(std::bind(&Connection::error_client, this));
    clientchannel->useET();                         //采用边缘触发  
    clientchannel->enabreading();                   //为新客户端连接准备读事件，并添加到epoll中
}

Connection::~Connection()
{
    // 清除所有回调函数，避免访问已销毁的对象
    clientchannel->setreadcallback(nullptr);
    clientchannel->setwritecallback(nullptr);
    clientchannel->setclosecallback(nullptr);
    clientchannel->seterrorcallback(nullptr);
}

int Connection::fd() const
{
    return client_socket->fd();
}

string Connection::ip() const
{
    return client_socket->ip();
}

uint16_t Connection::port()
{
    return client_socket->port();
}

void Connection::setclosecallback(function<void(Connection*)> fn)
{
    closecallback_ = fn;
}

void Connection::seterrorcallback(function<void(Connection*)> fn)
{
    errorcallback_ = fn;
}

void Connection::close_client()
{
    disconnect_ = true; // 关闭 TCP 链接时，将 disconnect_ 设置为 true
    if (closecallback_) {
        closecallback_(this);
    }
}

void Connection::error_client()
{
    disconnect_ = true;
    if (errorcallback_) {
        errorcallback_(this);
    }
}

void Connection::onmessage()
{
    char buffer[1024];
    while (true) // 由于使用非阻塞IO，一次读取buffer大小数据，直到全部的数据读取完毕。
    {
        bzero(&buffer, sizeof(buffer));
        ssize_t nread = read(fd(), buffer, sizeof(buffer)); // 这行代码用了read()，也可以用recv()，一样的。
        if (nread > 0) // 成功的读取到了数据。
        {
            inputbuffer_.append(buffer, nread); //接收到数据后先放到接收缓冲区中
        }
        else if (nread == -1 && errno == EINTR) // 读取数据的时候被信号中断，继续读取。
        {
            continue;
        }
        else if (nread == -1 && ((errno == EAGAIN) || (errno == EWOULDBLOCK))) // 全部的数据已读取完毕,接收缓冲区中没有数据了
        {
            while (true)
            {
                std::string message = inputbuffer_.getmessage();
                if (message == " ") break;
                last_time = TimeStamp::Now(); //每接收到一个报文，把时间戳更新为当前时间
                if (messagecallback_) {
                    messagecallback_(this, message); //这里最终回调的是EchoServer::HandleMessage()
                }
            }
            break;
        }
        else if (nread == 0) // 客户端连接已断开
        {
            clientchannel->remove(); //从事件循环中删除channel
            close_client(); //回调Tcpserver::closecallback
            break;
        }
    }
}

void Connection::setmessagecallback(function<void(Connection*, std::string&)> fn)
{
    messagecallback_ = std::move(fn);
}

void Connection::send_client()
{
    int writen = ::send(fd(), outputbuffer_.data(), outputbuffer_.size(), 0); //尝试把outputbuffer_中的全部数据发送给对端，send函数返回成功发送的字节数
    if (writen >= 0) outputbuffer_.erase(0, writen); //发送成功后，把已发送的数据从outputbuffer_中删除
    else
    {
        perror("send failed");
    }
    if (outputbuffer_.size() == 0)
    {
        clientchannel->disabwriting(); //如果outputbuffer_中没有数据了，就不需要再监视fd_的写事件了
        
        if (sendcompletioncallback_) {
            sendcompletioncallback_(this); // 调用发送完成回调
        }
    }
    
}

void Connection::setsendcompletioncallback(function<void(Connection*)> fn)
{
    sendcompletioncallback_ = fn;
}

void Connection::sendmessage(const char* data, size_t size)
{
    if (disconnect_ == true) { std::cout << "connection has been closed" << std::endl; return; } //如果连接已经断开，就不发送数据了
    std::string message(data, size); // 使用 std::string 来管理数据
    if (loop_->is_inloop_thread())
    {
        send_inloop(message); //如果在IO线程中，就直接发送，如果不在IO线程中，即在工作线程中，就把此函数传给IO线程去执行
    }
    else
    {
        loop_->addtask(std::bind(&Connection::send_inloop, this, message));
    }
//这样做的目的是只让工作线程去处理数据，处理完数据后，工作线程会调用sendmessage函数，把处理好的数据传进来，然后IO线程负责把处理好的数据发给outbuffer，然后激活写事件，channel调用回调写函数发送数据
}

void Connection::send_inloop(const std::string& message)
{
    outputbuffer_.append(message.data(), message.size()); //发送消息前先放到发送缓冲区中
    clientchannel->enabwriting(); // 为fd_添加写事件
}

bool Connection::timeout(time_t now, int val)
{
    return (now - last_time.toint()) > val;
}