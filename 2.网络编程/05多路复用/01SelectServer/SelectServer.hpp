#pragma once       

#include <iostream>       
#include <memory>         
#include <unistd.h>       

#include "Socket.hpp"     
#include "Log.hpp"        
using namespace SocketModule;  
using namespace LogModule;     


// SelectServer类：基于I/O多路复用select实现的TCP服务器
class SelectServer // 核心能力：同时监听多个文件描述符（监听套接字+客户端套接字）的读事件，实现并发处理
{
    // 静态常量定义
    const static int size = sizeof(fd_set) * 8;  // select支持的最大文件描述符数量（fd_set的位数）
    const static int defaultfd = -1;             // 标记数组中未使用的fd位置（初始值/释放后的值）

private:   
    //1.监听套接字（智能指针自动管理生命周期）
    //2.服务器运行状态标记（true=运行，false=停止）
    //3.fd管理数组 ---> 存储所有需要select监听的fd（监听fd+客户端fd）

    std::unique_ptr<Socket> _listensock;  
    bool _isrunning;                      
    int _fd_array[size];                  

public:
    /*------------------------------------------------【构造&析构】------------------------------------------------*/
    //1.“构造函数” ---> 初始化监听套接字并准备fd管理数组
    SelectServer(int port) : _listensock(std::make_unique<TcpSocket>()), _isrunning(false)
    {
        //1.初始化监听套接字 ---> 创建、绑定端口、开始监听
        _listensock->BuildTcpServerSocketMethod(port);
        
        //2.初始化fd管理数组 ---> 所有位置设为defaultfd（-1），表示未使用
        for (int i = 0; i < size; i++)
        {
            _fd_array[i] = defaultfd;
        }

        //3.将监听套接字的fd放入数组第一个位置（select需要监听新连接事件）
        _fd_array[0] = _listensock->Fd();
    }

    //2.“析构函数”
    ~SelectServer() // 智能指针会自动释放_listensock，无需手动close监听fd
    {}

    /*------------------------------------------------【启动&停止】------------------------------------------------*/
    //3.“启动服务器” ---> 进入select事件循环，持续监听fd事件
    void Start()
    {
        //1.标记服务器运行状态
        _isrunning = true;  

        //2.主循环：服务器运行时持续执行
        while (_isrunning) 
        {
            // ========== 1. 初始化select的读事件集合 ==========
            //1.1：定义读事件文件描述符集合
            fd_set rfds;     //注意：select只关心该集合中的fd读事件
 
            //1.2：清空集合
            FD_ZERO(&rfds);  //注意：必须每次循环重置，因为select会修改集合
            
            //1.3：记录集合中最大的fd（select的第一个参数需要）
            int maxfd = defaultfd;  

            //1.4：遍历fd管理数组，将有效fd加入读事件集合
            for (int i = 0; i < size; i++)
            {
                //第一步：跳过未使用的位置
                if (_fd_array[i] == defaultfd) 
                {
                    continue;
                }
                
                //第二步：将有效fd加入读事件集合
                FD_SET(_fd_array[i], &rfds);    

                //第三步：更新最大fd值
                if (maxfd < _fd_array[i])      
                {
                    maxfd = _fd_array[i];
                }
            }

            PrintFd();  // 调试用：打印当前管理的所有有效fd

            // ========== 2. 调用select等待事件就绪 ==========
            int n = select(maxfd + 1, &rfds, nullptr, nullptr, nullptr);

            // ========== 3. 处理select返回结果 ==========
            switch (n)
            {
            case -1:  // select调用失败（如：信号中断、参数错误）
                LOG(LogLevel::ERROR) << "select error";
                break;
            case 0:   // 超时
                LOG(LogLevel::INFO) << "time out...";
                break;
            default:  // 有n个fd事件就绪（读事件）
                LOG(LogLevel::DEBUG) << "有事件就绪了..., n : " << n;
                Dispatcher(rfds);  
                break;
            }
        }

        //3.服务器停止，重置运行状态
        _isrunning = false;  
    }

