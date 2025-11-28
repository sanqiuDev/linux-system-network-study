#pragma once 

#include <sys/wait.h>        // 包含“进程等待相关函数” ---> 如waitpid，用于多进程版本回收子进程
#include <signal.h>          // 包含“信号相关函数” ---> 如signal，用于忽略SIGCHLD信号
#include <pthread.h>         // 包含“POSIX线程库头文件” ---> 线程创建、分离、销毁等

#include "Common.hpp"        // 包含“通用工具头文件” ---> 错误码、常用宏、类型定义等
#include "Log.hpp"           // 包含“日志模块头文件” ---> 打印运行日志、调试信息
#include "InetAddr.hpp"      // 包含“网络地址封装类头文件” ---> 封装sockaddr_in，简化IP/端口操作
#include "ThreadPool.hpp"    // 包含“线程池模块头文件” ---> 预留线程池版本支持，当前未启用


//1.引入命名空间
using namespace LogModule;        // 日志模块命名空间，简化LOG宏的调用
using namespace ThreadPoolModule; // 线程池模块命名空间，预留线程池版本使用

//2.
using task_t = std::function<void()>;

//3.常量定义
const static int defaultsockfd = -1; // 默认监听套接字描述符（无效值，用于初始化）
const static int backlog = 8;        // listen函数的backlog参数（半连接队列大小，即最大等待连接数）


// TCP服务器核心类 ---> 继承NoCopy类，禁止拷贝构造和赋值（服务器对象通常全局唯一，避免拷贝导致资源冲突）
//说明：TcpServer类公有继承自NoCopy类，这样的话TcpServer要进行拷贝的话，就必须先将NoCopy先进行拷贝，但是基类是禁止拷贝的
//好处：就是将来使用UDP服务器的时候，要是也想实现服务器对象的全局唯一的话，不需要修改UdpServer内部的代码，只需要让起公有继承NoCopy类就行了
class TcpServer : public NoCopy
{
private:
    //1.监听套接字描述符 ---> 用于接收客户端连接
    //2.服务器监听端口号
    //3.服务器运行状态标志 ---> true=运行中，false=已停止

    int _listensockfd;   
    uint16_t _port;          
    bool _isrunning;         

public:
    //1.“构造函数” ---> 初始化服务器监听端口和业务回调函数
    TcpServer(uint16_t port) 
        :_listensockfd(defaultsockfd), // 初始化监听套接字为无效值
        _port(port),                   // 初始化监听端口
        _isrunning(false)              // 初始化服务器运行状态为"未运行"
    {}

    //2.析构函数” ---> 空实现
    ~TcpServer() { }

  
    //5.“业务处理核心函数” ---> 与单个客户端进行数据交互（长连接模式）
    void Service(int sockfd, InetAddr &peer)
    {
        //1.接收缓冲区 ---> 存储客户端发送的数据（大小1023字节，留1字节存'\0'）
        char buffer[1024];  

        //2.持续与客户端通信（长连接模式，直到客户端断开或出错）
        while (true)
        {
            //2.1：从客户端读取数据（read函数：从套接字读取字节流）
            ssize_t n = read(sockfd, buffer, sizeof(buffer) - 1);
            
            //2.2：读取结果处理
            //情况一：读取成功（n为实际读取的字节数）
            if (n > 0)  
            {
                //1)在数据末尾添加字符串结束符'\0'，转换为C风格字符串（避免打印乱码）
                buffer[n] = 0;  

                //2)打印调试日志 ---> 记录客户端IP、端口及发送的数据
                LOG(LogLevel::DEBUG) << peer.StringAddr() << " #" << buffer;

                //3)直接返回数据
                std::string echo_string = "echo# ";
                echo_string += buffer;

                //4)将响应数据写回客户端（write函数：将字节流写入套接字）
                write(sockfd, echo_string.c_str(), echo_string.size());
            }

            //情况二：客户端正常断开连接（read返回0表示读到文件结束符）
            else if (n == 0)  
            {
                //1)打印调试日志
                LOG(LogLevel::DEBUG) << peer.StringAddr() << " 退出了..."; 

                //2)关闭客户端套接字，释放资源
                close(sockfd);  

                //3)退出循环，结束与该客户端的通信
                break;         
            }

            //情况三：读取错误（n<0，如网络中断、套接字异常）
            else  
            {
                //1)打印
                LOG(LogLevel::DEBUG) << peer.StringAddr() << " 异常..."; 
                //2)关闭
                close(sockfd);  
                //3)退出 
                break;          
            }
        }
    }

