#pragma once  

#include <iostream>              
#include <string>               
#include <functional>             
#include "InetAddr.hpp"          

// 前向声明：解决循环依赖问题（Connection需要知道Reactor，Reactor也需要知道Connection）
class Reactor;
class Connection;

// 定义数据处理回调函数类型：统一规范所有连接的数据处理逻辑
using handler_t = std::function<std::string (std::string &)>;

// Connection类：所有网络连接的抽象基类
class Connection
{
private:
    //1.当前连接关注的epoll事件掩码（如：EPOLLIN|EPOLLET）---> 仅能通过SetEvent/GetEvent访问，保证数据封装
    //2.回指指针指向所属的Reactor对象 ---> 仅能通过SetOwner/GetOwner访问，避免外部非法修改
    uint32_t _events;
    Reactor *_owner;

public:
    /*-----------------------------------------------【公有属性】-----------------------------------------------*/
    //3.数据处理回调函数（公有权限，便于子类直接调用）---> 子类（如Channel）读取数据后，调用此回调处理业务逻辑
    handler_t _handler;

    /*-----------------------------------------------【公有方法】-----------------------------------------------*/

    /*===================================== “构造&析构” =====================================*/
    //1.“构造函数” ---> 初始化连接的核心属性
    Connection()
    :_events(0),    // _events：初始化关注的事件为无（0）
    _owner(nullptr) // _owner：初始化所属Reactor指针为空（后续由Reactor设置）
    { }

    //2.“析构函数” ---> 基类析构函数需为虚函数（此处虽为空，但保证子类析构时正确调用）
    ~Connection()
    {}


    /*===================================== “get&set” =====================================*/
    //3.设置当前连接关注的epoll事件
    void SetEvent(const uint32_t &events)
    {
        _events = events;
    }
    //4.获取当前连接关注的epoll事件 ---> 供Reactor/Epoller注册到epoll内核
    uint32_t GetEvent()
    {
        return _events;
    }

    //5.设置当前连接的所属Reactor（回指指针）---> 让Connection知道自己被哪个Reactor管理，便于调用Reactor的接口（如AddConnection/DelConnection）
    void SetOwner(Reactor *owner)
    {
        _owner = owner;
    }
    //6.获取当前连接的所属Reactor
    Reactor *GetOwner()
    {
        return _owner;
    }

    //7.注册数据处理回调函数 ---> 将业务处理逻辑注入到Connection，实现网络层与业务层解耦
    void RegisterHandler(handler_t handler)
    {
        _handler = handler;
    }



    /*===================================== “纯虚函数” =====================================*/

    //1.定义读事件处理接口（由子类实现具体逻辑）
    // 子类实现：
    // - Listener：处理监听fd的读事件（accept新连接）
    // - Channel：处理客户端fd的读事件（读取客户端数据）
    virtual void Recver() = 0;

    //2.定义写事件处理接口（由子类实现具体逻辑）
    // 子类实现：
    // - Listener：空实现（监听fd无需写事件）
    // - Channel：处理客户端fd的写事件（发送响应数据）
    virtual void Sender() = 0;

    //3.定义异常事件处理接口（由子类实现具体逻辑）----> 处理fd的错误/挂起等异常（如EPOLLERR/EPOLLHUP）
    virtual void Excepter() = 0;

    //4.获取当前连接对应的fd（由子类实现）---> 供Reactor/Epoller定位具体的fd
    // 子类实现：
    // - Listener：返回监听fd
    // - Channel：返回客户端fd
    virtual int GetSockFd() = 0;
};