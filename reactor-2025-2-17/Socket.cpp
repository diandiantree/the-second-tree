#include "Socket.h"
#include "Connection.h"
#include "EchoServer.h"

int createnonblocking()
{
// 创建服务端用于监听的listenfd。
int listenfd = socket(AF_INET,SOCK_STREAM|SOCK_NONBLOCK,IPPROTO_TCP);//添加SOCK_NONBLOCK直接可以创建非阻塞的listenfd
if (listenfd < 0)
{
    std::cout<<"listen socket create error:"<<__FILE__<<__FUNCTION__<<__LINE__<<errno<<endl;//输出错误信息，包括文件名，函数，行号等
    exit(-1);
}
return listenfd;
}
Socket::Socket(int fd):fd_(fd){}      
Socket:: ~Socket(){close(fd_);}           
int Socket::fd()const{return fd_;}      
void Socket::set_REUSEADDR(bool on)
{
    int opt=on?1:0;
    setsockopt(fd_,SOL_SOCKET,SO_REUSEADDR,&opt,static_cast<socklen_t>(sizeof opt)); 
}
void Socket::set_NODELAY(bool on)
{
    int opt=on?1:0;
    setsockopt(fd_,SOL_SOCKET,TCP_NODELAY,&opt,static_cast<socklen_t>(sizeof opt)); 
}
void Socket::set_REUSEPORT(bool on)
{
    int opt=on?1:0;
    setsockopt(fd_,SOL_SOCKET,SO_REUSEPORT,&opt,static_cast<socklen_t>(sizeof opt)); 
}
void Socket::set_KEEPALIVE(bool on)
{
    int opt=on?1:0;
    setsockopt(fd_,SOL_SOCKET,SO_KEEPALIVE,&opt,static_cast<socklen_t>(sizeof opt)); 
}
void Socket::bind(const InetAddress& serveraddr)
{
    if (::bind(fd_,serveraddr.addr(),sizeof(sockaddr)) < 0 )
    {
        perror("bind() failed"); close(fd_); exit(-1);
    }
    ip_=serveraddr.ip();
    port_=serveraddr.port();
    cout<<"bind success"<<endl;
}
void Socket::listen(int n)
{
    if (::listen(fd_,n) != 0 )        // 在高并发的网络服务器中，第二个参数要大一些
    {
        perror("listen() failed"); close(fd_); exit(-1);
    }
    cout<<"listen success"<<endl;
}
int Socket::accept()
{
    struct sockaddr_in peeraddr;
    socklen_t len = sizeof(peeraddr);
    int clientfd = accept4(fd_,(sockaddr*)&peeraddr,&len,SOCK_NONBLOCK);//同socket函数，变为非阻塞的fd
    InetAddress clientaddr(peeraddr);
    EchoServer::showinfo(clientfd,clientaddr);//std::cout<<"new connection fd="<<clientfd<<" "<<"ip="<<clientaddr.ip()<<" "<<"port="<<clientaddr.port()<<" "<<std::endl;
    return clientfd;
}
string Socket::ip()const
{
    return ip_;
};                             
uint16_t Socket::port()const
{
    return port_;
};