    //2.“停止服务器” ---> 修改运行状态，退出主循环
    void Stop()
    {
        _isrunning = false;
    }

    /*------------------------------------------------【辅助函数】------------------------------------------------*/    
    //1.“事件派发器” ---> 遍历就绪的fd，区分是新连接事件还是客户端数据事件
    void Dispatcher(fd_set &rfds)
    {
        //1.遍历fd管理数组，检查每个有效fd是否在就绪集合中
        for (int i = 0; i < size; i++)
        {
            //1.1：跳过未使用的位置
            if (_fd_array[i] == defaultfd)  
            {
                continue;
            }

            //1.2：检查当前fd是否在就绪集合中（读事件就绪）
            if (FD_ISSET(_fd_array[i], &rfds))
            {
                //情况一：监听套接字就绪 → 新连接
                if (_fd_array[i] == _listensock->Fd())
                {
                    Accepter();  
                }

                //情况二：客户端套接字就绪 → 数据可读
                else
                {
                    Recver(_fd_array[i], i); 
                }
            }
        }
    }

    //2.“连接管理器” ---> 处理监听套接字的新连接事件
    void Accepter()
    {
        //1.存储客户端地址信息（IP+端口）
        InetAddr client;

        //2.接受新连接 （此时select已确认监听fd就绪，accept不会阻塞）
        int sockfd = _listensock->Accept(&client); 
        
        //3.新连接接受成功
        if (sockfd >= 0)  
        {
            //3.1：打印
            LOG(LogLevel::INFO) << "get a new link, sockfd: "
                                << sockfd << ", client is: " << client.StringAddr();
            
            //3.2：寻找fd管理数组中未使用的位置，存放新客户端fd
            int pos = 0;
            for (; pos < size; pos++)
            {
                if (_fd_array[pos] == defaultfd)
                {
                    break;
                }
            }

            //3.3：根据数组的使用情况做出相应的处理操作
            //情况一：数组已满，无法接受新连接
            if (pos == size)
            {
                LOG(LogLevel::WARNING) << "select server full";
                close(sockfd);  // 关闭新连接fd，避免资源泄漏
            }

            //情况二：数组有空闲位置，托管新fd给select
            else  
            {
                _fd_array[pos] = sockfd;
            }
        }
    }

    //3.“IO处理器” ---> 处理客户端套接字的数据读取事件
    void Recver(int fd, int pos)
    {
        //1.定义数据接收缓冲区
        char buffer[1024]; 

        //2.读取客户端数据（此时select已确认fd就绪，recv不会阻塞）
        ssize_t n = recv(fd, buffer, sizeof(buffer)-1, 0);   // 注意：sizeof(buffer)-1 预留末尾存'\0'，避免字符串越界
        
        //3.根据读取的情况做出相应的处理动作
        //情况一：成功读取到数据
        if (n > 0) 
        {
            buffer[n] = 0; 
            std::cout << "client say@ "<< buffer << std::endl;
        }

        //情况二：客户端关闭连接（recv返回0表示EOF）
        else if (n == 0)  
        {
            LOG(LogLevel::INFO) << "client quit...";

            //1.将数组中该位置标记为未使用（select不再监听）
            _fd_array[pos] = defaultfd;
            //2.关闭fd，释放系统资源
            close(fd);
        }

        //情况三：读取失败（如连接异常、中断等）
        else  
        {
            LOG(LogLevel::ERROR) << "recv error";

            //1.将数组中该位置标记为未使用
            _fd_array[pos] = defaultfd;
            //2.关闭fd，释放系统资源
            close(fd);
        }
    }

    //4.“调试辅助函数” ---> 打印当前管理的所有有效fd
    void PrintFd()
    {
        std::cout << "_fd_array[]: ";
        for (int i = 0; i < size; i++)
        {
            if (_fd_array[i] == defaultfd)
            {
                continue;
            }
            std::cout << _fd_array[i] << " ";
        }
        std::cout << "\r\n";
    }


};