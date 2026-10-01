#pragma once  

#include <iostream>              
#include <memory>               

#include "Epoller.hpp"            // Epoll事件模型封装（定义EPOLLIN/EPOLLET等事件宏）
#include "Socket.hpp"             // 套接字封装类（TcpSocket/InetAddr，封装socket/accept等系统调用）
#include "Common.hpp"             // 通用常量（如defaultport默认端口、ACCEPT_*错误码）
#include "Connection.hpp"         // 连接基类（定义Recver/Sender等虚函数接口）
#include "Channel.hpp"            // 客户端连接通道类（处理客户端IO的具体实现）
using namespace SocketModule;    

// Listener类：专门负责监听端口、接受新客户端连接的模块
class Listener : public Connection // 核心职责：初始化监听套接字、处理新连接就绪事件、创建客户端Channel并交给Reactor管理
{
private:
    //1.监听端口号
    //2.监听套接字对象（智能指针自动释放资源）

    int _port;                            
    std::unique_ptr<Socket> _listensock; 

public:
    /*--------------------------------------------【构造&析构】--------------------------------------------*/
    //1.“构造函数” ---> 初始化监听端口、创建并配置监听套接字
    // 参数：port - 监听端口（默认使用Common.hpp中定义的defaultport）
    Listener(int port = defaultport)
    :_port(port),                               // 初始化端口 
    _listensock(std::make_unique<TcpSocket>())  // 创建监听套接字对象
    {
        //1.初始化监听套接字 ---> 内部封装socket()/bind()/listen()系统调用，完成监听准备
        _listensock->BuildTcpServerSocketMethod(_port);
        
        //2.设置监听fd关注的事件 ---> EPOLLIN（读事件，新连接到来） + EPOLLET（边缘触发）
        SetEvent(EPOLLIN | EPOLLET); 
        
        //3.将监听fd设置为非阻塞模式 ---> 适配ET模式，确保accept能一次性处理所有待接受的新连接
        SetNonBlock(_listensock->Fd());
    }

    //2.“析构函数” ---> 智能指针自动释放_listensock，无需手动清理
    ~Listener()
    {}


    /*--------------------------------------------【函数重写】--------------------------------------------*/
    //1.处理监听fd的读事件（新连接就绪）---> 在ET+非阻塞模式下，循环accept所有待接受的新连接
    void Recver() override
    {
        //1.存储客户端地址信息（IP+端口）
        InetAddr client;
        
        //2.ET模式下，事件仅触发一次，需循环accept所有待处理的新连接
        // 监听fd已设为非阻塞，无新连接时accept会立即返回，避免阻塞
        while(true)
        {
            //1.尝试接受一个新连接：封装的accept调用，返回值包含不同状态码
            int sockfd = _listensock->Accept(&client);
            
            //2.根据accept返回值处理不同情况
            if(sockfd == ACCEPT_ERR)           // 致命错误（如fd非法），终止循环
                break;  
            else if(sockfd == ACCEPT_CONTINUE) // 临时无新连接（如EAGAIN），继续循环尝试
                continue;  
            else if(sockfd == ACCEPT_DONE)     // 已处理完所有新连接，正常终止循环
                break;  
            else
            {
                //1）成功获取合法的客户端fd ---> 创建Channel对象管理该客户端连接
                std::shared_ptr<Connection> conn = std::make_shared<Channel>(sockfd, client);
                
                //2）设置客户端fd的事件 ---> EPOLLIN（读事件） + EPOLLET（边缘触发）
                conn->SetEvent(EPOLLIN|EPOLLET);
                
                //3）若注册了数据处理回调函数，传递给客户端连接（处理客户端发送的数据）
                if(_handler != nullptr)
                {
                    conn->RegisterHandler(_handler);
                }

                //4）获取当前Listener所属的Reactor，将新客户端连接添加到Reactor管理 ---> nReactor会将该fd注册到epoll，并接管后续IO事件处理
                GetOwner()->AddConnection(conn);
            }
        }
    }

    //2.Listener无需处理写事件，空实现
    void Sender() override
    {}

    //3.Listener异常处理逻辑，暂空实现（可扩展）
    void Excepter() override
    {}

    //4.返回监听套接字的fd，供Reactor/Epoll调用
    int GetSockFd() override
    {
        return _listensock->Fd();
    }

};