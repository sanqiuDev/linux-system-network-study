# 定义伪目标all，用于生成所有可执行文件
.PHONY:all
all:client server

# 生成server可执行文件的规则
client:Client.cc
	g++ -o $@ $^ -std=c++11
# 生成client可执行文件的规则
server:Server.cc
	g++ -o $@ $^ -std=c++11

# 定义伪目标clean，用于清理生成的文件
.PHONY:clean
clean:
	rm -f client server
