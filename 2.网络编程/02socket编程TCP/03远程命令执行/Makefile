.PHONY:all
all:tcpclient tcpserver

tcpclient:TcpClient.cc
	g++ -o $@ $^ -std=c++17
tcpserver:TcpServer.cc
	g++ -o $@ $^ -std=c++17 -lpthread

.PHONY:clean
clean:
	rm -f tcpclient tcpserver