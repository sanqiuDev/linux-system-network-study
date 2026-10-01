#pragma once  

#include <iostream>       
#include <memory>       
#include <unistd.h>     
#include <sys/poll.h>     // poll多路复用相关头文件 ---> 定义pollfd结构体、poll函数

#include "Socket.hpp"  
#include "Log.hpp"       
using namespace SocketModule;  
using namespace LogModule;     

// PollServer类：基于I/O多路复用poll实现的TCP服务器
class PollServer // 核心能力：相比select，poll使用结构体数组管理fd，无fd数量的位图限制（仅受数组大小限制），支持更多并发连接
{
    // 静态常量定义
    const static int size = 4096;       // poll管理的最大fd数量（可根据需求调整，远大于select的1024限制）
    const static int defaultfd = -1;    // 标记数组中未使用的fd位置（初始值/释放后的值）

private:
    //1.监听套接字（智能指针自动管理生命周期）
    //2.服务器运行状态标记（true=运行，false=停止）
    //3.pollfd数组：管理所有需要监听的fd及事件

    std::unique_ptr<Socket> _listensock; 
    bool _isrunning;                     
    struct pollfd _fds[size];      
    
    //struct pollfd *_fds;  //注意：根据需要可以将上面数组修改成动态数组  按需扩容，减少内存占用


public:
    /*-------------------------------------------------【构造&析构】-------------------------------------------------*/
    //1.“构造函数” ---> 初始化监听套接字并准备pollfd管理数组
    PollServer(int port) : _listensock(std::make_unique<TcpSocket>()), _isrunning(false)
    {
        //1.初始化监听套接字
        _listensock->BuildTcpServerSocketMethod(port);
        
        //2.初始化pollfd数组 ---> 所有元素置为默认状态（未使用）
        for (int i = 0; i < size; i++)
        {
            _fds[i].fd = defaultfd;      // fd设为-1，表示该位置未使用
            _fds[i].events = 0;          // 监听的事件集合初始化为空
            _fds[i].revents = 0;         // 就绪的事件集合初始化为空（由内核填充）
        }

        //3.将监听套接字加入poll管理数组 ---> 监听读事件（新连接到来）
        _fds[0].fd = _listensock->Fd();  // 监听fd放入数组第一个位置
        _fds[0].events = POLLIN;         // 关注监听fd的读事件（POLLIN：数据可读/新连接到来）
    }

    //2.“析构函数” ---> 智能指针会自动释放_listensock，无需手动close监听fd
    ~PollServer()
    {
        //注意：若需要优雅退出，可遍历数组关闭所有客户端fd
    }

