#pragma once  

#include <iostream>               
#include <memory>                
#include <unordered_map>     

#include "Epoller.hpp"            // Epoll封装类（封装epoll_create/ctl/wait等接口）
#include "Connection.hpp"         // 连接封装类（封装fd、事件、读写回调等）
#include "Log.hpp"                
using namespace LogModule;        

// Reactor类：反应堆模式核心实现
class Reactor // 核心职责：管理所有网络连接（Connection）、驱动epoll事件循环、分发就绪事件、维护fd与Connection的映射
{
    //1.epoll_wait单次最多处理的就绪事件数量（可根据并发调整）
    static const int revs_num = 128; 

private:
    /*-----------------------------------------【私有属性】-----------------------------------------*/
    //1.Epoll模型封装 --->     智能指针独占，自动释放epoll内核实例
    //2.Reactor运行状态标记 -> true=运行中，false=已停止
    //3.连接管理哈希表 --->    key=fd，value=Connection智能指针，高效映射fd到连接对象
    //4.就绪事件数组 --->      存储epoll_wait返回的就绪事件，单次最多存储revs_num个

    std::unique_ptr<Epoller> _epoller_ptr;
    bool _isrunning;
    std::unordered_map<int, std::shared_ptr<Connection>> _connections;
    struct epoll_event _revs[revs_num];

    /*-----------------------------------------【私有方法】-----------------------------------------*/
    /*========================== “内部辅助函数” ==========================*/
    //1.检查指定fd对应的Connection是否存在（核心逻辑抽离）
    bool IsConnectionExistsHelper(int sockfd) 
    {
        auto iter = _connections.find(sockfd);
        if (iter == _connections.end())
        {
            return false;
        }
        else
        {
            return true;
        }

    }


    /*========================== “对外封装接口” ==========================*/
    //1.通过Connection对象检查连接是否存在
    bool IsConnectionExists(const std::shared_ptr<Connection> &conn)
    {
        return IsConnectionExistsHelper(conn->GetSockFd());
    }
    //2.通过fd检查连接是否存在
    bool IsConnectionExists(int sockfd)
    {
        return IsConnectionExistsHelper(sockfd);
    }


    //3.检查Reactor管理的连接是否为空
    bool IsConnectionEmpty()
    {
        return _connections.empty();
    }


    /*========================== “核心函数” ==========================*/
    //1.单次事件循环 ---> 调用epoll_wait等待事件就绪
    int LoopOnce(int timeout)
    {
        return _epoller_ptr->WaitEvents(_revs, revs_num, timeout); //注意：调用Epoller封装的WaitEvents，本质是epoll_wait，就绪事件存入_revs数组
    }

    //2.事件派发器 ---> 处理epoll_wait返回的就绪事件
    void Dispatcher(int n)
    {
        //1.遍历所有就绪事件
        for (int i = 0; i < n; i++)
        {
            //1.1：获取就绪事件对应的fd
            int sockfd = _revs[i].data.fd;     

            //1.2：获取就绪的事件类型（EPOLLIN/EPOLLOUT/EPOLLERR等）
            uint32_t revents = _revs[i].events;

            //1.3：异常事件统一处理 ---> 将EPOLLERR（错误）、EPOLLHUP（挂起）转为读写事件
            if (revents & EPOLLERR) // 目的：简化异常处理逻辑，所有异常都通过 ---> 读写事件的错误处理流程处理
            {
                revents |= (EPOLLIN | EPOLLOUT);
            }
            if (revents & EPOLLHUP)
            {
                revents |= (EPOLLIN | EPOLLOUT);
            }

            //1.4：处理读事件就绪（新连接/客户端数据可读）
            if (revents & EPOLLIN)
            {
                if (IsConnectionExists(sockfd))     // 确保fd对应的Connection存在，避免野指针
                {
                    _connections[sockfd]->Recver(); // 调用Connection的读处理函数
                }
            }

            //1.5：处理写事件就绪（可向客户端发送数据）
            if (revents & EPOLLOUT)
            {
                if (IsConnectionExists(sockfd))     // 确保fd对应的Connection存在
                {
                    _connections[sockfd]->Sender(); // 调用Connection的写处理函数
                }
            }
        }
    }

public:
    /*-------------------------------------------------------------------------------------------------------*/
    //1.“构造函数” ---> 初始化Reactor核心组件
    Reactor()
        : _epoller_ptr(std::make_unique<Epoller>()),  // 创建Epoller对象（封装epoll内核实例）
          _isrunning(false)                           // 初始化运行状态为停止
    { }

