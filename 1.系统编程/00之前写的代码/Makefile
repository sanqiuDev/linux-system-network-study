
#------------------------------- 链接所有的”位置无关目标文件“生成”动态库文件“ -------------------------------#
# 目标：生成动态库文件 libmyc.so
# 依赖：mystdio.o 和 mystring.o 两个目标文件
# 命令：gcc -shared -o $@ $^
#    1. -shared：指定生成动态库
#    2. -o $@：将输出文件命名为当前目标（即：libmyc.so）
#    3. $^：自动变量，代表所有依赖文件（即：mystdio.o mystring.o）
libmyc.so: mystdio.o mystring.o
	gcc -shared -o $@ $^

#------------------------------- 编译“源文件”生成“位置无关目标文件” -------------------------------#
# 目标：生成 mystdio.o 目标文件
# 依赖：mystdio.c 源文件
# 命令：gcc -fPIC -c $<
#    1. -fPIC：生成位置无关代码，是动态库的关键要求
#    2. -c：只编译源文件，生成目标文件（.o），不进行链接
#    3. $<：自动变量，代表第一个依赖文件（即：mystdio.c）
mystdio.o: mystdio.c
	gcc -fPIC -c $<


# 目标：生成 mystring.o 目标文件
# 依赖：mystring.c 源文件
# 命令：gcc -fPIC -c $< （同上面的编译逻辑，处理 mystring.c）
mystring.o: mystring.c
	gcc -fPIC -c $<


#------------------------------- 声明“output 是伪目标”---> 打包动态库和头文件，方便分发 -------------------------------#
# .PHONY：声明 output 是伪目标（即使存在同名文件，也强制执行命令）
.PHONY: output
output :

# 创建目录结构：lib/include（存放头文件）、lib/mylib（存放动态库）
	mkdir -p lib/include
	mkdir -p lib/mylib

# 复制所有.h 头文件到 lib/include 目录（ -f：强制覆盖已有文件）
	cp -f *.h lib/include

# 复制所有.so 动态库文件到 lib/mylib 目录
	cp -f *.so lib/mylib

# 将 lib 目录压缩为 lib.tgz（.tar.gz 格式），方便传输
	tar -czf lib.tgz lib



#------------------------------- 声明“clean 是伪目标”---> 清理编译生成的中间文件和目标文件，恢复初始状态 -------------------------------#
# .PHONY：声明 clean 是伪目标
.PHONY: clean
clean :

# 删除所有.o 目标文件、动态库 libmyc.so、lib 目录及压缩包 lib.tgz
	rm -rf *.o libmyc.so lib lib.tgz