    /*-------------------------------------------------【启动&停止】-------------------------------------------------*/
    //1.“启动服务器” ---> 进入poll事件循环，持续监听fd事件
    void Start()
    {
        //1.设置poll超时时间
        /*  1. -1表示永久阻塞 （直到有事件就绪）
        *   2. 0表示非阻塞
        *   3. >0表示毫秒级超时
        */
        int timeout = -1;            
        
        //2.标记服务器运行状态
        _isrunning = true;               

        //3.主循环：服务器运行时持续执行
        while (_isrunning)               
        {
            PrintFd();  // 调试用：打印当前管理的所有有效fd

            // ========== 调用poll等待事件就绪 ==========
            int n = poll(_fds, size, timeout);

            // ========== 处理poll返回结果 ==========
            switch (n)
            {
            case -1:  // poll调用失败（如信号中断、参数错误）
                LOG(LogLevel::ERROR) << "poll error";
                break;
            case 0:   // 超时（本示例timeout=-1，此分支不会触发）
                LOG(LogLevel::INFO) << "poll time out...";
                break;
            default:  // 有n个fd事件就绪
                LOG(LogLevel::DEBUG) << "有事件就绪了..., n : " << n;
                Dispatcher();  // 事件派发：处理就绪的fd
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

    /*-------------------------------------------------【辅助函数】-------------------------------------------------*/
    //1.“事件派发器” ---> 遍历pollfd数组，处理就绪的fd事件
    void Dispatcher() //注意：无需传入就绪集合，poll直接通过revents字段标记每个fd的就绪事件
    {
        //1.遍历pollfd数组，检查每个有效fd的就绪事件
        for (int i = 0; i < size; i++)
        {
            //1.1：跳过未使用的位置
            if (_fds[i].fd == defaultfd)  
            {
                continue;
            }
        
            //1.2：检查当前fd是否有读事件就绪（POLLIN：新连接/客户端数据可读）
            if (_fds[i].revents & POLLIN)
            {
                //情况一：监听套接字就绪 → 新连接
                if (_fds[i].fd == _listensock->Fd())
                {
                    Accepter();  // 处理新连接
                }

                //情况二：客户端套接字就绪 → 数据可读
                else
                {
                    Recver(i);   // 处理客户端数据读取
                }
            }

            //1.3：可扩展：处理写事件就绪（POLLOUT），用于主动给客户端发送数据
            // else if(_fds[i].revents & POLLOUT)
            // {}
        }
    }

    //2.“连接管理器” ---> 处理监听套接字的新连接事件
    void Accepter()
    {
        //1.存储客户端地址信息（IP+端口）
        InetAddr client;  

        //2.接受新连接（此时poll已确认监听fd就绪，accept不会阻塞）
        int sockfd = _listensock->Accept(&client);
        
        //3.新连接接受成功
        if (sockfd >= 0)  
        {
            //3.1：打印
            LOG(LogLevel::INFO) 
            << "get a new link, sockfd: "  
            << sockfd << ", client is: " << client.StringAddr();
            
            //3.2：寻找pollfd数组中未使用的位置，存放新客户端fd
            int pos = 0;
            for (; pos < size; pos++)
            {
                if (_fds[pos].fd == defaultfd)
                {
                    break;
                }
            }

            //3.3：根据查找的结果做出相应的操作
            //情况一：数组已满，无法接受新连接
            if (pos == size)
            {
                LOG(LogLevel::WARNING) << "poll server full";


                //1）关闭新连接fd，避免资源泄漏
                close(sockfd);  
            }

            //情况二：数组有空闲位置，托管新fd给poll
            else  
            {
                //1）存放客户端fd
                _fds[pos].fd = sockfd;      

                //2）关注该fd的读事件（客户端数据可读）
                _fds[pos].events = POLLIN; 

                //3）重置就绪事件（由内核填充）
                _fds[pos].revents = 0;    
            }
        }
    }

    //3.“IO处理器” ---> 处理客户端套接字的数据读取事件
    void Recver(int pos)
    {
        //1.数据接收缓冲区
        char buffer[1024];  

        //2.读取客户端数据（此时poll已确认fd就绪，recv不会阻塞）
        ssize_t n = recv(_fds[pos].fd, buffer, sizeof(buffer) - 1, 0); //注意：sizeof(buffer)-1 预留末尾存'\0'，避免字符串越界
        
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

            //1.关闭fd释放系统资源
            close(_fds[pos].fd);
            //2.重置pollfd数组对应位置（标记为未使用，poll不再监听）
            _fds[pos].fd = defaultfd;
            _fds[pos].events = 0;
            _fds[pos].revents = 0;
        }

        //情况三：读取失败（如连接异常、中断等）
        else 
        {
            LOG(LogLevel::ERROR) << "recv error";

            //1.关闭fd释放系统资源
            close(_fds[pos].fd);
            //2.重置pollfd数组对应位置
            _fds[pos].fd = defaultfd;
            _fds[pos].events = 0;
            _fds[pos].revents = 0;
        }
    }

    //4.“调试辅助函数” ---> 打印当前管理的所有有效fd
    void PrintFd()
    {
        std::cout << "_fds[]: ";
        for (int i = 0; i < size; i++)
        {
            if (_fds[i].fd == defaultfd)
                continue;
            std::cout << _fds[i].fd << " ";
        }
        std::cout << "\r\n";
    }
};