    //2.“析构函数” ---> Reactor销毁时自动释放资源（智能指针自动管理Epoller）
    ~Reactor()
    { }


    //3.“启动Reactor事件循环” ---> 持续等待并处理事件
    void Loop()
    {
        //1.无管理的连接时直接返回，避免空循环
        if (IsConnectionEmpty())
        {
            return;
        }

        //2.标记Reactor进入运行状态
        _isrunning = true;  

        //3.epoll_wait超时时间：永久阻塞（直到有事件就绪）
        int timeout = -1;   

        //4.事件循环主逻辑：持续运行直到_stop被调用
        while (_isrunning)
        {
            PrintConnection();         // 打印当前管理的所有fd
            int n = LoopOnce(timeout); // 单次事件等待，获取就绪事件数
            Dispatcher(n);             // 派发并处理就绪事件
        }

        //5.重置运行状态为停止
        _isrunning = false; 
    }

    //4.“停止Reactor事件循环” ---> 
    void Stop()
    {
        _isrunning = false; // 修改运行状态，事件循环会在下一次迭代退出
    }


    /*-------------------------------------------------------------------------------------------------------*/
    //1.“添加新连接到Reactor管理” ---> 
    void AddConnection(std::shared_ptr<Connection> &conn)
    {
        //1.前置检查避免重复添加同一fd
        if (IsConnectionExists(conn))
        {
            LOG(LogLevel::WARNING) << "conn is exists: " << conn->GetSockFd();
            return;
        }

        //2.从Connection获取要监听的事件，注册到epoll内核
        int sockfd = conn->GetSockFd();         // 获取Connection对应的fd
        uint32_t events = conn->GetEvent();     // 获取Connection关注的事件（如EPOLLIN）
        _epoller_ptr->AddEvent(sockfd, events); // 调用Epoller封装的AddEvent（本质epoll_ctl ADD）

        //3.设置Connection的归属Reactor（让Connection知道自己被哪个Reactor管理）
        conn->SetOwner(this);

        //4.将Connection加入哈希表（建立fd到Connection的映射，便于快速查找）
        _connections[sockfd] = conn;
    }


    //2.“修改fd对应的事件监听状态” ---> 
    void EnableReadWrite(int sockfd, bool enableread, bool enablewrite)
    {
        //1.前置检查：fd不存在则直接返回
        if (!IsConnectionExists(sockfd))
        {
            LOG(LogLevel::WARNING) << "EnableReadWrite, conn not exists: " << sockfd;
            return;
        }

        //2.计算新的事件掩码：ET边缘触发 + 读事件（可选） + 写事件（可选）
        uint32_t new_event = (EPOLLET | (enableread ? EPOLLIN : 0) | (enablewrite ? EPOLLOUT : 0));
        
        //3.更新Connection内部的事件状态
        _connections[sockfd]->SetEvent(new_event);

        //4.将新事件状态同步到epoll内核（本质epoll_ctl MOD）
        _epoller_ptr->ModEvent(sockfd, new_event);
    }

    //3.“删除指定fd的连接并释放资源” ---> 流程：epoll移除事件 → 哈希表删除映射 → 关闭fd → 日志记录
    void DelConnection(int sockfd)
    {
        //1.从epoll内核中移除该fd的事件监听（epoll_ctl DEL）
        _epoller_ptr->DelEvent(sockfd);

        //2.从哈希表中删除fd到Connection的映射，释放Connection（智能指针自动计数减1）
        _connections.erase(sockfd);

        //3.关闭fd，释放系统资源
        close(sockfd);

        //4.记录客户端退出日志
        LOG(LogLevel::INFO) << "client quit: " << sockfd;
    }


    //4.打印当前Reactor管理的所有fd
    void PrintConnection()
    {
        std::cout << "当前Reactor正在进行管理的fd List:";
        for(auto &conn : _connections)
        {
            std::cout << conn.second->GetSockFd() << " ";
        }
        std::cout << "\r\n";
    }
};