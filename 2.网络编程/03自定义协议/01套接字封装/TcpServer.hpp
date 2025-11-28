#include <iostream>                  // 标准输入输出
#include <memory>                    // 智能指针（std::unique_ptr/std::shared_ptr）
#include <sys/wait.h>                // 进程等待函数（waitpid，用于回收子进程）
#include <functional>                // 函数包装器（std::function，定义回调函数类型）

#include "Socket.hpp"                // 包含Socket模块头文件（提供TcpSocket、InetAddr等类）


//1.引入命令空间
using namespace SocketModule; // 引入Socket模块命名空间（简化TcpSocket、Socket的使用）
using namespace LogModule;    // 引入日志模块命名空间（简化LOG宏调用）

//2.定义IO服务回调函数类型 ---> 封装客户端通信的业务逻辑
using ioservice_t = std::function<void(std::shared_ptr<Socket> &sock, InetAddr &client)>;
/*介绍说明：
*   1. 参数1：std::shared_ptr<Socket> &sock - 客户端Socket对象（智能指针管理通信句柄）
*   2. 参数2：InetAddr &client - 客户端地址对象（包含IP和端口，标识请求来源）
* 作用：服务器接收连接后，通过该回调函数处理与客户端的IO通信（如：数据收发、业务处理）
*/

// TCP服务器类 ---> 封装TCP服务器的核心逻辑（连接接收、子进程处理）
class TcpServer
{
private:
    //1.监听Socket对象（智能指针管理，自动释放）
    //2.服务器监听端口号
    std::unique_ptr<Socket> _listensockptr;  
    uint16_t _port;                         

    //3.服务器运行状态标志（true=运行中，false=已停止）
    //4.IO通信回调函数（业务逻辑载体）
    bool _isrunning;                        
    ioservice_t _service;                   

public:
    //1.“构造函数” ---> 初始化服务器监听端口和IO回调函数
    TcpServer(uint16_t port, ioservice_t service) 
        : _listensockptr(std::make_unique<TcpSocket>()),   // 创建监听Socket对象
          _port(port),                                     // 初始化监听端口
          _isrunning(false),                               // 初始化服务器运行状态为"未运行"
          _service(service)                                // 绑定IO通信回调函数
    {
        //1.初始化TCP Socket：自动执行「创建Socket→绑定端口→监听端口」流程
        _listensockptr->BuildTcpSocketMethod(_port);
    }

    
    //2.“析构函数” ---> 空实现（智能指针自动释放_listensockptr，无需手动处理）
    ~TcpServer() {}

    //3.“启动服务器” ---> 进入事件循环，持续接收客户端连接并处理
    void Start()
    {
        //1.标记服务器为"运行中"状态
        _isrunning = true; 

        //2.事件循环 ---> 只要服务器运行，就持续接收客户端连接
        while(_isrunning)
        {
            /* ===================== 第一步：客户端建立连接 ===================== */
            //2.1：存储客户端地址信息的对象
            InetAddr client;  

            //2.2：接收客户端连接
            auto sock = _listensockptr->Accept(&client);
            if(sock == nullptr)  
            {
                continue;  // 跳过当前循环，继续等待下一个连接
            }
            LOG(LogLevel::DEBUG) << "accept success ...";  

            /* ===================== 第二步：多进程处理连接 ===================== */
            //3.为每个客户端连接创建子进程，子进程处理与客户端的IO通信
            pid_t id = fork();  
            if(id < 0)  
            {
                LOG(LogLevel::FATAL) << "fork error ...";  
                exit(FORK_ERR);  
            }

            //4.子进程逻辑
            else if(id == 0) 
            {
                //4.1：子进程不需要监听Socket，关闭以释放资源（避免文件描述符泄漏）
                _listensockptr->Close();

                //4.2：二次fork创建孙子进程 ---> 子进程退出后，父进程无需waitpid，避免僵尸进程
                if(fork() > 0)
                {
                    exit(OK);  // 子进程退出，孙子进程成为孤儿进程
                }

                //4.3：孙子进程执行IO通信逻辑 ---> 调用回调函数处理客户端请求
                _service(sock, client);

                //4.4：孙子进程处理完请求后退出
                exit(OK);  
            }
            //4.父进程逻辑
            else 
            {
                //4.1：父进程不需要客户端Socket，关闭以释放资源
                sock->Close();

                //4.2：等待子进程退出（子进程二次fork后立即退出，不会阻塞父进程）
                pid_t rid = ::waitpid(id, nullptr, 0);
                (void)rid;  // 消除未使用变量警告
            }
        }

        //3.服务器退出循环后，标记为"未运行"状态
        _isrunning = false;  
    }

};