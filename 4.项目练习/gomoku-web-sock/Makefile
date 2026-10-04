.PHONY gobang
gobang:gobang.cc
	g++ -g -std=c++11 $^ -o $@ -L/usr/lib64/mysql -lmysqlclient -ljsoncpp -lpthread


# 1. -L/usr/lib64/mysql
# -L：是 GCC 的选项，用来指定额外的库文件搜索目录。
# /usr/lib64/mysql：这是 MySQL 客户端库文件（libmysqlclient.so）所在的路径。
# 作用：告诉编译器 “去这个目录里找我后面要链接的库文件”，不然编译器可能找不到 mysqlclient 库。

# 2. -lmysqlclient
# -l：是 GCC 的选项，用来指定要链接的库名。
# mysqlclient：MySQL 客户端库的名称，实际对应的文件是 libmysqlclient.so（-l 会自动补全 lib 前缀和 .so 后缀）。
# 作用：链接 MySQL 客户端库，让你的代码里可以调用 MySQL 的 API（比如连接数据库、执行 SQL 语句）。

# 3. -ljsoncpp
# 同理，jsoncpp 是一个 JSON 处理库，文件名为 libjsoncpp.so。
# 作用：链接 JSON 库，让你的代码里可以解析 / 生成 JSON 数据（比如处理前后端通信的消息）。

# 4. -lpthread
# pthread 是 POSIX 线程库，文件名为 libpthread.so。
# 作用：链接线程库，让你的代码里可以使用多线程（比如 WebSocket 服务的事件循环、定时器等）。

# 你虽然没有自己写 pthread_create，
# 但你用的第三方库 —— websocketpp 底层强制依赖线程库！
# 所以 必须加 -lpthread，不然编译报错！



# 1. .PHONY 是干什么的？
# 它用来声明 gobang 是一个伪目标（phony target），意思是：
# 不要去检查当前目录下有没有一个叫 gobang 的文件，每次执行 make gobang 都要执行下面的规则。

# 2. 什么时候必须用？
# 当你的目标名和当前目录下的真实文件名重名时，就必须用它。比如你编译出的程序就叫 gobang，如果不写 .PHONY: gobang，make 会检查：
# 有没有叫 gobang 的文件？
# 这个文件是不是比它的所有依赖都新？
# 如果文件存在且比依赖新，make 会认为 “不需要重新编译”，导致你修改了代码也不会重新构建。
# 加上 .PHONY: gobang 后，make 就会忽略这个文件，每次都执行编译规则，避免了这种 “不更新” 的问题。
