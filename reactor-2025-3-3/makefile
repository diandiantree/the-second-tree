all:client tcp

client:client.cpp
	g++ -g -o client client.cpp
tcp:tcp.cpp InetAddress.cpp Socket.cpp Epoll.cpp Channel.cpp EventLoop.cpp Tcpserver.cpp Acceptor.cpp Connection.cpp Buffer.cpp EchoServer.cpp ThreadPool.cpp Tpl.cpp TimeStamp.cpp
	g++ -g -o tcp tcp.cpp InetAddress.cpp Socket.cpp Epoll.cpp Channel.cpp EventLoop.cpp Tcpserver.cpp Acceptor.cpp Connection.cpp Buffer.cpp EchoServer.cpp ThreadPool.cpp Tpl.cpp TimeStamp.cpp -lpthread

clean:
	rm -f client tcp
