#pragma once  
 
// 标准库头文件包含
#include <iostream>         // 提供 ---> 标准输入输出功能（如调试打印）
#include <cstring>          // 提供 ---> C风格字符串操作函数（如memset、strerror等）
#include <functional>       // 提供 ---> std::function模板（用于定义回调函数类型，如TcpServer中的func_t）
#include <unistd.h>         // 提供 ---> POSIX系统调用（如close、sleep等）

// 网络编程相关系统头文件（POSIX标准，封装socket API）
#include <sys/socket.h>     // 提供 ---> socket核心函数（socket、bind、listen、accept、connect等）
#include <sys/types.h>      // 提供 ---> 基础系统类型定义（如pid_t、ssize_t等，配合socket API使用）
#include <arpa/inet.h>      // 提供 ---> IP地址转换函数（如inet_addr、htons等，主机字节序与网络字节序转换）
#include <netinet/in.h>     // 提供 ---> 网络地址结构体定义（如sockaddr_in，存储IPv4地址和端口）

//1.程序退出错误码定义 ---> 统一管理程序中各种错误场景的退出标识，使错误类型更清晰（替代魔法数字）
enum ExitCode
{
    OK = 0,               // 无错误，程序正常退出
    USAGE_ERR,            // 命令行参数使用错误（如：参数个数不对、格式错误）
    SOCKET_ERR,           // 创建socket失败错误
    BIND_ERR,             // 绑定端口（bind）失败错误
    LISTEN_ERR,           // 监听端口（listen）失败错误
    CONNECT_ERR,          // 发起连接（connect）失败错误（客户端专用）
    FORK_ERR              // 创建子进程（fork）失败错误（多进程服务器专用）
};

//2.不可拷贝基类 ---> 用于禁止子类对象的拷贝和赋值操作
// 设计理念：某些类（如：TcpServer、线程池）的对象应全局唯一，拷贝/赋值会导致资源冲突（如重复关闭套接字）
class NoCopy
{
public:
    //1.“默认构造函数” ---> 允许子类正常创建对象
    NoCopy(){}               
    //2.“默认析构函数” ---> 允许子类正常销毁对象，释放资源 
    ~NoCopy(){}              

    //3.“禁用拷贝构造函数” ---> 禁止通过已有对象创建新对象（如：NoCopy a; NoCopy b = a;）
    NoCopy(const NoCopy &) = delete;

    //4.“禁用赋值运算符重载” ---> 禁止对象间的赋值操作（如：NoCopy a; NoCopy b; b = a;）
    const NoCopy &operator = (const NoCopy&) = delete;
};

//3.地址类型转换宏定义 ---> 简化socket API中地址结构体的类型转换（sockaddr_in ↔ sockaddr）
#define CONV(addr) ((struct sockaddr*)&addr)

/* 宏的使用背景：
*     1. socket API的函数（如：bind、accept、connect）要求传入struct sockaddr*类型的地址指针，
*     2. 但实际存储IPv4地址时使用的是struct sockaddr_in（更易操作IP和端口）
*
*  宏的使用示例：
*    struct sockaddr_in local;
*    bind(sockfd, CONV(local), sizeof(local));  // 等价于 (struct sockaddr*)&local
*/