    //4.线程数据封装类 ---> 将线程需要的资源（客户端套接字、客户端地址、服务器指针）封装为一个对象，便于传递给线程函数
    class ThreadData
    {
    public:
        /*------------------------【属性】------------------------*/
        //1.客户端连接套接字描述符
        //2.客户端网络地址对象
        //3.TcpServer对象指针（跨线程访问服务器成员方法）

        int sockfd;      
        InetAddr addr;  
        TcpServer *tsvr; 


        /*------------------------【方法】------------------------*/
        //1.“构造函数” ---> 初始化线程所需资源
        ThreadData(int fd, InetAddr &ar, TcpServer *ts) 
            : sockfd(fd), // 客户端连接套接字（用于与客户端收发数据）
            addr(ar),     // 客户端地址对象（标识客户端身份）
            tsvr(ts)      // TcpServer对象指针（用于线程函数中调用服务器的Service方法）
        {}
    };



    //6.“线程入口函数” ---> 多线程版本中，新线程通过该函数执行业务逻辑
    /* 解释：为什么线程入口函数Routine要是静态的？
    *      1.POSIX 线程库（pthread）的线程入口函数，不允许带额外的隐式参数
    *      2.但类的非静态成员函数默认会隐藏一个this指针 —— this指针，用于指向调用该函数的对象实例
    *      3.而这就导致函数签名不匹配，编译器无法识别
    *      4.类的静态成员函数（用static修饰） 有一个关键特性：不属于任何对象实例，因此编译器不会给它添加this指针
    */
    static void *Routine(void *args) 
    {
        //1.设置线程为分离状态
        pthread_detach(pthread_self());
        
        //2.将void*类型的参数转换为ThreadData指针（获取线程所需资源）
        ThreadData *td = static_cast<ThreadData *>(args);
        
        //3.调用服务器的Service方法处理业务（通过ThreadData中的tsvr指针访问非静态成员）
        td->tsvr->Service(td->sockfd, td->addr);
        
        //4.释放ThreadData对象（动态分配的资源，避免内存泄漏）
        delete td;
        
        //5.线程函数返回值（POSIX线程要求返回void*）
        return nullptr;  
    }

    //3.“服务器初始化函数” ---> 完成socket创建、bind绑定、listen监听（TCP服务器启动前三步）
    void Init()
    {
        //1.创建TCP监听套接字（socket函数）
        _listensockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (_listensockfd < 0)  
        {
            LOG(LogLevel::FATAL) << "socket error";  // 打印致命错误日志（程序无法继续运行）
            exit(SOCKET_ERR);                        // 退出程序，SOCKET_ERR为Common.hpp定义的错误码
        }
        LOG(LogLevel::INFO) << "socket success: " << _listensockfd;  // 打印创建成功日志（输出套接字描述符）


        
        //2.绑定端口号（bind函数）
        //2.1：初始化本地地址对象（仅指定端口，IP自动绑定所有网卡）
        InetAddr local(_port);  
        //2.2：将套接字与本地IP和端口绑定
        int n = bind(_listensockfd, local.NetAddrPtr(), local.NetAddrLen());
        if (n < 0)  
        {
            LOG(LogLevel::FATAL) << "bind error";  // 打印致命错误日志
            exit(BIND_ERR);                        // 退出程序，BIND_ERR为绑定错误码
        }
        //2.3：打印绑定成功日志
        LOG(LogLevel::INFO) << "bind success: " << _listensockfd;  



        //3.设置套接字为监听状态（listen函数）---> 开始接收客户端连接
        n = listen(_listensockfd, backlog);
        if (n < 0)  
        {
            LOG(LogLevel::FATAL) << "listen error";  // 打印致命错误日志
            exit(LISTEN_ERR);                        // 退出程序，LISTEN_ERR为监听错误码
        }
        LOG(LogLevel::INFO) << "listen success: " << _listensockfd;  // 打印监听成功日志
    }



