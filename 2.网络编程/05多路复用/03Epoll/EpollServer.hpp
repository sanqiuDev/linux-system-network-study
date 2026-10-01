#pragma once 

#include <iostream>  
#include <memory>       
#include <unistd.h>       // POSIX系统调用（close、read/recv等）
#include <sys/epoll.h>    // epoll多路复用相关头文件（定义epoll_create/epoll_ctl/epoll_wait等）

#include "Socket.hpp"     
#include "Log.hpp"        
using namespace SocketModule;  
using namespace LogModule;     

// EpollServer类：基于Linux特有的epoll I/O多路复用实现的高性能TCP服务器
class EpollServer // 核心优势：相比select/poll，epoll采用事件驱动+红黑树管理fd，无需遍历全量fd，高并发下性能优势显著
{ 
    // 静态常量定义
    const static int size = 64;          // epoll_wait单次最多处理的就绪事件数量（可根据需求调整）
    const static int defaultfd = -1;     // 无效fd标记（epoll句柄/套接字fd的初始值）

private:
    //1.监听套接字（智能指针自动管理生命周期）
    //2.服务器运行状态标记（true=运行，false=停止）
    //3.epoll实例句柄（内核级事件模型的标识）
    //4.就绪事件数组（存储epoll_wait返回的就绪事件）

    std::unique_ptr<Socket> _listensock; 
    bool _isrunning;                     
    int _epfd;                            
    struct epoll_event _revs[size];     

    
public:
    /*--------------------------------------------【构造&析构】--------------------------------------------*/
    //1.“构造函数” ---> 初始化监听套接字、创建epoll模型并将监听fd加入epoll管理
    EpollServer(int port) : _listensock(std::make_unique<TcpSocket>()), _isrunning(false), _epfd(defaultfd)
    {
        //1.初始化监听套接字：创建TCP套接字、绑定端口、开始监听（内部已封装）
        _listensock->BuildTcpServerSocketMethod(port); 

        //2.创建epoll实例（内核级事件模型）
        _epfd = epoll_create(256);
        if (_epfd < 0)  // epoll实例创建失败（如系统资源不足）
        {
            LOG(LogLevel::FATAL) << "epoll_create error";  
            exit(EPOLL_CREATE_ERR); 
        }
        LOG(LogLevel::INFO) << "epoll_create success: " << _epfd; 

        //3.将监听套接字注册到epoll模型中，关注其读事件（新连接到来）
        //3.1：创建出描述“需要监听的fd”和“事件”的结构体
        struct epoll_event ev; 
        //3.2：关注读事件 ---> EPOLLIN表示fd可读（监听fd可读=新连接到来） 
        ev.events = EPOLLIN;    
        //3.3：绑定监听fd到事件结构体 ---> data字段可存储自定义数据，此处暂存fd（后续可扩展为自定义结构体）
        ev.data.fd = _listensock->Fd(); 
     
       
        //4.向epoll实例中添加监听fd及关注的事件
        int n = epoll_ctl(_epfd, EPOLL_CTL_ADD, _listensock->Fd(), &ev);  // 参数说明：_epfd(epoll句柄)、EPOLL_CTL_ADD(添加操作)、监听fd、事件结构体
        if (n < 0) 
        {
            LOG(LogLevel::FATAL) << "add listensockfd failed"; 
            exit(EPOLL_CTL_ERR); 
        }
    }

    //2.“析构函数” ---> 释放epoll实例和监听套接字资源
    ~EpollServer()
    {
        //1.关闭监听套接字（智能指针辅助，双重保障）
        _listensock->Close();  

        //2.若epoll实例有效，关闭epoll句柄
        if (_epfd > 0)      
        {
            close(_epfd);
        }
    }

    /*--------------------------------------------【启动&停止】--------------------------------------------*/
    //1.“启动服务器” ---> 进入epoll事件循环，持续监听并处理fd事件
    void Start()
    {
        //1.epoll_wait超时时间 ---> -1=永久阻塞（直到有事件就绪），0=非阻塞，>0=毫秒级超时
        int timeout = -1;        
        
        //2.标记服务器运行状态
        _isrunning = true;       

        //3.主循环：服务器运行时持续执行
        while (_isrunning)             
        {
            // ========== 调用epoll_wait等待事件就绪 ==========
            int n = epoll_wait(_epfd, _revs, size, timeout);  // 参数说明：_epfd：epoll句柄、 _revs：输出参数、 size：_revs数组的最大容量（单次最多处理size个就绪事件）、timeout：超时时间
            
            // ========== 处理epoll_wait返回结果 ==========
            switch (n)
            {
            case 0:  // 超时
                LOG(LogLevel::DEBUG) << "timeout...";
                break;
            case -1: // epoll_wait调用失败（如：信号中断）
                LOG(LogLevel::ERROR) << "epoll error";
                break;
            default: // 有n个事件就绪，调用派发器处理
                Dispatcher(n);
                break;
            }
        }

        //4.服务器停止，重置运行状态
        _isrunning = false;  
    }

