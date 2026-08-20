.PHONY:all
all:server_netcal client_netcal

server_netcal:Main.cc
	g++ -o $@ $^ -std=c++17 -ljsoncpp

client_netcal:TcpClient.cc
	g++ -o $@ $^ -std=c++17 -ljsoncpp

.PHONY:clean
clean:
	rm -f server_netcal client_netcal