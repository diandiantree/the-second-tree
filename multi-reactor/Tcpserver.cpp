#include "Tcpserver.h"
#include <memory>


//客户端与服务端的tcp连接断开后，还要再等一段时间，connection才会析构
//虽然connection析构的时候，会主动delete掉clientchannel和client_sock，但是有延迟
//所以在connection析构之前我们要把clientchannel主动从epoll中删除，要不然会一直触发读事件（水平触发：客户端连接已断开）
//所以我们要在connection析构之前，主动调用clientchannel->disableall()，把clientchannel从epoll中删除
Tcpserver::Tcpserver(const string& ip, uint16_t port, int Threadnum) : threadnum(Threadnum)
{
    main_eloop = make_unique<EventLoop>(true); //主事件循环
    main_eloop->settimeoutcallback(std::bind(&Tcpserver::epolltimeout, this, std::placeholders::_1)); //设置epoll_wait()超时的回调函数

    acceptor_ = std::make_unique<Acceptor>(*main_eloop, ip, port);
    acceptor_->setnewconnection(std::bind(&Tcpserver::newconnection, this, std::placeholders::_1)); //不是线程安全的，因为传入了this指针，而this指针是指向当前对象的指针，但是如果此时有别的线程在调用Tcpserver的成员函数，就会出现问题，因为他还没有完全被构造出来

    tpl = &(Threadpool::Get_pl(threadnum)); //创建线程池，因为线程池是单例模式，所以这里不需要new
    //创建从事件循环，也就相当于创建任务队列中的任务
    for (int i = 0; i < threadnum; i++) {
        sub_eloops.emplace_back(make_unique<EventLoop>(false,5,10));
        sub_eloops[i]->settimeoutcallback(std::bind(&Tcpserver::epolltimeout, this, std::placeholders::_1)); //设置epoll_wait()超时的回调函数
        sub_eloops[i]->set_close_callback(std::bind(&Tcpserver::delete_connection, this, std::placeholders::_1)); //设置删除连接的回调函数
        tpl->enqueue(std::bind(&EventLoop::run, sub_eloops[i].get())); //把从事件循环放到线程池中，线程池中的线程会去执行从事件循环的run()函数
    }
}

Tcpserver::~Tcpserver()
{
}

void Tcpserver::start()
{
    main_eloop->run();
}

void Tcpserver::newconnection(int clientfd)
{
    auto connection = std::make_shared<Connection>(sub_eloops[clientfd % threadnum].get(), clientfd);
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        connections[clientfd] = std::move(connection);//把conn放在map容器中
    }
    connections[clientfd]->setclosecallback(std::bind(&Tcpserver::closeconnection, this, std::placeholders::_1));
    connections[clientfd]->seterrorcallback(std::bind(&Tcpserver::errorconnection, this, std::placeholders::_1));
    connections[clientfd]->setmessagecallback(std::bind(&Tcpserver::onmessage, this, std::placeholders::_1, std::placeholders::_2));
    connections[clientfd]->setsendcompletioncallback(std::bind(&Tcpserver::sendcompletion, this, std::placeholders::_1));
    newconnection_(clientfd);
    sub_eloops[clientfd % threadnum].get()->add_connection(connections[clientfd]);//使用move之后，就不能使用connection了，所以要用connections[clientfd]
}

void Tcpserver::closeconnection(spConnection connection)
{
    closeconnection_(connection);
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        connections.erase(connection->fd());
    }
}

void Tcpserver::errorconnection(spConnection connection)
{
    errorconnection_(connection);
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        connections.erase(connection->fd());
    }
}

void Tcpserver::onmessage(spConnection conn, std::string message)
{
    onmessage_(conn, message);
}

void Tcpserver::sendcompletion(spConnection conn)
{
    sendcompletion_(conn);
}

void Tcpserver::epolltimeout(EventLoop* loop)
{
    epolltimeout_(loop);
}

void Tcpserver::setnewconnection(std::function<void(int)> fn)
{
    newconnection_ = std::move(fn);
}

void Tcpserver::setcloseconnection(std::function<void(spConnection)> fn)
{
    closeconnection_ = std::move(fn);
}

void Tcpserver::seterrorconnection(std::function<void(spConnection)> fn)
{
    errorconnection_ = std::move(fn);
}

void Tcpserver::setmessagecallback(std::function<void(spConnection, std::string&)> fn)
{
    onmessage_ = std::move(fn);
}

void Tcpserver::setsendcompletioncallback(std::function<void(spConnection)> fn)
{
    sendcompletion_ = std::move(fn);
}

void Tcpserver::setepolltimeout(std::function<void(EventLoop*)> fn)
{
    epolltimeout_ = std::move(fn);
}
void Tcpserver::delete_connection(std::shared_ptr<Connection> con)
{
    {
    std::lock_guard<std::mutex> lock(m_mutex);
    connections.erase(con->fd());
    }
}