    //7.“服务器启动函数” ---> 进入事件循环，持续接收客户端连接并处理
    void Run()
    {
        //1.标记服务器为"运行中"状态
        _isrunning = true; 

        //2.事件循环 ---> 只要服务器运行，就持续接收客户端连接
        while (_isrunning)
        {
            //2.1：存储客户端地址信息（原始sockaddr_in结构）
            struct sockaddr_in peer; 
            //2.2：地址结构体大小
            socklen_t len = sizeof(sockaddr_in); 
            //2.3：接收客户端连接（accept函数）---> 阻塞等待客户端连接，成功返回客户端套接字
            int sockfd = accept(_listensockfd, CONV(peer), &len);
            if (sockfd < 0)  
            {
                //1)打印警告日志（非致命错误，继续循环）
                LOG(LogLevel::WARNING) << "accept error";  

                //2)跳过当前循环，继续等待下一个连接
                continue;  
            }
            
            //2.3：封装客户端地址为InetAddr对象 ---> 简化IP/端口的打印和操作
            InetAddr addr(peer);
            //2.4：打印信息日志 ---> 记录客户端连接成功（输出客户端IP和端口）
            LOG(LogLevel::INFO) << "accept success, peer addr : " << addr.StringAddr();

            // /* ================================= 【版本1：单进程版本】 ================================= */
            // //缺点：同一时间只能处理一个客户端，处理完一个才能接收下一个（无并发能力）
            // Service(sockfd, addr);

            // /* ================================= 【版本2：多进程版本】 ================================= */
            // // 核心逻辑：为每个客户端连接创建子进程，由子进程处理通信（通过两次fork避免僵尸进程）
            // //1.创建子进程
            // pid_t id = fork(); 
            // if(id < 0)
            // {
            //     LOG(LogLevel::FATAL) << "fork error";
            //     exit(FORK_ERR);
            // }
            // //2.子进程
            // else if(id == 0)
            // {

            //     //2.1：关闭监听套接字（子进程无需监听，避免资源泄漏）
            //     close(_listensockfd);

            //     //2.2：二次fork：子进程退出，孙子进程成为孤儿进程（由init进程回收）
            //     if(fork() > 0)
            //     {
            //         exit(OK);
            //     }

            //     //2.3：孙子进程处理业务
            //     Service(sockfd, addr); 
            //     exit(OK);
            // }
            // //3.父进程
            // else
            // {
            //     //3.1：关闭客户端套接字（父进程仅负责接收连接，不处理通信）
            //     close(sockfd);

            //     //3.2：等待子进程退出（子进程二次fork后立即退出，不会阻塞父进程）
            //     pid_t rid = waitpid(id, nullptr, 0);
            //     (void)rid; // 消除未使用变量警告
            // }



            // /* ================================= 【版本3：多线程版本】 =================================  */
            // // 核心逻辑：为每个客户端连接创建一个独立线程，由线程处理与客户端的通信
            // //1.动态创建ThreadData对象，封装客户端套接字、地址、服务器指针
            // ThreadData *td = new ThreadData(sockfd, addr, this);
            // //2.存储新线程ID
            // pthread_t tid; 

            // //3.创建线程 
            // pthread_create(&tid, nullptr, Routine, td); // 调用Routine函数作为线程入口，传入ThreadData对象指针



            /* ================================= 【版本4：线程池版本】=================================  */
            // 核心逻辑：将客户端通信任务提交到线程池，由线程池中的线程处理（适合短连接、高并发场景）
            
            ThreadPool<task_t>::GetInstance()->Enqueue([this, sockfd, &addr](){
                this->Service(sockfd, addr); 
            });
        }

        //3.服务器退出循环后，标记为"未运行"状态
        _isrunning = false;  
    }

};