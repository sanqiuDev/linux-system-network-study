#pragma once  

/*-----------------------------------------------头文件-----------------------------------------------*/
// 标准库头文件包含
#include <iostream>         // 标准输入输出（日志打印辅助）
#include <string>           // 字符串处理（预留扩展）
#include <unistd.h>         // POSIX系统调用（如close函数）
#include <cstdlib>          // 标准库函数（如exit）
#include <memory>           // 智能指针（std::shared_ptr）

#include <sys/socket.h>     // socket核心函数（socket、bind、listen、accept等）
#include <sys/types.h>      // 基础系统类型（如pid_t、socklen_t）
#include <netinet/in.h>     // 网络地址结构体（sockaddr_in）
#include <arpa/inet.h>      // IP地址转换函数（如inet_pton、inet_ntop）

// 自定义头文件包含
#include "Log.hpp"          // 日志模块（打印运行日志）
#include "InetAddr.hpp"     // 网络地址封装类（简化IP/端口操作）
#include "Common.hpp"       // 通用工具头文件（错误码、CONV宏等）


/*-----------------------------------------------命令空间-----------------------------------------------*/
// 命名空间：封装Socket相关类，避免全局命名空间污染
namespace SocketModule
{
    /*------------------------------------------准备阶段------------------------------------------*/
    //1.引入日志模块命名空间，简化LOG宏调用
    using namespace LogModule;

    //2.默认Socket文件描述符（无效值，用于初始化）
    const static int defaultfd = -1;

    //3.listen函数的默认backlog参数（半连接队列大小）
    const static int gbacklog = 16;


    /*------------------------------------------基类Socket------------------------------------------*/
    // 基类Socket：采用「模板方法模式」设计，定义Socket操作的骨架流程
    // 核心思想：基类定义算法骨架（如：BuildTcpSocketMethod），具体步骤由子类实现
    class Socket
    {
    public:
        /*-----------------------【虚析构函数】-----------------------*/
        //1.确保子类对象通过基类指针销毁时，调用正确的析构函数
        virtual ~Socket() {}


        /*-----------------------【纯虚函数】-----------------------*/
        //1.创建Socket（子类需实现具体协议的Socket创建，如TCP/UDP）
        virtual void SocketOrDie() = 0;

        //2.绑定端口（子类实现具体的绑定逻辑）
        virtual void BindOrDie(uint16_t port) = 0;

        //3.监听端口（仅TCP需要，UDP无需实现）
        virtual void ListenOrDie(int backlog) = 0;

        //4.接收连接（仅TCP需要，返回新的Socket对象处理客户端通信） 
        virtual std::shared_ptr<Socket> Accept(InetAddr *client) = 0; // 返回值：std::shared_ptr<Socket> - 新的Socket对象（智能指针自动管理资源）

        //5.关闭Socket（释放文件描述符）
        virtual void Close() = 0;

        /*-----------------------【成员函数】-----------------------*/
        //1.TCP Socket创建的固定流程（模板方法模式） ---> 封装「创建Socket→绑定端口→监听端口」的步骤，子类只需实现具体步骤
        void BuildTcpSocketMethod(uint16_t port, int backlog = gbacklog)
        {
            SocketOrDie();        // 步骤1：创建Socket
            BindOrDie(port);      // 步骤2：绑定端口
            ListenOrDie(backlog); // 步骤3：监听端口
        }

        // //2.UDP Socket创建流程（UDP无需listen步骤）
        // void BuildUdpSocketMethod()
        // {
        //     SocketOrDie();    // 步骤1：创建Socket
        //     BindOrDie();      // 步骤2：绑定端口（UDP也需要绑定端口）
        // }
    };


    /*------------------------------------------TCP Socket子类------------------------------------------*/
    // TCP Socket子类：实现基类的纯虚函数，处理TCP协议的具体操作
    class TcpSocket : public Socket
    {
    private:
        //1.Socket文件描述符
        int _sockfd;  
                    
    public:
        /*-----------------------【构造析构】-----------------------*/
        //1.“默认构造” ---> 初始化Socket描述符为无效值）
        TcpSocket():_sockfd(defaultfd)
        {}

        //2.“构造函数” ---> 通过已有的文件描述符创建TcpSocket（用于accept返回的客户端Socket）
        TcpSocket(int fd):_sockfd(fd)
        {}

        //3.“析构函数” ---> 空实现（Close方法负责关闭描述符，避免双重释放）
        ~TcpSocket() {}

        /*-----------------------【函数重写】-----------------------*/
        //1.创建TCP Socket
        void SocketOrDie() override
        {
            //1.1：调用系统socket函数 ---> AF_INET(IPv4) + SOCK_STREAM(TCP) + 0(默认协议)
            _sockfd = ::socket(AF_INET, SOCK_STREAM, 0);
            if (_sockfd < 0)  
            {
                LOG(LogLevel::FATAL) << "socket error";  
                exit(SOCKET_ERR); 
            }

            //1.2：打印创建成功日志
            LOG(LogLevel::INFO) << "socket success";  
        }

        //2.绑定TCP端口
        void BindOrDie(uint16_t port) override
        {
            //2.1：创建本地地址对象（绑定所有网卡，端口为指定值）
            InetAddr localaddr(port);
            //2.2：调用系统bind函数 ---> 绑定Socket描述符到本地地址
            int n = ::bind(_sockfd, localaddr.NetAddrPtr(), localaddr.NetAddrLen());
            if (n < 0) 
            {
                LOG(LogLevel::FATAL) << "bind error";  
                exit(BIND_ERR);  
            }

            //2.3：打印绑定成功日志
            LOG(LogLevel::INFO) << "bind success"; 
        }

        //3.监听TCP端口
        void ListenOrDie(int backlog) override
        {
            //3.1：调用系统listen函数 ---> 将Socket设为监听状态，backlog为半连接队列大小
            int n = ::listen(_sockfd, backlog);
            if (n < 0)  
            {
                LOG(LogLevel::FATAL) << "listen error";  
                exit(LISTEN_ERR);
            }

            //3.2：打印监听成功日志
            LOG(LogLevel::INFO) << "listen success";  
        }

        //4.接收TCP连接
        std::shared_ptr<Socket> Accept(InetAddr *client) override  //注意：std::shared_ptr<Socket> - 新的TcpSocket对象（用于与客户端通信）
        {
            //4.1：存储客户端地址的原始结构体
            struct sockaddr_in peer; 
            //4.2：地址结构体大小
            socklen_t len = sizeof(peer);  
            //4.3：调用系统accept函数 ---> 阻塞等待客户端连接，成功返回客户端Socket描述符
            int fd = ::accept(_sockfd, CONV(peer), &len);
            if(fd < 0)  
            {
                LOG(LogLevel::WARNING) << "accept warning ..."; 
                return nullptr; 
            }

            //4.4：将客户端地址结构体转换为InetAddr对象（供上层使用）
            client->SetAddr(peer);

            //4.5：创建新的TcpSocket对象（用智能指针管理，自动释放资源）
            return std::make_shared<TcpSocket>(fd);
        }

        //5.关闭TCP Socket
        void Close() override
        {
            //5.1：确保描述符有效后调用系统close函数释放文件描述符
            if(_sockfd >= 0) 
            {
                ::close(_sockfd);  
            } 

            //5.2：重置为无效值，避免重复关闭
            _sockfd = defaultfd;  
        }
    };


    // /*------------------------------------------UDP Socket子类------------------------------------------*/
    // // UDP Socket子类（预留扩展，实现UDP协议的Socket操作）
    // class UdpSocket : public Socket
    // {
    // };
}