    //2.“停止服务器” ---> 修改运行状态，退出主循环
    void Stop()
    {
        _isrunning = false;
    }


    /*--------------------------------------------【辅助函数】--------------------------------------------*/
    //1.“事件派发器” ---> 处理epoll_wait返回的就绪事件
    void Dispatcher(int rnum)
    {
        //1.LT: 水平触发模式--epoll默认（只要数据未读完，会持续触发事件）
        LOG(LogLevel::DEBUG) << "event ready ..."; 

        //2.仅遍历就绪的事件（无需遍历全量fd，epoll核心优势）
        for (int i = 0; i < rnum; i++)
        {
            //2.1：获取就绪事件对应的fd
            int sockfd = _revs[i].data.fd;    

            //2.2：获取就绪的事件类型
            uint32_t revent = _revs[i].events;
            
            //2.3：读事件就绪（新连接/客户端数据可读）
            if (revent & EPOLLIN)  
            { 
                //情况一：监听fd就绪 → 新连接
                if (sockfd == _listensock->Fd())
                {
                    Accepter();  // 处理新连接
                }
                //情况二：客户端fd就绪 → 数据可读
                else
                {
                    Recver(sockfd);  // 处理客户端数据读取
                }
            }
            
            //2.4：可扩展：处理写事件就绪（EPOLLOUT），用于主动给客户端发送数据
            // if(_revs[i].events & EPOLLOUT)
            // {
            // }
        }
    }

    //2.“连接管理器” ---> 处理监听套接字的新连接事件
    void Accepter()
    {
        //1.存储客户端地址信息（IP+端口）
        InetAddr client;  

        //2.接受新连接：epoll已确认监听fd读就绪，accept不会阻塞
        int sockfd = _listensock->Accept(&client);
        
        //3.新连接接受成功
        if (sockfd >= 0)  
        {
            //3.1：打印
            LOG(LogLevel::INFO) 
            << "get a new link, sockfd: "
            << sockfd << ", client is: " 
            << client.StringAddr();
            
            
            //3.2：将新客户端fd注册到epoll模型，关注其读事件（数据可读时触发）
            //1）创建描述结构体
            struct epoll_event ev;   //注意：不能直接recv：客户端可能未发送数据，recv会阻塞

            //2）关注客户端fd的读事件
            ev.events = EPOLLIN;   
            
            //3）绑定客户端fd到事件结构体
            ev.data.fd = sockfd;   
            
            //3.3：向epoll实例中添加客户端fd
            int n = epoll_ctl(_epfd, EPOLL_CTL_ADD, sockfd, &ev);
            if (n < 0)  
            {
                LOG(LogLevel::WARNING) << "add client sockfd failed: " << sockfd;
            }
            else
            {
                LOG(LogLevel::INFO) << "epoll_ctl add sockfd success: " << sockfd;
            }
        }
    }

    //3.“IO处理器” ---> 处理客户端套接字的数据读取事件
    void Recver(int sockfd)
    {
        //1.定义数据接收缓冲区
        char buffer[1024]; 

        //2.读取客户端数据：epoll已确认fd读就绪，recv不会阻塞
        ssize_t n = recv(sockfd, buffer, sizeof(buffer) - 1, 0);  // 注意：sizeof(buffer)-1 预留末尾存'\0'，避免字符串越界
        
        //3.根据读取的情况做出相应的处理动作
        //情况一：成功读取到数据
        if (n > 0) 
        {
            buffer[n] = 0;  
            std::cout << "client say@ " << buffer << std::endl;
        }

        //情况二：客户端关闭连接（recv返回0表示EOF）
        else if (n == 0)  
        {
            LOG(LogLevel::INFO) << "client quit...";

            //1.先从epoll中移除fd，再关闭fd（避免epoll监听已关闭的fd）
            int m = epoll_ctl(_epfd, EPOLL_CTL_DEL, sockfd, nullptr);
            if(m == 0) 
            {
                LOG(LogLevel::INFO) << "epoll_ctl remove sockfd success: " << sockfd;
            }

            //2.关闭fd，释放系统资源
            close(sockfd);   
        }

        //情况三：读取失败（如连接异常、中断等，recv返回-1）
        else 
        {
            LOG(LogLevel::ERROR) << "recv error, sockfd: " << sockfd;


            //1.先移除epoll监听，再关闭fd
            int ret = epoll_ctl(_epfd, EPOLL_CTL_DEL, sockfd, nullptr);
            if(ret == 0)
            {
                LOG(LogLevel::INFO) << "epoll_ctl remove sockfd success: " << sockfd;
            }

            //2.关闭fd，释放系统资源
            close(sockfd);  
        }
    }

};