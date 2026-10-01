# ===================== 变量定义 =====================
# 目标程序名变量：定义编译生成的可执行文件名称，便于统一管理
PARSER=parser           # 解析器程序（预处理HTML，生成结构化文档数据）
DUG=debug               # 调试程序（测试索引构建/搜索功能）
HTTP_SERVER=http_server # HTTP服务器程序（对外提供搜索接口）

# 编译器变量：指定C++编译器（g++），便于后续切换编译器（如clang++）
cc=g++

# ===================== 伪目标定义 =====================
# .PHONY：声明伪目标（非文件目标），避免与同名文件冲突
# all：默认目标（执行make时默认执行），依赖parser、debug、http_server三个目标
.PHONY:all
all:$(PARSER)  $(HTTP_SERVER)  # 编译所有目标程序

# ===================== 编译规则 =====================
# 规则格式：目标:依赖文件
#          命令（必须以Tab开头）
# $@：自动替换为当前目标名（如parser）
# $^：自动替换为所有依赖文件（如parser.cc）

# 1. 编译parser程序（HTML解析器）
# $(PARSER):parser.cc
# 	$(cc) -o $@ $^ -lboost_system -lboost_filesystem -std=c++11
	

$(PARSER):parser.cc
	g++ -o $@ $^ \
		-I./ \
		-I/usr/local/third_party/cppjieba/deps/limonp/include \
		-lboost_system -lboost_filesystem \
		-std=c++17
# 编译命令说明：
# -o $@：输出可执行文件名为目标名（parser）
# $^：依赖文件为parser.cc
# -lboost_system：链接Boost系统库（文件系统库依赖）
# -lboost_filesystem：链接Boost文件系统库（递归遍历目录）
# -std=c++11：启用C++11标准（支持lambda、std::move等特性）



# # 2. 编译debug程序（调试工具）
# # $(DUG):debug.cc
# # 	$(cc) -o $@ $^ -ljsoncpp -std=c++11


# $(DUG):debug.cc
# 	g++ -o $@ $^ \
# 		-I./ \
# 		-I/usr/local/third_party/cppjieba/deps/limonp/include \
# 		-lboost_system -lboost_filesystem \
# 		-std=c++17
# # 编译命令说明：
# # -ljsoncpp：链接JsonCpp库（JSON序列化/反序列化）
# # -std=c++11：启用C++11标准


# 3. 编译http_server程序（HTTP服务器）
$(HTTP_SERVER):http_server.cc
	$(cc) -o $@ $^ \
    	-I./ \
    	-I/usr/local/third_party/cppjieba/deps/limonp/include \
    	-lboost_system -lboost_filesystem \
    	-ljsoncpp \
		-pthread \
    	-std=c++17
# 编译命令说明：
# -ljsoncpp：链接JsonCpp库（处理JSON格式的请求/响应）
# -lpthread：链接线程库（HTTP服务器多线程处理请求）
# -std=c++11：启用C++11标准

# ===================== 清理目标 =====================
# clean：伪目标，清理编译生成的可执行文件
.PHONY:clean
clean:
	rm -f $(PARSER)  $(HTTP_SERVER)
# rm -f：强制删除文件（忽略不存在的文件，无报错）
# $(PARSER) $(DUG) $(HTTP_SERVER)：删除所有编译生成的可执行文件
